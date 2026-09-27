#!/usr/bin/env python3
"""Send a packaged DM4310 update through Linux SocketCAN.

This sender paces individual CAN/CAN-FD frames and waits for the original
bootloader's 0x7fe acknowledgement after each encrypted chunk.  Supplying
--yes is mandatory because a successful transfer erases/programs app flash.
"""

from __future__ import annotations

import argparse
import json
import socket
import struct
import sys
import time
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path


UPDATE_CAN_ID = 0x7FF
REPLY_CAN_ID = 0x7FE
CAN_SFF_MASK = 0x7FF
CAN_EFF_FLAG = 0x80000000
CAN_RTR_FLAG = 0x40000000
CAN_FRAME = struct.Struct("=IB3x8s")
CANFD_FRAME = struct.Struct("=IBBBB64s")
CANFD_BRS = 0x01
SOL_CAN_RAW = getattr(socket, "SOL_CAN_RAW", 101)
CAN_RAW_FD_FRAMES = getattr(socket, "CAN_RAW_FD_FRAMES", 5)


@dataclass(frozen=True)
class LoadedUpdate:
    chunks: list[list[bytes]]
    can_fd: bool
    bit_rate_switch: bool


class RecoverableUpdateError(RuntimeError):
    """The loader rejected only the current packet and permits a retry."""


def crc8_maxim(data: bytes) -> int:
    value = 0
    for item in data:
        value ^= item
        for _ in range(8):
            value = (value >> 1) ^ (0x8C if value & 1 else 0)
    return value


def load_chunks(path: Path) -> LoadedUpdate:
    chunks: dict[int, list[tuple[int, bytes]]] = defaultdict(list)
    transport_modes: set[tuple[bool, bool]] = set()
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line.strip():
            continue
        try:
            item = json.loads(line)
            can_id = int(item["can_id"])
            chunk = int(item["chunk"])
            frame = int(item["frame"])
            data = bytes.fromhex(item["data"])
            can_fd = bool(item.get("can_fd", False))
            bit_rate_switch = bool(item.get("bit_rate_switch", False))
        except (KeyError, TypeError, ValueError, json.JSONDecodeError) as error:
            raise ValueError(f"invalid frame at {path}:{line_number}: {error}") from error
        if can_id != UPDATE_CAN_ID or not 0 < len(data) <= 8:
            raise ValueError(f"invalid CAN id/DLC at {path}:{line_number}")
        if bit_rate_switch and not can_fd:
            raise ValueError(f"BRS without CAN FD at {path}:{line_number}")
        transport_modes.add((can_fd, bit_rate_switch))
        chunks[chunk].append((frame, data))

    if not chunks or sorted(chunks) != list(range(len(chunks))):
        raise ValueError("chunk indexes must be contiguous and start at zero")

    result: list[list[bytes]] = []
    expected_sequence = len(chunks) - 1
    for chunk_index in range(len(chunks)):
        numbered = sorted(chunks[chunk_index])
        if [number for number, _ in numbered] != list(range(len(numbered))):
            raise ValueError(f"frame indexes in chunk {chunk_index} are not contiguous")
        frames = [data for _, data in numbered]
        record = b"".join(frames)
        if len(record) < 6 or record[0] != 0x23 or record[2] != 0x23:
            raise ValueError(f"chunk {chunk_index} has an invalid update header")
        length = int.from_bytes(record[3:5], "little")
        if record[1] != expected_sequence - chunk_index:
            raise ValueError(f"chunk {chunk_index} has an invalid descending sequence")
        if len(record) != 5 + length + 1:
            raise ValueError(f"chunk {chunk_index} record length does not match header")
        if crc8_maxim(record[5:-1]) != record[-1]:
            raise ValueError(f"chunk {chunk_index} has an invalid CRC-8/MAXIM")
        result.append(frames)
    if len(transport_modes) != 1:
        raise ValueError("all update frames must use one CAN transport mode")
    can_fd, bit_rate_switch = transport_modes.pop()
    return LoadedUpdate(result, can_fd, bit_rate_switch)


def pack_can_frame(can_id: int, data: bytes) -> bytes:
    if not 0 <= can_id <= CAN_SFF_MASK or not 0 <= len(data) <= 8:
        raise ValueError("only classic standard-ID CAN frames are supported")
    return CAN_FRAME.pack(can_id, len(data), data.ljust(8, b"\0"))


def unpack_can_frame(frame: bytes) -> tuple[int, bytes]:
    if len(frame) == CAN_FRAME.size:
        can_id, dlc, data = CAN_FRAME.unpack(frame)
        limit = 8
    elif len(frame) == CANFD_FRAME.size:
        can_id, dlc, _flags, _res0, _res1, data = CANFD_FRAME.unpack(frame)
        limit = 64
    else:
        raise ValueError(f"unexpected SocketCAN frame size {len(frame)}")
    if can_id & (CAN_EFF_FLAG | CAN_RTR_FLAG):
        return -1, b""
    return can_id & CAN_SFF_MASK, data[:min(dlc, limit)]


