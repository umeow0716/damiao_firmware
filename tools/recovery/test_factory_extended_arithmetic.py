#!/usr/bin/env python3
"""Differentially execute factory 0x21704/0x2172e and analysis-only C ports."""
import ctypes
import hashlib
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB
from unicorn.arm_const import UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2
from unicorn.arm_const import UC_ARM_REG_R3, UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC


class Extended(ctypes.Structure):
    _fields_ = [("word", ctypes.c_uint32 * 3)]


def triple(words):
    value = Extended()
    value.word[:] = words
    return value


def main():
    root = Path(__file__).resolve().parents[2]
    data = (root / "recovered/binaries/dm4310/dm4310_v3_v5017_app_flash_00020000_memory.bin").read_bytes()
    assert hashlib.sha256(data).hexdigest() == "65aab219268e9159b196d4578d3cd530e6fa90a731a82670a0d3d3be609b59d4"
    machine = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    machine.mem_map(0x20000, 0x20000)
    machine.mem_write(0x20000, data)
    machine.mem_map(0x20000000, 0x10000)
    rng = random.Random(431021704)
    cases = []
    literals = [(0x3fff,0x80000000,0), (0x4002,0xa0000000,0),
                (0x4034,0x8e1bc9bf,0x04000000),
                (0x80003fff,0x80000000,0)]
    for left in literals:
        for right in literals[:3]:
            for rounding in (0, 1, 0xffffffff):
                cases.append((left, right, rounding, 1))
    for _ in range(2500):
        left = (rng.randrange(0x3f00,0x4100), rng.randrange(0x80000000,1<<32), rng.getrandbits(32))
        right = (rng.randrange(0x3f00,0x4100), rng.randrange(0x80000000,1<<32), rng.getrandbits(32))
        cases.append((left, right, rng.choice((0,1,0xffffffff)), rng.choice((0,1))))
    with tempfile.TemporaryDirectory(prefix="dm4310-extended-") as tmp:
        library = Path(tmp) / "extended.so"
        subprocess.run(["cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                        "-shared", "-fPIC", str(Path(__file__).with_name("factory_extended_arithmetic.c")),
                        "-o", str(library)], check=True)
        dll = ctypes.CDLL(str(library))
        functions = [("divide", 0x21705, dll.factory_extended_divide),
                     ("multiply", 0x2172f, dll.factory_extended_multiply)]
        for name, entry, function in functions:
            function.argtypes = [ctypes.POINTER(Extended), ctypes.POINTER(Extended),
                                 ctypes.c_uint32, ctypes.c_uint32]
            function.restype = Extended
            for index, (left, right, rounding, mode) in enumerate(cases):
                machine.mem_write(0x20000000, struct.pack("<3I", *left))
                machine.mem_write(0x20000010, struct.pack("<3I", *right))
                for register, value in ((UC_ARM_REG_R0,0x20000000),(UC_ARM_REG_R1,0x20000010),
                                        (UC_ARM_REG_R2,rounding),(UC_ARM_REG_R3,mode),
                                        (UC_ARM_REG_SP,0x20008000),(UC_ARM_REG_LR,0x3f101)):
                    machine.reg_write(register, value)
                machine.emu_start(entry, 0x3f100, count=10000)
                expected = tuple(machine.reg_read(register) for register in
                                 (UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2))
                actual_value = function(ctypes.byref(triple(left)), ctypes.byref(triple(right)), rounding, mode)
                actual = tuple(actual_value.word)
                assert actual == expected, (name,index,left,right,hex(rounding),mode,actual,expected)
            print(f"{name}: {len(cases)} factory three-word comparisons passed")


if __name__ == "__main__":
    main()
