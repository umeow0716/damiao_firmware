#!/usr/bin/env python3
"""Execute factory formatting instructions in isolation; never linked to APP.

Requires unicorn in a separate analysis venv. Captures the character callback
of 0x20ee6 rather than running UART hardware. Supports one double, integer or
string argument selected by the Python value's type.
"""
import argparse
import hashlib
from pathlib import Path
import struct
from unicorn import Uc, UcError, UC_ARCH_ARM, UC_MODE_THUMB, UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2
from unicorn.arm_const import UC_ARM_REG_R3, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R8, UC_ARM_REG_R9
from unicorn.arm_const import UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC


def format_factory(fmt, value, trace=None):
    root = Path(__file__).resolve().parents[2]
    binary = root / "recovered/binaries/dm4310/dm4310_v3_v5017_app_flash_00020000_memory.bin"
    data = binary.read_bytes()
    if hashlib.sha256(data).hexdigest() != (
        "65aab219268e9159b196d4578d3cd530e6fa90a731a82670a0d3d3be609b59d4"
    ):
        raise RuntimeError("unexpected factory reference")
    machine = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    machine.mem_map(0x20000, 0x20000)
    machine.mem_write(0x20000, data)
    machine.mem_map(0x20000000, 0x10000)
    machine.mem_map(0x1FFF0000, 0x10000)
    # runtime init 0x2035c calls locale selector (0,0); 0x27064 returns
    # PC-relative 0x28670, stored at auxiliary runtime context +0x0c.
    machine.mem_write(0x1FFFF4BC, struct.pack("<I", 0x28670))
    machine.mem_write(0x20000000, fmt.encode() + b"\0")
    if isinstance(value, tuple):
        argument = b"".join(struct.pack("<I", item & 0xFFFFFFFF) for item in value)
    elif isinstance(value, str):
        machine.mem_write(0x20000200, value.encode() + b"\0")
        argument = struct.pack("<I", 0x20000200)
    elif isinstance(value, int):
        argument = struct.pack("<I", value & 0xFFFFFFFF)
    else:
        argument = struct.pack("<d", value)
    machine.mem_write(0x20000100, argument)
    output = bytearray()

    def hook(uc, address, size, unused):
        if trace is not None and address in (0x20A60, 0x20A82, 0x20A98, 0x20A9C, 0x20AB0):
            regs = [uc.reg_read(reg) for reg in
                    (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2,
                     UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R8, UC_ARM_REG_R9)]
            sp = uc.reg_read(UC_ARM_REG_SP)
            words = struct.unpack("<6I", uc.mem_read(sp, 24))
            trace.append({"pc": address, "r0_r1_r2_r5_r6_r8_r9": regs,
                          "stack_words": list(words)})
        if address == 0x3F000:
            output.append(uc.reg_read(UC_ARM_REG_R0) & 255)
            uc.reg_write(UC_ARM_REG_PC, uc.reg_read(UC_ARM_REG_LR))
        elif address == 0x3F100:
            uc.emu_stop()

    machine.hook_add(UC_HOOK_CODE, hook)
    for reg, value_reg in [(UC_ARM_REG_R0, 0x20000000), (UC_ARM_REG_R1, 0),
                           (UC_ARM_REG_R2, 0x20000100), (UC_ARM_REG_R3, 0x3F001),
                           (UC_ARM_REG_SP, 0x20008000), (UC_ARM_REG_LR, 0x3F101)]:
        machine.reg_write(reg, value_reg)
    try:
        machine.emu_start(0x20EE7, 0x3F100, count=2000000)
    except UcError as error:
        pc = machine.reg_read(UC_ARM_REG_PC)
        raise RuntimeError(f"factory execution stopped at {pc:#010x}: {error}") from error
    if machine.reg_read(UC_ARM_REG_PC) != 0x3F100:
        raise RuntimeError("factory formatter did not return within instruction budget")
    return output.decode("ascii")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("format")
    parser.add_argument("value", type=float)
    args = parser.parse_args()
    print(repr(format_factory(args.format, args.value)))