def pack_canfd_frame(can_id: int, data: bytes,
                     bit_rate_switch: bool = True) -> bytes:
    if not 0 <= can_id <= CAN_SFF_MASK or not 0 <= len(data) <= 64:
        raise ValueError("only standard-ID CAN FD frames are supported")
    flags = CANFD_BRS if bit_rate_switch else 0
    return CANFD_FRAME.pack(can_id, len(data), flags, 0, 0,
                            data.ljust(64, b"\0"))


def wait_for_reply(bus: socket.socket, timeout: float, require_complete: bool) -> None:
    deadline = time.monotonic() + timeout
    saw_ok = False
    saw_complete = not require_complete
    replies: list[str] = []
    while time.monotonic() < deadline and not (saw_ok and saw_complete):
        bus.settimeout(max(0.001, deadline - time.monotonic()))
        try:
            can_id, data = unpack_can_frame(bus.recv(CANFD_FRAME.size))
        except socket.timeout:
            break
        if can_id != REPLY_CAN_ID:
            continue
        text = data.rstrip(b"\0").decode("ascii", errors="replace")
        replies.append(text)
        if data.startswith((b"CRCERROR", b"TimERROR")):
            raise RecoverableUpdateError(
                f"bootloader rejected current chunk: {text}"
            )
        if data.startswith((b"EFMERROR", b"APPERROR")):
            raise RuntimeError(f"bootloader rejected update: {text}")
        saw_ok |= b"OK!" in data
        saw_complete |= b"complete" in data
    if not (saw_ok and saw_complete):
        expected = "OK and complete" if require_complete else "OK"
        raise TimeoutError(
            f"no {expected} reply from CAN 0x{REPLY_CAN_ID:03x}; replies={replies!r}"
        )


def send_over_bus(bus: socket.socket, update: LoadedUpdate, timeout: float,
                  frame_delay: float, chunk_retries: int) -> None:
    """Send an update over an already configured bus (also host-testable)."""
    for chunk_index, frames in enumerate(update.chunks):
        final = chunk_index == len(update.chunks) - 1
        for attempt in range(chunk_retries + 1):
            for data in frames:
                encoded = (pack_canfd_frame(UPDATE_CAN_ID, data,
                                            update.bit_rate_switch)
                           if update.can_fd
                           else pack_can_frame(UPDATE_CAN_ID, data))
                bus.send(encoded)
                if frame_delay:
                    time.sleep(frame_delay)
            try:
                wait_for_reply(bus, timeout, require_complete=final)
                break
            except RecoverableUpdateError:
                if attempt == chunk_retries:
                    raise
                print(
                    f"retrying chunk {chunk_index + 1}/"
                    f"{len(update.chunks)} after recoverable loader error "
                    f"({attempt + 1}/{chunk_retries})",
                    file=sys.stderr,
                    flush=True,
                )
        print(f"acknowledged chunk {chunk_index + 1}/{len(update.chunks)}",
              flush=True)


def send(interface: str, update: LoadedUpdate, timeout: float,
         frame_delay: float, chunk_retries: int) -> None:
    if not hasattr(socket, "AF_CAN"):
        raise RuntimeError("this Python/platform does not provide Linux SocketCAN")
    with socket.socket(socket.AF_CAN, socket.SOCK_RAW, socket.CAN_RAW) as bus:
        if update.can_fd:
            bus.setsockopt(SOL_CAN_RAW, CAN_RAW_FD_FRAMES, 1)
        bus.bind((interface,))
        send_over_bus(bus, update, timeout, frame_delay, chunk_retries)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--interface", default="can0")
    parser.add_argument("--frames", type=Path, required=True)
    parser.add_argument("--reply-timeout", type=float, default=3.0)
    parser.add_argument("--frame-delay-ms", type=float, default=0.2)
    parser.add_argument(
        "--chunk-retries", type=int, default=2,
        help="retry a chunk after CRCERROR/TimERROR (default: 2)",
    )
    parser.add_argument(
        "--yes", action="store_true",
        help="confirm that programming application flash is intended",
    )
    args = parser.parse_args()
    if not args.yes:
        parser.error("refusing to program flash without --yes")
    if (args.reply_timeout <= 0 or args.frame_delay_ms < 0 or
            args.chunk_retries < 0):
        parser.error("timeouts/delays must be non-negative and timeout must be positive")
    update = load_chunks(args.frames)
    mode = "CAN FD+BRS" if update.bit_rate_switch else (
        "CAN FD" if update.can_fd else "Classic CAN"
    )
    print(
        f"sending {len(update.chunks)} encrypted chunks over {mode} on "
        f"{args.interface}; "
        "the target must already be waiting in its bootloader",
        file=sys.stderr,
    )
    send(args.interface, update, args.reply_timeout,
         args.frame_delay_ms / 1000.0, args.chunk_retries)


if __name__ == "__main__":
    main()
