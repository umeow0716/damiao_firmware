#!/usr/bin/env python3
"""Differential test of C helper against factory 0x2120c instructions."""
import ctypes
import hashlib
from pathlib import Path
import random
import subprocess
import tempfile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB
from unicorn.arm_const import UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2
from unicorn.arm_const import UC_ARM_REG_LR, UC_ARM_REG_PC


class Extended(ctypes.Structure):
    _fields_ = [("exponent", ctypes.c_uint32), ("high", ctypes.c_uint32),
                ("low", ctypes.c_uint32)]


def main():
    root = Path(__file__).resolve().parents[2]
    data = (root / "recovered/binaries/dm4310/dm4310_v3_v5017_app_flash_00020000_memory.bin").read_bytes()
    assert hashlib.sha256(data).hexdigest() == "65aab219268e9159b196d4578d3cd530e6fa90a731a82670a0d3d3be609b59d4"
    machine = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    machine.mem_map(0x20000, 0x20000)
    machine.mem_write(0x20000, data)
    values = [0, 1 << 63, 1, (1 << 52)-1, 1 << 52,
              0x3FF0000000000000, 0x7FF0000000000000,
              0x7FF0000000000001, 0x7FF8000000000000,
              0xFFFFFFFFFFFFFFFF]
    # Every subnormal normalization distance, both signs and mantissa ends.
    for bit in range(52):
        values.extend([1 << bit, (1 << bit) | (1 << 63), (1 << (bit+1))-1])
    rng = random.Random(43102120)
    values.extend(rng.getrandbits(64) for _ in range(4096))
    with tempfile.TemporaryDirectory(prefix="dm4310-binary64-helper-") as tmp:
        library = Path(tmp) / "helper.so"
        subprocess.run(["cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                        "-shared", "-fPIC", str(Path(__file__).with_name("factory_binary64_extended.c")),
                        "-o", str(library)], check=True)
        converter = ctypes.CDLL(str(library)).factory_binary64_extended
        converter.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
        converter.restype = Extended
        for bits in values:
            high, low = bits >> 32, bits & 0xFFFFFFFF
            machine.reg_write(UC_ARM_REG_R0, high)
            machine.reg_write(UC_ARM_REG_R1, low)
            machine.reg_write(UC_ARM_REG_LR, 0x3F101)
            machine.emu_start(0x2120D, 0x3F100, count=1000)
            assert machine.reg_read(UC_ARM_REG_PC) == 0x3F100
            expected = tuple(machine.reg_read(reg) for reg in
                             (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2))
            actual = converter(high, low)
            result = (actual.exponent, actual.high, actual.low)
            assert result == expected, (hex(bits), result, expected)
    print(f"binary64 helper: {len(values)} factory three-word comparisons passed.")


if __name__ == "__main__":
    main()
