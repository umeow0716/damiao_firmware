#!/usr/bin/env python3
"""Differential test of C decimal-power chain against factory 0x20f8c."""
import ctypes
import hashlib
from pathlib import Path
import struct
import subprocess
import tempfile
from unicorn import Uc, UcError, UC_ARCH_ARM, UC_MODE_THUMB
from unicorn.arm_const import UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2
from unicorn.arm_const import UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC


class Extended(ctypes.Structure):
    _fields_ = [("word", ctypes.c_uint32 * 3)]


def main():
    root = Path(__file__).resolve().parents[2]
    data = (root / "recovered/binaries/dm4310/dm4310_v3_v5017_app_flash_00020000_memory.bin").read_bytes()
    assert hashlib.sha256(data).hexdigest() == "65aab219268e9159b196d4578d3cd530e6fa90a731a82670a0d3d3be609b59d4"
    machine = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    machine.mem_map(0x20000, 0x20000)
    machine.mem_write(0x20000, data)
    machine.mem_map(0x20000000, 0x10000)
    with tempfile.TemporaryDirectory(prefix="dm4310-decimal-power-") as tmp:
        library = Path(tmp) / "power.so"
        subprocess.run(["cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                        "-shared", "-fPIC",
                        str(Path(__file__).with_name("factory_extended_arithmetic.c")),
                        str(Path(__file__).with_name("factory_decimal_power.c")),
                        "-o", str(library)], check=True)
        function = ctypes.CDLL(str(library)).factory_decimal_power
        function.argtypes = [ctypes.c_int, ctypes.c_int]
        function.restype = Extended
        total = 0
        for exponent in range(-27, 28):
            for rounding in (0, 1, -1):
                machine.reg_write(UC_ARM_REG_R0, 0x20000000)
                machine.reg_write(UC_ARM_REG_R1, exponent & 0xffffffff)
                machine.reg_write(UC_ARM_REG_R2, rounding & 0xffffffff)
                machine.reg_write(UC_ARM_REG_SP, 0x20008000)
                machine.reg_write(UC_ARM_REG_LR, 0x3f101)
                try:
                    machine.emu_start(0x20f8d, 0x3f100, count=50000)
                except UcError as error:
                    raise AssertionError((exponent,rounding,error,machine.reg_read(UC_ARM_REG_PC)))
                expected = struct.unpack("<3I", machine.mem_read(0x20000000,12))
                actual = tuple(function(exponent, rounding).word)
                assert actual == expected, (exponent,rounding,actual,expected)
                total += 1
        print(f"decimal power: {total} factory three-word comparisons passed")


if __name__ == "__main__":
    main()
