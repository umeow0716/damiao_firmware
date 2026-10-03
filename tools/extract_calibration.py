#!/usr/bin/env python3
"""Generate exact C float tables from the SHA-256-locked live RAM capture."""

from __future__ import annotations

import argparse
import hashlib
import struct
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CAPTURE_SHA256 = "b906c084654ef5275fa4d80fcb11f0e510b488c1830c711174cc15ec56b18155"
CAPTURE_BASE = 0x1FFFA510
POSITION_LUT_ADDRESS = 0x1FFFC84C
MOTOR_SENSOR_STATE_ADDRESS = 0x1FFFF088
MOTOR_ELECTRICAL_OFFSET_OFFSET = 0x14
MOTOR_DIRECTION_OFFSET = 0x34
POSITION_RAW_TABLE_ADDRESS = 0x1FFFD078
OUTPUT_SENSOR_CALIBRATION_ADDRESS = 0x1FFFF078
SINE_LUT_ADDRESS = 0x1FFFA674
FIRMWARE_SHA256 = "65aab219268e9159b196d4578d3cd530e6fa90a731a82670a0d3d3be609b59d4"
FIRMWARE_BASE = 0x00020000
TEMPERATURE_LUT_ADDRESS = 0x00027CA0


def c_float_rows(words: tuple[int, ...]) -> str:
    rows = []
    for start in range(0, len(words), 4):
        values = []
        for word in words[start:start + 4]:
            value = struct.unpack("<f", struct.pack("<I", word))[0]
            values.append(f"{value.hex()}f")
        rows.append("    " + ", ".join(values) + ",")
    return "\n".join(rows)


def c_float_literal(word: int) -> str:
    value = struct.unpack("<f", struct.pack("<I", word))[0]
    return f"{value.hex()}f"


