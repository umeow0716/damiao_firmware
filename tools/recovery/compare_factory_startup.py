#!/usr/bin/env python3
"""Compare two factory images after their real scatter-load startup code."""

import argparse
from pathlib import Path
import struct

from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB
import unicorn.arm_const as arm


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_LEFT = ROOT / "reference/APP_DM4310_V3_V5017_04.decrypted.bin"
DEFAULT_RIGHT = ROOT / "reference/APP_DM4340_V3_V5117_04_decrypted.bin"
FLASH_BASE = 0x00020000
FLASH_SIZE = 0x00020000
SRAM_A_BASE = 0x1FFF0000
SRAM_A_SIZE = 0x00010000
SRAM_B_BASE = 0x20000000
SRAM_B_SIZE = 0x00010000
SCATTER_ENTRY = 0x00020259
SCATTER_RETURN = 0x00020368
MARKER = 0xA5


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("left", nargs="?", type=Path, default=DEFAULT_LEFT)
    parser.add_argument("right", nargs="?", type=Path, default=DEFAULT_RIGHT)
    return parser.parse_args()


def run_startup(path):
    image = path.read_bytes()
    if len(image) > FLASH_SIZE:
        raise ValueError(f"{path}: image exceeds mapped APP flash")

    machine = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    machine.mem_map(FLASH_BASE, FLASH_SIZE)
    machine.mem_map(SRAM_A_BASE, SRAM_A_SIZE)
    machine.mem_map(SRAM_B_BASE, SRAM_B_SIZE)
    machine.mem_write(FLASH_BASE, image)
    machine.mem_write(SRAM_A_BASE, bytes([MARKER]) * SRAM_A_SIZE)
    machine.mem_write(SRAM_B_BASE, bytes([MARKER]) * SRAM_B_SIZE)
    machine.reg_write(arm.UC_ARM_REG_C1_C0_2, 0x00F00000)
    machine.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    machine.reg_write(arm.UC_ARM_REG_SP, 0x2000F000)
    machine.reg_write(arm.UC_ARM_REG_LR, 0x00030001)
    machine.emu_start(SCATTER_ENTRY, SCATTER_RETURN, count=2_000_000)
    pc = machine.reg_read(arm.UC_ARM_REG_PC)
    if pc != SCATTER_RETURN:
        raise RuntimeError(f"{path}: startup stopped at {pc:#010x}")

    return {
        SRAM_A_BASE: bytes(machine.mem_read(SRAM_A_BASE, SRAM_A_SIZE)),
        SRAM_B_BASE: bytes(machine.mem_read(SRAM_B_BASE, SRAM_B_SIZE)),
    }


def difference_runs(left, right, base):
    runs = []
    start = None
    for offset, (left_byte, right_byte) in enumerate(zip(left, right)):
        if left_byte != right_byte and start is None:
            start = offset
        if left_byte == right_byte and start is not None:
            runs.append((base + start, left[start:offset], right[start:offset]))
            start = None
    if start is not None:
        runs.append((base + start, left[start:], right[start:]))
    return runs


def word_at(data, base, address):
    aligned = address & ~3
    offset = aligned - base
    if not 0 <= offset <= len(data) - 4:
        return None
    return aligned, struct.unpack_from("<I", data, offset)[0]


def summarize_run(run, left_image, right_image, base):
    address, left, right = run
    left_word = word_at(left_image, base, address)
    right_word = word_at(right_image, base, address)
    preview_size = min(len(left), 16)
    print(
        f"{address:#010x}..{address + len(left) - 1:#010x} "
        f"({len(left):4d} bytes) "
        f"{left[:preview_size].hex()} -> {right[:preview_size].hex()}"
    )
    if left_word is not None and right_word is not None:
        print(
            f"  aligned word {left_word[0]:#010x}: "
            f"{left_word[1]:#010x} -> {right_word[1]:#010x}"
        )


def main():
    args = parse_args()
    left_memories = run_startup(args.left)
    right_memories = run_startup(args.right)
    total_bytes = 0
    total_runs = 0

    print(f"left:  {args.left} ({args.left.stat().st_size} bytes)")
    print(f"right: {args.right} ({args.right.stat().st_size} bytes)")
    for base in (SRAM_A_BASE, SRAM_B_BASE):
        left = left_memories[base]
        right = right_memories[base]
        runs = difference_runs(left, right, base)
        changed = sum(len(run[1]) for run in runs)
        total_bytes += changed
        total_runs += len(runs)
        print(
            f"SRAM {base:#010x}..{base + len(left) - 1:#010x}: "
            f"{changed} differing bytes in {len(runs)} runs"
        )
        for run in runs:
            summarize_run(run, left, right, base)

    print(f"TOTAL: {total_bytes} differing bytes in {total_runs} runs")


if __name__ == "__main__":
    main()
