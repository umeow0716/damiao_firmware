#!/usr/bin/env python3
"""Compare actual production formatter C with isolated factory execution.

An explicit analysis-only command; fails when output differs. Host execution
does not verify target stack/SRAM layout or ARM arithmetic runtime internals.
"""
from pathlib import Path
from io import BytesIO
import math
import random
import struct
import subprocess
import tempfile
from elftools.elf.elffile import ELFFile
from test_factory_formatter_oracle import CASES
from run_factory_formatter import format_factory
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_HOOK_CODE
from unicorn import arm_const as arm


def arm_formatter(root):
    """Execute the actual source-built ELF; intercept only UART output."""
    elf = root / "build/dm4310.elf"
    symbols = {}
    for line in subprocess.check_output(
            ["arm-none-eabi-nm", "--defined-only", str(elf)], text=True).splitlines():
        fields = line.split()
        if len(fields) == 3:
            symbols[fields[2]] = int(fields[0], 16)
    machine = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    machine.mem_map(0x20000, 0x20000)
    machine.mem_map(0x1fff0000, 0x10000)
    machine.mem_map(0x20000000, 0x10000)
    image = ELFFile(BytesIO(elf.read_bytes()))
    for section in image.iter_sections():
        address = section["sh_addr"]
        if (section["sh_type"] != "SHT_NOBITS" and
                section["sh_flags"] & 2 and section["sh_size"] != 0 and
                (0x00020000 <= address < 0x00040000 or
                 0x1FFF0000 <= address < 0x20000000 or
                 0x20000000 <= address < 0x20010000)):
            machine.mem_write(address, section.data())
    output = bytearray()

    def capture(uc, address, size, unused):
        pointer = uc.reg_read(arm.UC_ARM_REG_R0)
        length = uc.reg_read(arm.UC_ARM_REG_R1)
        output.extend(uc.mem_read(pointer, length))
        uc.reg_write(arm.UC_ARM_REG_PC, uc.reg_read(arm.UC_ARM_REG_LR))

    callback = symbols["platform_debug_write"]
    machine.hook_add(UC_HOOK_CODE, capture, begin=callback, end=callback)

    def execute(fmt, value):
        output.clear()
        machine.mem_write(0x20000000, bytes(0x1000))
        machine.mem_write(0x20001000, fmt.encode() + b"\0")
        machine.mem_write(0x1ffff4bc, struct.pack("<I", symbols["c_locale"]))
        for register in (arm.UC_ARM_REG_R1, arm.UC_ARM_REG_R2, arm.UC_ARM_REG_R3):
            machine.reg_write(register, 0)
        if isinstance(value, tuple):
            for index, item in enumerate(value, 1):
                machine.reg_write(getattr(arm, f"UC_ARM_REG_R{index}"), item & 0xffffffff)
        elif isinstance(value, str):
            machine.mem_write(0x20001100, value.encode() + b"\0")
            machine.reg_write(arm.UC_ARM_REG_R1, 0x20001100)
        elif isinstance(value, int):
            machine.reg_write(arm.UC_ARM_REG_R1, value & 0xffffffff)
        else:
            low, high = struct.unpack("<II", struct.pack("<d", value))
            machine.reg_write(arm.UC_ARM_REG_R2, low)
            machine.reg_write(arm.UC_ARM_REG_R3, high)
        machine.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
        machine.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
        machine.reg_write(arm.UC_ARM_REG_FPSCR, 0x3000000)
        machine.reg_write(arm.UC_ARM_REG_R0, 0x20001000)
        machine.reg_write(arm.UC_ARM_REG_SP, 0x200004f8)
        machine.reg_write(arm.UC_ARM_REG_LR, 0x3f101)
        machine.emu_start(symbols["debug_console_printf"] | 1, 0x3f100, count=2000000)
        assert machine.reg_read(arm.UC_ARM_REG_PC) == 0x3f100, (fmt, value)
        assert machine.reg_read(arm.UC_ARM_REG_SP) == 0x200004f8, (fmt, value)
        return output.decode("ascii")

    return execute


def main():
    root = Path(__file__).resolve().parents[2]
    arm_mismatches = 0
    target_formatter = arm_formatter(root)
    cases = list(CASES)
    edges = [0.0, -0.0, 1e16, 1e17, 1e100, 1e300, 1e-100, 1e-300,
             float.fromhex("0x1.fffffffffffffp+1023"),
             float.fromhex("0x0.0000000000001p-1022"),
             math.nextafter(1e17, 0.0), math.nextafter(1e17, math.inf)]
    for value in edges:
        for precision in (0, 6, 17, 18, 30, 100, 324):
            cases.append((f"%+035.{precision}f", value, None))
            cases.append((f"%-#35.{precision}f", -value, None))
    rng = random.Random(431020996)
    for _ in range(512):
        bits = rng.getrandbits(64)
        if (bits >> 52) & 0x7ff == 0x7ff:
            continue
        value = struct.unpack("<d", struct.pack("<Q", bits))[0]
        precision = rng.choice((0, 1, 2, 6, 17, 18, 30, 100, 324))
        cases.append((f"%.{precision}f", value, None))
    for fmt, value, unused in cases:
        expected = format_factory(fmt, value)
        target_actual = target_formatter(fmt, value)
        if target_actual != expected:
            arm_mismatches += 1
            print(f"ARM DIFF {fmt} {value!r}: "
                  f"factory={expected!r} ARM={target_actual!r}")
    print(f"ARM ELF: {len(cases) - arm_mismatches}/{len(cases)} match; {arm_mismatches} differ.")
    return 1 if arm_mismatches else 0


if __name__ == "__main__":
    raise SystemExit(main())
