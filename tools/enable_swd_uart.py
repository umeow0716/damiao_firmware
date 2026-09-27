#!/usr/bin/env python3
"""Send the exact recovered UART command that enables SWD access."""

from __future__ import annotations

import argparse
import os
import select
import termios
import time
from pathlib import Path


COMMAND = b"\xff\xfemxpshenzhenshidamiaokejiyouxiangongsi"
EXPECTED_REPLY = b"Enable SWD!  \r\n"


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
    termios.tcflush(fd, termios.TCIOFLUSH)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("device", type=Path, help="UART TTY, for example /dev/ttyUSB0")
    parser.add_argument("--yes", action="store_true",
                        help="actually transmit; without this flag only print the payload")
    parser.add_argument("--timeout", type=float, default=1.0)
    args = parser.parse_args()

    print(f"payload ({len(COMMAND)} bytes): {COMMAND.hex(' ')}")
    if not args.yes:
        print("dry run: add --yes to transmit")
        return

    fd = os.open(args.device, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        configure_uart(fd)
        written = os.write(fd, COMMAND)
        if written != len(COMMAND):
            raise RuntimeError(f"short UART write: {written}/{len(COMMAND)}")
        termios.tcdrain(fd)

        deadline = time.monotonic() + args.timeout
        reply = bytearray()
        while time.monotonic() < deadline:
            readable, _, _ = select.select([fd], [], [], deadline - time.monotonic())
            if not readable:
                break
            part = os.read(fd, 256)
            if part:
                reply.extend(part)
                if EXPECTED_REPLY in reply:
                    break
        if EXPECTED_REPLY not in reply:
            raise SystemExit(f"command sent, but expected reply was not observed: {reply!r}")
        print("SWD enable acknowledged by bootloader")
    finally:
        os.close(fd)


if __name__ == "__main__":
    main()
