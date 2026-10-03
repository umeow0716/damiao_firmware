#!/usr/bin/env python3
"""Verify the complete recovered DM4310-to-DM4340 factory delta."""

import csv
from pathlib import Path

from compare_factory_startup import run_startup


ROOT = Path(__file__).resolve().parents[2]
DM4310_PATH = ROOT / "reference/V3/APP_DM4310(V3)_V5017_04.decrypted.bin"
DM4340_PATH = ROOT / "reference/V3/APP_DM4340(V3)_V5117_04.decrypted.bin"
INVENTORY_PATH = (
    ROOT / "recovered/dm4310/tables/factory_function_inventory.tsv"
)
FLASH_BASE = 0x00020000
RAW_FIXED_SRAM_START = 0x8680
RAW_FIXED_SRAM_END = 0xAB90
EXPECTED_UNCOMPRESSED_BYTE_DELTA = {
    0x00022B95: (0x30, 0x31),
    0x000255CD: (0x30, 0x31),
    0x00026818: (0x99, 0xFD),
    0x00028654: (0x4C, 0x50),
}
EXPECTED_CONFIG_WORD_DELTA = {
    0x1FFFA5F8: (0x3796FEB5, 0x37A7C5AC),
    0x1FFFA60C: (0x3F59999A, 0x3F6147AE),
    0x1FFFA610: (0x39B4E11E, 0x39BCBE62),
    0x1FFFA614: (0x3B9374BC, 0x3B9EECC0),
    0x1FFFA618: (0x41200000, 0x42200000),
    0x1FFFA620: (0x41F00000, 0x41200000),
    0x1FFFA624: (0x41200000, 0x41E00000),
    0x1FFFA62C: (0x3B73CB3E, 0x3B7BA882),
}


def byte_differences(left, right, base):
    return {
        base + offset: (left_byte, right_byte)
        for offset, (left_byte, right_byte) in enumerate(zip(left, right))
        if left_byte != right_byte
    }


def memory_byte(memories, address):
    for base, data in memories.items():
        if base <= address < base + len(data):
            return data[address - base]
    raise KeyError(hex(address))


def read_word(memories, address):
    return int.from_bytes(
        bytes(memory_byte(memories, address + offset) for offset in range(4)),
        "little",
    )


def verify_function_inventory(dm4310, dm4340):
    with INVENTORY_PATH.open(newline="", encoding="utf-8") as inventory_file:
        rows = list(csv.DictReader(inventory_file, delimiter="\t"))
    assert len(rows) == 170

    contiguous = []
    noncontiguous = []
    changed = []
    for row in rows:
        entry = int(row["entry"], 16)
        end = int(row["end"], 16)
        body_size = int(row["body_bytes"], 16)
        if end - entry + 1 != body_size:
            noncontiguous.append(row["name"])
            continue
        contiguous.append(row["name"])
        if entry >= 0x1FFF8000:
            offset = RAW_FIXED_SRAM_START + entry - 0x1FFF8000
        else:
            offset = entry - FLASH_BASE
        left = dm4310[offset : offset + body_size]
        right = dm4340[offset : offset + body_size]
        if left != right:
            changed.append((entry, row["name"], byte_differences(left, right, entry)))

    assert len(contiguous) == 144
    assert len(noncontiguous) == 26
    assert changed == [
        (0x00026758, "FUN_00026758", {0x00026818: (0x99, 0xFD)})
    ]
    print(
        "PASS: 170-entry global function inventory covered; all 144 "
        "contiguous bodies scanned, with only the 5017->5117 MOVW changed; "
        "the 26 non-contiguous bodies are bounded by the complete raw-region "
        "delta below"
    )


def main():
    dm4310 = DM4310_PATH.read_bytes()
    dm4340 = DM4340_PATH.read_bytes()
    assert len(dm4310) == len(dm4340) == 51_284

    uncompressed_delta = byte_differences(
        dm4310[:RAW_FIXED_SRAM_START],
        dm4340[:RAW_FIXED_SRAM_START],
        FLASH_BASE,
    )
    assert uncompressed_delta == EXPECTED_UNCOMPRESSED_BYTE_DELTA
    assert (
        dm4310[RAW_FIXED_SRAM_START:RAW_FIXED_SRAM_END]
        == dm4340[RAW_FIXED_SRAM_START:RAW_FIXED_SRAM_END]
    )
    assert dm4310[0x6816:0x681A] == bytes.fromhex("41f29931")
    assert dm4340[0x6816:0x681A] == bytes.fromhex("41f2fd31")
    print(
        "PASS: complete uncompressed Flash/fixed-SRAM scan; four known bytes "
        "differ and all 9,488 fixed-SRAM image bytes are identical"
    )
    verify_function_inventory(dm4310, dm4340)

    dm4310_memory = run_startup(DM4310_PATH)
    dm4340_memory = run_startup(DM4340_PATH)
    actual_memory_delta = {}
    for base in dm4310_memory:
        actual_memory_delta.update(
            byte_differences(
                dm4310_memory[base], dm4340_memory[base], base
            )
        )
    expected_byte_addresses = set()
    for address, (left, right) in EXPECTED_CONFIG_WORD_DELTA.items():
        assert read_word(dm4310_memory, address) == left
        assert read_word(dm4340_memory, address) == right
        for offset in range(4):
            if ((left >> (offset * 8)) & 0xFF) != ((right >> (offset * 8)) & 0xFF):
                expected_byte_addresses.add(address + offset)
    assert set(actual_memory_delta) == expected_byte_addresses
    assert len(actual_memory_delta) == 18
    print(
        "PASS: real factory startup leaves exactly eight changed config "
        "words (18 changed bytes); all remaining 128 KiB mapped SRAM matches"
    )


if __name__ == "__main__":
    main()
