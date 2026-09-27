#!/usr/bin/env python3
"""Provision the captured per-device calibration through the source APP UART.

The operation is deliberately two-stage: this tool only writes calibration
while the bridge remains disarmed, then the operator must reset the target so
the APP can validate the Flash records during its normal startup sequence.
"""

from __future__ import annotations

import argparse
import hashlib
import math
import os
import select
import struct
import termios
import time
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CAPTURE_SHA256 = "b906c084654ef5275fa4d80fcb11f0e510b488c1830c711174cc15ec56b18155"
CAPTURE_BASE = 0x1FFFA510
MOTOR_CORRECTION_ADDRESS = 0x1FFFC84C
MOTOR_STATE_ADDRESS = 0x1FFFF088
MOTOR_ELECTRICAL_OFFSET_OFFSET = 0x14
MOTOR_DIRECTION_OFFSET = 0x34
OUTPUT_TABLE_ADDRESS = 0x1FFFD078
OUTPUT_PARAMETERS_ADDRESS = 0x1FFFF078

MOTOR_RECORD_WORDS = 259
OUTPUT_TABLE_ENTRIES = 4096
CHUNK_BYTES = 60
FRAME_BYTES = 64
BAUD = 921600


@dataclass(frozen=True)
class CalibrationBundle:
    motor_record: bytes
    output_table: bytes
    output_parameters: tuple[float, float, float, float]
    capture_sha256: str


def _capture_slice(data: bytes, address: int, size: int) -> bytes:
    offset = address - CAPTURE_BASE
    if offset < 0 or offset + size > len(data):
        raise ValueError(f"capture does not contain 0x{address:08x}..0x{address + size:08x}")
    return data[offset:offset + size]


def load_bundle(capture: Path) -> CalibrationBundle:
    data = capture.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != CAPTURE_SHA256:
        raise ValueError(
            f"capture hash {digest} is not the analyzed target capture {CAPTURE_SHA256}"
        )

    correction = _capture_slice(data, MOTOR_CORRECTION_ADDRESS, 256 * 4)
    electrical_offset = _capture_slice(
        data,
        MOTOR_STATE_ADDRESS + MOTOR_ELECTRICAL_OFFSET_OFFSET,
        4,
    )
    direction = _capture_slice(
        data,
        MOTOR_STATE_ADDRESS + MOTOR_DIRECTION_OFFSET,
        4,
    )
    correction_values = struct.unpack("<256f", correction)
    offset_value = struct.unpack("<f", electrical_offset)[0]
    direction_value = struct.unpack("<f", direction)[0]
    if not all(math.isfinite(value) and abs(value) <= 64.0
               for value in correction_values):
        raise ValueError("captured motor correction table is invalid")
    if not math.isfinite(offset_value) or direction_value not in (1.0, 2.0):
        raise ValueError("captured motor calibration metadata is invalid")

    # Word 257 is not consumed by the recovered loader.  Keep it erased/zero;
    # words 256 and 258 are the electrical offset and direction respectively.
    motor_record = bytearray(MOTOR_RECORD_WORDS * 4)
    motor_record[:len(correction)] = correction
    motor_record[256 * 4:257 * 4] = electrical_offset
    motor_record[258 * 4:259 * 4] = direction

    output_table = _capture_slice(
        data, OUTPUT_TABLE_ADDRESS, OUTPUT_TABLE_ENTRIES * 2
    )
    output_parameters = struct.unpack(
        "<4f", _capture_slice(data, OUTPUT_PARAMETERS_ADDRESS, 4 * 4)
    )
    if (not all(math.isfinite(value) for value in output_parameters)
            or not 0.0 <= output_parameters[0] <= 4095.0
            or not 0.0 <= output_parameters[1] <= 4095.0
            or not 0.25 <= output_parameters[2] <= 4.0):
        raise ValueError("captured output-sensor parameters are invalid")

    return CalibrationBundle(
        motor_record=bytes(motor_record),
        output_table=output_table,
        output_parameters=output_parameters,
        capture_sha256=digest,
    )


def make_upload_frames(command: int, payload: bytes, element_size: int) -> list[bytes]:
    if command not in (ord("d"), ord("M")) or element_size not in (2, 4):
        raise ValueError("invalid calibration upload kind")
    frames: list[bytes] = []
    for index, offset in enumerate(range(0, len(payload), CHUNK_BYTES)):
        chunk = payload[offset:offset + CHUNK_BYTES]
        if len(chunk) % element_size != 0 or index > 255:
            raise ValueError("payload cannot be represented by the UART framing")
        frame = bytearray(FRAME_BYTES)
        frame[0] = ord("U")
        frame[1] = command
        frame[2:2 + len(chunk)] = chunk
        frame[62] = len(chunk) // element_size
        frame[63] = index
        frames.append(bytes(frame))
    return frames


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


def write_all(fd: int, payload: bytes) -> None:
    offset = 0
    while offset < len(payload):
        written = os.write(fd, payload[offset:])
        if written <= 0:
            raise RuntimeError("UART write made no progress")
        offset += written
    termios.tcdrain(fd)


def read_until(fd: int, marker: bytes, timeout: float) -> bytes:
    deadline = time.monotonic() + timeout
    received = bytearray()
    while time.monotonic() < deadline:
        readable, _, _ = select.select([fd], [], [], max(0.0, deadline - time.monotonic()))
        if not readable:
            break
        part = os.read(fd, 512)
        if part:
            received.extend(part)
            if marker in received:
                return bytes(received)
    raise RuntimeError(
        f"UART acknowledgement {marker!r} not observed; received {bytes(received)!r}"
    )


