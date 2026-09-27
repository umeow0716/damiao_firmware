#!/usr/bin/env python3
"""Verify the original IRQ data flow for rotor and output feedback.

This is intentionally narrower than a decompiler comparison.  It ties the
official APP's literal pointers and exact Thumb instructions to the two
velocity coordinate systems used by the controller.
"""

from __future__ import annotations

import csv
import hashlib
import struct
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OFFICIAL_APP = (
    ROOT / "reference/official/APP_DM4310_V3_V5017_04.decrypted.bin"
)
APP_ASM = ROOT / "recovered/raw/app_asm.tsv"

EXPECTED_APP_SHA256 = (
    "65aab219268e9159b196d4578d3cd530e6fa90a731a82670a0d3d3be609b59d4"
)
APP_FLASH_BASE = 0x00020000
RAM_LOAD_ADDRESS = 0x00028680
RAM_EXEC_ADDRESS = 0x1FFF8000

MOTOR_STATE = 0x1FFFF088
ENCODER_SAMPLE = 0x1FFFF190

# Literal pool words establish which base register owns each field access.
EXPECTED_POINTERS = {
    0x1FFF8404: MOTOR_STATE,     # ADC IRQ r4
    0x1FFF8444: ENCODER_SAMPLE, # ADC IRQ r7
    0x1FFF8B50: MOTOR_STATE,     # position DMA IRQ r5
    0x1FFF8B58: ENCODER_SAMPLE, # position DMA IRQ r4
}

# Exact instructions which distinguish motor-side velocity from gear-scaled
# output velocity.  Addresses and bytes come from the pinned official image.
EXPECTED_INSTRUCTIONS = {
    0x1FFF8872: (
        "position_sensor_dma_irq", "c5ed060a", "vstr.32 s1,[r5,#0x18]"
    ),
    0x1FFF888C: (
        "position_sensor_dma_irq", "84ed040a", "vstr.32 s0,[r4,#0x10]"
    ),
    0x1FFF8354: (
        "adc_foc_control_irq", "97ed040a", "vldr.32 s0,[r7,#0x10]"
    ),
    0x1FFF8386: (
        "adc_foc_control_irq", "84ed070a", "vstr.32 s0,[r4,#0x1c]"
    ),
    0x1FFF839A: (
        "adc_foc_control_irq", "d4ed060a", "vldr.32 s1,[r4,#0x18]"
    ),
    0x1FFF84BE: (
        "adc_foc_control_irq", "94ed010a", "vldr.32 s0,[r4,#0x4]"
    ),
    0x1FFF854E: (
        "adc_foc_control_irq", "d4ed060a", "vldr.32 s1,[r4,#0x18]"
    ),
    0x1FFF8552: (
        "adc_foc_control_irq", "94ed071a", "vldr.32 s2,[r4,#0x1c]"
    ),
}


def ram_load_offset(runtime_address: int) -> int:
    return (
        RAM_LOAD_ADDRESS
        - APP_FLASH_BASE
        + runtime_address
        - RAM_EXEC_ADDRESS
    )


def read_runtime_word(image: bytes, runtime_address: int) -> int:
    offset = ram_load_offset(runtime_address)
    if offset < 0 or offset + 4 > len(image):
        raise ValueError(f"runtime address 0x{runtime_address:08x} is out of range")
    return struct.unpack_from("<I", image, offset)[0]


def assembly_by_address() -> dict[int, tuple[str, str, str]]:
    instructions: dict[int, tuple[str, str, str]] = {}
    with APP_ASM.open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream, delimiter="\t"):
            address = int(row["instruction_address"], 16)
            instructions[address] = (
                row["function"], row["bytes"], row["instruction"]
            )
    return instructions


def main() -> None:
    image = OFFICIAL_APP.read_bytes()
    digest = hashlib.sha256(image).hexdigest()
    if digest != EXPECTED_APP_SHA256:
        raise SystemExit(
            f"{OFFICIAL_APP}: expected SHA-256 {EXPECTED_APP_SHA256}, got {digest}"
        )

    for literal_address, expected_pointer in EXPECTED_POINTERS.items():
        actual_pointer = read_runtime_word(image, literal_address)
        if actual_pointer != expected_pointer:
            raise SystemExit(
                f"literal 0x{literal_address:08x}: expected "
                f"0x{expected_pointer:08x}, got 0x{actual_pointer:08x}"
            )

    instructions = assembly_by_address()
    for address, expected in EXPECTED_INSTRUCTIONS.items():
        actual = instructions.get(address)
        if actual != expected:
            raise SystemExit(
                f"instruction 0x{address:08x}: expected {expected!r}, "
                f"got {actual!r}"
            )

    print(
        "control feedback trace verified: motor state 0x1ffff088, "
        "encoder sample 0x1ffff190, 4 pointers, 8 Thumb instructions"
    )


if __name__ == "__main__":
    main()
