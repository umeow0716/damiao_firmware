#!/usr/bin/env python3
"""Verify the official 1 kHz fault-indicator event and GPIO sequence."""

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

STATUS_EVENTS = 0x1FFFF1F0
GPIO_POSRC = 0x40053828
GPIO_POTRH = 0x4005385C

EXPECTED_FLASH_WORDS = {
    0x000255FC: GPIO_POSRC,
    0x00025674: STATUS_EVENTS,
    0x00025678: GPIO_POTRH,
}
EXPECTED_RAM_WORDS = {
    0x1FFF83F8: STATUS_EVENTS,
}
EXPECTED_INSTRUCTIONS = {
    0x1FFF8392: (
        "adc_foc_control_irq", "c9f82c00", "str.w r0,[r9,#0x2c]"
    ),
    0x000254A0: ("main", "f06a", "ldr r0,[r6,#0x2c]"),
    0x000254A8: ("main", "0128", "cmp r0,#0x1"),
    0x000254AE: ("main", "401c", "adds r0,r0,#0x1"),
    0x000254B2: ("main", "fa28", "cmp r0,#0xfa"),
    0x000254BC: ("main", "40f40050", "orr r0,r0,#0x2000"),
    0x000254C0: ("main", "abf80000", "strh.w r0,[r11,#0x0]"),
    0x000254C8: ("main", "41f00401", "orr r1,r1,#0x4"),
    0x000254CC: ("main", "0180", "strh r1,[r0,#0x0]"),
}


def read_word(image: bytes, offset: int) -> int:
    if offset < 0 or offset + 4 > len(image):
        raise ValueError(f"image offset 0x{offset:x} is out of range")
    return struct.unpack_from("<I", image, offset)[0]


def read_flash_word(image: bytes, address: int) -> int:
    return read_word(image, address - APP_FLASH_BASE)


def read_ram_load_word(image: bytes, address: int) -> int:
    offset = (
        RAM_LOAD_ADDRESS - APP_FLASH_BASE + address - RAM_EXEC_ADDRESS
    )
    return read_word(image, offset)


def assembly_by_address() -> dict[int, tuple[str, str, str]]:
    instructions: dict[int, tuple[str, str, str]] = {}
    with APP_ASM.open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream, delimiter="\t"):
            instructions[int(row["instruction_address"], 16)] = (
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

    for address, expected in EXPECTED_FLASH_WORDS.items():
        actual = read_flash_word(image, address)
        if actual != expected:
            raise SystemExit(
                f"Flash word 0x{address:08x}: expected 0x{expected:08x}, "
                f"got 0x{actual:08x}"
            )
    for address, expected in EXPECTED_RAM_WORDS.items():
        actual = read_ram_load_word(image, address)
        if actual != expected:
            raise SystemExit(
                f"RAM-load word 0x{address:08x}: expected 0x{expected:08x}, "
                f"got 0x{actual:08x}"
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
        "status indicator trace verified: 4 pointer literals, "
        "9 Thumb instructions, fault threshold 251 ms"
    )


if __name__ == "__main__":
    main()
