#!/usr/bin/env python3
"""Compare source-built ARM sqrt with factory instructions, outside make."""
import hashlib
from pathlib import Path
import random
import struct
import subprocess
import tempfile

from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB
from unicorn import arm_const as arm


def execute(image, entry, bits, fpscr):
    machine = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    machine.mem_map(0x20000, 0x20000)
    machine.mem_write(0x20000, image)
    machine.mem_map(0x1fff0000, 0x10000)
    machine.mem_map(0x20000000, 0x10000)
    machine.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
    machine.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    machine.reg_write(arm.UC_ARM_REG_FPSCR, fpscr)
    for index in range(13):
        machine.reg_write(getattr(arm, f"UC_ARM_REG_R{index}"), 0x12340000 + index)
    for index in range(32):
        machine.reg_write(getattr(arm, f"UC_ARM_REG_S{index}"), 0x3f000000 + index)
    machine.reg_write(arm.UC_ARM_REG_S0, bits)
    machine.reg_write(arm.UC_ARM_REG_SP, 0x200004f8)
    machine.reg_write(arm.UC_ARM_REG_LR, 0x3f101)
    machine.mem_write(0x1ffff490, struct.pack("<I", 0x5a5a1234))
    machine.emu_start(entry | 1, 0x3f100, count=1000)
    assert machine.reg_read(arm.UC_ARM_REG_PC) == 0x3f100
    # PC/LR and saved return addresses relocate with flash code. Compare all
    # other live registers, VFP flags, SP and the runtime errno side effect.
    registers = [getattr(arm, f"UC_ARM_REG_R{i}") for i in range(13)]
    registers += [getattr(arm, f"UC_ARM_REG_S{i}") for i in range(32)]
    registers += [arm.UC_ARM_REG_SP, arm.UC_ARM_REG_FPSCR]
    return (tuple(machine.reg_read(reg) for reg in registers),
            bytes(machine.mem_read(0x1ffff490, 0x60)))


def main():
    root = Path(__file__).resolve().parents[2]
    factory = (root / "recovered/binaries/dm4310/"
               "dm4310_v3_v5017_app_flash_00020000_memory.bin").read_bytes()
    assert hashlib.sha256(factory).hexdigest() == (
        "65aab219268e9159b196d4578d3cd530e6fa90a731a82670a0d3d3be609b59d4")
    elf = root / "build/dm4310.elf"
    symbols = subprocess.check_output(
        ["arm-none-eabi-nm", "--defined-only", str(elf)], text=True)
    entry = next(int(line.split()[0], 16) for line in symbols.splitlines()
                 if line.split()[-1] == "dm4310_checked_sqrtf")
    with tempfile.TemporaryDirectory(prefix="dm4310-sqrt-") as temporary:
        path = Path(temporary) / "text.bin"
        subprocess.run(["arm-none-eabi-objcopy", "--dump-section",
                        f".text={path}", str(elf)], check=True)
        sections = subprocess.check_output(
            ["arm-none-eabi-objdump", "-h", str(elf)], text=True)
        base = next(int(line.split()[3], 16) for line in sections.splitlines()
                    if len(line.split()) > 3 and line.split()[1] == ".text")
        compiled = bytes(base - 0x20000) + path.read_bytes()
    cases = [0, 0x80000000, 1, 0x80000001, 0x007fffff, 0x00800000,
             0x3f800000, 0x40800000, 0xbf800000, 0x7f7fffff, 0xff7fffff,
             0x7f800000, 0xff800000, 0x7fc00000, 0x7f800001, 0xff800001]
    rng = random.Random(0x431023fd0)
    cases += [rng.getrandbits(32) for _ in range(512)]
    count = 0
    for fpscr in (0, 0x400000, 0x800000, 0xc00000, 0x3000000):
        for bits in cases:
            expected = execute(factory, 0x23fd0, bits, fpscr)
            actual = execute(compiled, entry, bits, fpscr)
            assert actual == expected, (hex(bits), hex(fpscr), actual, expected)
            count += 1
    print(f"sqrt: {count} factory/compiled ARM register, FPSCR and errno checks passed")


if __name__ == "__main__":
    main()