def discard_input(fd: int, quiet_time: float = 0.02) -> None:
    """Drain already queued console text without discarding future bytes."""
    deadline = time.monotonic() + quiet_time
    while time.monotonic() < deadline:
        readable, _, _ = select.select([fd], [], [], max(0.0, deadline - time.monotonic()))
        if not readable:
            return
        if os.read(fd, 512):
            deadline = time.monotonic() + quiet_time


def query_identity(fd: int, timeout: float) -> tuple[int, int]:
    write_all(fd, b"s")
    discard_input(fd)
    write_all(fd, b"Uf\xaa")
    deadline = time.monotonic() + timeout
    reply = bytearray()
    while time.monotonic() < deadline:
        readable, _, _ = select.select([fd], [], [], max(0.0, deadline - time.monotonic()))
        if not readable:
            break
        part = os.read(fd, 512)
        if part:
            reply.extend(part)
            for marker in range(len(reply)):
                if reply[marker] == ord("f") and len(reply) >= marker + 9:
                    return struct.unpack_from("<II", reply, marker + 1)
    raise RuntimeError(f"complete Uf identity reply not observed; received {bytes(reply)!r}")


def send_frames(fd: int, frames: list[bytes], timeout: float,
                flash_settle: float) -> None:
    for index, frame in enumerate(frames):
        acknowledgement = bytes((frame[1], frame[63]))
        write_all(fd, frame)
        read_until(fd, acknowledgement, timeout)
        if index + 1 == len(frames):
            # The factory protocol acknowledges the final indexed chunk
            # before the main loop performs the deferred Flash replacement;
            # it sends no second commit-status message.
            time.sleep(flash_settle)


def write_bundle_files(bundle: CalibrationBundle, output_dir: Path) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    motor_frames = make_upload_frames(ord("d"), bundle.motor_record, 4)
    output_frames = make_upload_frames(ord("M"), bundle.output_table, 2)
    (output_dir / "motor_encoder_record.bin").write_bytes(bundle.motor_record)
    (output_dir / "output_sensor_table.bin").write_bytes(bundle.output_table)
    (output_dir / "motor_encoder_uart.frames.bin").write_bytes(b"".join(motor_frames))
    (output_dir / "output_sensor_uart.frames.bin").write_bytes(b"".join(output_frames))
    parameters = "\n".join(value.hex() for value in bundle.output_parameters) + "\n"
    (output_dir / "output_sensor_parameters.txt").write_text(parameters, encoding="ascii")


def parse_u32(text: str) -> int:
    value = int(text, 0)
    if not 0 <= value <= 0xFFFFFFFF:
        raise argparse.ArgumentTypeError("must fit in uint32")
    return value


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture", type=Path,
                        default=ROOT / "bins/app_runtime_config.bin")
    parser.add_argument("--output-dir", type=Path,
                        default=ROOT / "dist/calibration")
    parser.add_argument("--device", type=Path,
                        help="UART TTY, for example /dev/ttyUSB0")
    parser.add_argument("--expected-device-id", type=parse_u32,
                        help="required with --yes; compared with the live Uf reply")
    parser.add_argument("--timeout", type=float, default=1.0)
    parser.add_argument("--flash-settle", type=float, default=0.5)
    parser.add_argument("--yes", action="store_true",
                        help="write device Flash; otherwise only export/describe frames")
    args = parser.parse_args()

    bundle = load_bundle(args.capture)
    write_bundle_files(bundle, args.output_dir)
    motor_frames = make_upload_frames(ord("d"), bundle.motor_record, 4)
    output_frames = make_upload_frames(ord("M"), bundle.output_table, 2)
    print(f"capture: {bundle.capture_sha256}")
    print(f"motor record: {len(bundle.motor_record)} bytes, {len(motor_frames)} frames")
    print(f"output table: {len(bundle.output_table)} bytes, {len(output_frames)} frames")
    print(f"output parameters: {bundle.output_parameters!r}")
    print(f"exported calibration bundle to {args.output_dir}")

    if not args.yes:
        print("dry run: no UART or Flash access; add --device, --expected-device-id and --yes")
        return
    if args.device is None or args.expected_device_id is None:
        parser.error("--yes requires --device and --expected-device-id")
    if args.timeout <= 0.0 or args.flash_settle < 0.0:
        parser.error("timeouts must be positive")

    fd = os.open(args.device, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        configure_uart(fd)
        device_id, application_identity = query_identity(fd, args.timeout)
        print(
            f"live identity: device_id=0x{device_id:08x}, "
            f"application_identity=0x{application_identity:08x}"
        )
        if device_id != args.expected_device_id:
            raise SystemExit(
                f"refusing calibration write: expected device 0x{args.expected_device_id:08x}"
            )
        # Uf returns to the menu, so explicitly re-enter setup mode.
        write_all(fd, b"s")
        time.sleep(0.02)
        send_frames(fd, motor_frames, args.timeout, args.flash_settle)
        send_frames(fd, output_frames, args.timeout, args.flash_settle)
    finally:
        os.close(fd)

    print("all calibration frames were acknowledged by the APP")
    print("reset/power-cycle the target and confirm that startup accepts both sectors")


if __name__ == "__main__":
    main()