def c_uint16_rows(words: tuple[int, ...]) -> str:
    rows = []
    for start in range(0, len(words), 12):
        values = [f"0x{word:04x}U" for word in words[start:start + 12]]
        rows.append("    " + ", ".join(values) + ",")
    return "\n".join(rows)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture", type=Path,
                        default=ROOT / "bins/app_runtime_config.bin")
    parser.add_argument("--firmware", type=Path,
                        default=ROOT / "reference" / "V3" /
                        "APP_DM4310(V3)_V5017_04.decrypted.bin")
    parser.add_argument(
        "--fixture-header", type=Path,
        default=ROOT / "tests/generated/captured_calibration.h",
    )
    parser.add_argument(
        "--fixture-source", type=Path,
        default=ROOT / "tests/generated/captured_calibration.c",
    )
    parser.add_argument(
        "--sine-header", type=Path,
        default=ROOT / "common/generated/motor_sine_table.h",
    )
    parser.add_argument(
        "--sine-source", type=Path,
        default=ROOT / "common/generated/motor_sine_table.c",
    )
    parser.add_argument(
        "--temperature-header", type=Path,
        default=ROOT / "app/generated/temperature_table.h",
    )
    parser.add_argument(
        "--temperature-source", type=Path,
        default=ROOT / "app/generated/temperature_table.c",
    )
    args = parser.parse_args()

    data = args.capture.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != CAPTURE_SHA256:
        raise SystemExit(
            f"capture hash {digest} is not the analyzed app RAM snapshot"
        )
    offset = POSITION_LUT_ADDRESS - CAPTURE_BASE
    position_words = struct.unpack_from("<256I", data, offset)
    electrical_offset_word = struct.unpack_from(
        "<I", data,
        MOTOR_SENSOR_STATE_ADDRESS + MOTOR_ELECTRICAL_OFFSET_OFFSET -
        CAPTURE_BASE,
    )[0]
    motor_direction_word = struct.unpack_from(
        "<I", data,
        MOTOR_SENSOR_STATE_ADDRESS + MOTOR_DIRECTION_OFFSET - CAPTURE_BASE,
    )[0]
    raw_offset = POSITION_RAW_TABLE_ADDRESS - CAPTURE_BASE
    position_raw_words = struct.unpack_from("<4096H", data, raw_offset)
    output_calibration_offset = OUTPUT_SENSOR_CALIBRATION_ADDRESS - CAPTURE_BASE
    output_calibration_words = struct.unpack_from(
        "<4I", data, output_calibration_offset
    )
    sine_offset = SINE_LUT_ADDRESS - CAPTURE_BASE
    sine_words = struct.unpack_from("<2049I", data, sine_offset)
    for index in range(1025):
        opposite = sine_words[2048 - index]
        expected = 0 if sine_words[index] == 0 else sine_words[index] ^ 0x80000000
        if opposite != expected:
            raise SystemExit("captured sine table is not sign symmetric")
    for index in range(513):
        if sine_words[index] != sine_words[1024 - index]:
            raise SystemExit("captured sine table is not quarter-wave symmetric")
    sine_quarter_words = sine_words[:513]

    firmware = args.firmware.read_bytes()
    firmware_digest = hashlib.sha256(firmware).hexdigest()
    if firmware_digest != FIRMWARE_SHA256:
        raise SystemExit(
            f"firmware hash {firmware_digest} is not the analyzed app image"
        )
    temperature_offset = TEMPERATURE_LUT_ADDRESS - FIRMWARE_BASE
    temperature_words = struct.unpack_from("<256I", firmware,
                                           temperature_offset)

    args.fixture_header.parent.mkdir(parents=True, exist_ok=True)
    args.fixture_header.write_text(
        """/* Generated test fixture; do not hand-edit. */
#ifndef DAMIAO_CAPTURED_CALIBRATION_H
#define DAMIAO_CAPTURED_CALIBRATION_H

#include <stdint.h>

extern const float captured_motor_position_correction[256];
extern const float captured_motor_electrical_offset;
extern const float captured_motor_direction;
extern const uint16_t captured_output_position_table[4096];
extern const float captured_output_sensor_calibration[4];

#endif
""",
        encoding="utf-8",
    )
    args.fixture_source.write_text(
        "/* Generated test fixture; do not hand-edit. */\n"
        "#include \"captured_calibration.h\"\n\n"
        "const float captured_motor_electrical_offset = "
        + c_float_literal(electrical_offset_word)
        + ";\nconst float captured_motor_direction = "
        + c_float_literal(motor_direction_word)
        + ";\n\nconst float captured_motor_position_correction[256] = {\n"
        + c_float_rows(position_words)
        + "\n};\n\nconst uint16_t captured_output_position_table[4096] = {\n"
        + c_uint16_rows(position_raw_words)
        + "\n};\n\nconst float captured_output_sensor_calibration[4] = {\n"
        + c_float_rows(output_calibration_words)
        + "\n};\n",
        encoding="utf-8",
    )

    args.sine_header.parent.mkdir(parents=True, exist_ok=True)
    args.sine_header.write_text(
        """/* Generated lookup table; do not hand-edit. */
#ifndef DAMIAO_MOTOR_SINE_TABLE_H
#define DAMIAO_MOTOR_SINE_TABLE_H

extern const float motor_sine_quarter_table[513];

#endif
""",
        encoding="utf-8",
    )
    args.sine_source.write_text(
        "/* Generated lookup table; do not hand-edit. */\n"
        "#include \"motor_sine_table.h\"\n\n"
        "const float motor_sine_quarter_table[513] = {\n"
        + c_float_rows(sine_quarter_words)
        + "\n};\n",
        encoding="utf-8",
    )

    args.temperature_header.parent.mkdir(parents=True, exist_ok=True)
    args.temperature_header.write_text(
        """/* Generated lookup table; do not hand-edit. */
#ifndef DAMIAO_TEMPERATURE_TABLE_H
#define DAMIAO_TEMPERATURE_TABLE_H

extern const float temperature_celsius_table[256];

#endif
""",
        encoding="utf-8",
    )
    args.temperature_source.write_text(
        "/* Generated lookup table; do not hand-edit. */\n"
        "#include \"temperature_table.h\"\n\n"
        "const float temperature_celsius_table[256] = {\n"
        + c_float_rows(temperature_words)
        + "\n};\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
