#!/usr/bin/env python3
"""Capture and classify Damiao boot/APP output over 921600-8N1 UART.

Start this tool before manually resetting or power-cycling the controller.  It
does not reset, erase, flash, or otherwise modify the device.  ``--escape-after``
optionally sends one ESC byte so a running APP prints its menu.
"""

from __future__ import annotations

import argparse
import json
import os
import select
import termios
import time
from datetime import datetime
from pathlib import Path
from typing import Any


APP_MARKERS = (
    b"DMBOT Motor Driver",
    b"MOSFET ERROR,  WH | WL | VH | VL | UH | UL",
    b" Over Voltage!!!",
)
LOADER_FAILURE = b"Upgrade failed,please retry!"
UPDATE_COMPLETE = b"Update firmware completed!!!"
ENTER_LOADER = b"Enter Bootloader!"


def configure_uart(fd: int) -> None:
    if not hasattr(termios, "B921600"):
        raise RuntimeError("this platform's termios does not expose 921600 baud")
    attributes = termios.tcgetattr(fd)
    attributes[0] = 0
    attributes[1] = 0
    attributes[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    attributes[3] = 0
    attributes[4] = termios.B921600
    attributes[5] = termios.B921600
    attributes[6][termios.VMIN] = 0
    attributes[6][termios.VTIME] = 1
    termios.tcsetattr(fd, termios.TCSANOW, attributes)
    termios.tcflush(fd, termios.TCIFLUSH)


def _last_marker(data: bytes, markers: tuple[bytes, ...]) -> int:
    return max((data.rfind(marker) for marker in markers), default=-1)


def classify_capture(data: bytes) -> dict[str, Any]:
    app_at = _last_marker(data, APP_MARKERS)
    failed_at = data.rfind(LOADER_FAILURE)
    complete_at = data.rfind(UPDATE_COMPLETE)
    loader_at = data.rfind(ENTER_LOADER)

    if app_at >= 0 and app_at > failed_at:
        status = "app_running"
        conclusion = (
            "APP output was observed after any loader failure text; the first "
            "loader-to-APP handoff succeeded."
        )
    elif failed_at >= 0:
        status = "upgrade_failed"
        conclusion = (
            "The loader is in persistent (0,0) failure state; APP confirmation "
            "has not been demonstrated."
        )
    elif complete_at >= 0:
        status = "update_complete_no_app_evidence"
        conclusion = (
            "All UART packs completed, but this capture contains no proof that "
            "the APP started. Perform a warm reset while capturing again."
        )
    elif loader_at >= 0:
        status = "bootloader_waiting"
        conclusion = "The bootloader is alive; no APP startup marker was captured."
    elif app_at >= 0:
        status = "app_running"
        conclusion = "A reconstructed/original-compatible APP marker was observed."
    else:
        status = "no_decisive_output"
        conclusion = "No decisive loader or APP marker was captured."

    return {
        "status": status,
        "conclusion": conclusion,
        "bytes_captured": len(data),
        "markers": {
            "enter_bootloader": ENTER_LOADER in data,
            "update_complete": UPDATE_COMPLETE in data,
            "upgrade_failed": LOADER_FAILURE in data,
            "app_banner": b"DMBOT Motor Driver" in data,
            "app_power_stage_fault": APP_MARKERS[1] in data,
            "app_overvoltage_fault": APP_MARKERS[2] in data,
        },
    }


def escaped_text(data: bytes) -> str:
    result: list[str] = []
    for value in data:
        if value in (0x0A, 0x0D, 0x09):
            result.append(chr(value))
        elif 0x20 <= value <= 0x7E:
            result.append(chr(value))
        else:
            result.append(f"\\x{value:02x}")
    return "".join(result)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("device", type=Path, help="UART TTY, e.g. /dev/ttyUSB0")
    parser.add_argument("--duration", type=float, default=20.0,
                        help="capture duration in seconds (default: 20)")
    parser.add_argument("--escape-after", type=float,
                        help="send one ESC byte this many seconds after start")
    parser.add_argument("--label", default="boot",
                        help="short label used in the result directory")
    parser.add_argument("--output-root", type=Path,
                        default=Path("dist/target-validation"))
    return parser.parse_args()


def main() -> int:
    args = parse_arguments()
    if args.duration <= 0.0:
        raise SystemExit("--duration must be positive")
    if args.escape_after is not None and not (0.0 <= args.escape_after < args.duration):
        raise SystemExit("--escape-after must be within the capture duration")

    timestamp = datetime.now().astimezone().strftime("%Y%m%d-%H%M%S%z")
    safe_label = "".join(ch if ch.isalnum() or ch in "-_" else "_"
                         for ch in args.label)
    output_dir = args.output_root / f"{timestamp}-{safe_label}"
    output_dir.mkdir(parents=True, exist_ok=False)

    fd = os.open(args.device, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    captured = bytearray()
    sent_escape = False
    started = time.monotonic()
    deadline = started + args.duration
    print(f"capturing {args.device} for {args.duration:.1f}s; reset the device now")
    try:
        configure_uart(fd)
        while time.monotonic() < deadline:
            now = time.monotonic()
            if (args.escape_after is not None and not sent_escape and
                    now - started >= args.escape_after):
                if os.write(fd, b"\x1b") != 1:
                    raise RuntimeError("short UART write while sending ESC")
                termios.tcdrain(fd)
                sent_escape = True
            wait = min(0.2, max(0.0, deadline - now))
            readable, _, _ = select.select([fd], [], [], wait)
            if readable:
                part = os.read(fd, 4096)
                if part:
                    captured.extend(part)
                    print(escaped_text(part), end="", flush=True)
    finally:
        os.close(fd)

    data = bytes(captured)
    result = classify_capture(data)
    result.update({
        "schema": 1,
        "captured_at": datetime.now().astimezone().isoformat(),
        "device": str(args.device),
        "duration_seconds": args.duration,
        "escape_sent": sent_escape,
    })
    (output_dir / "uart.bin").write_bytes(data)
    (output_dir / "uart.txt").write_text(escaped_text(data), encoding="utf-8")
    (output_dir / "result.json").write_text(
        json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(f"\nresult: {result['status']} — {result['conclusion']}")
    print(f"evidence saved to {output_dir}")
    return 2 if result["status"] == "upgrade_failed" else 0


if __name__ == "__main__":
    raise SystemExit(main())
