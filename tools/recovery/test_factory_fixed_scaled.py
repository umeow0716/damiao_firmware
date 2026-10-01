#!/usr/bin/env python3
"""Compare composed C fixed scaling with factory final text for <=17 digits."""
import ctypes
from pathlib import Path
import struct
import subprocess
import tempfile
from test_factory_formatter_oracle import CASES
from run_factory_formatter import format_factory


def main():
    directory = Path(__file__).parent
    sources = [directory / name for name in
               ("factory_binary64_extended.c", "factory_extended_arithmetic.c",
                "factory_decimal_power.c", "factory_fixed_scaled.c")]
    with tempfile.TemporaryDirectory(prefix="dm4310-fixed-scale-") as tmp:
        library = Path(tmp) / "fixed.so"
        subprocess.run(["cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                        "-shared", "-fPIC", *map(str,sources), "-o", str(library)], check=True)
        function = ctypes.CDLL(str(library)).factory_fixed_scaled
        function.argtypes = [ctypes.c_uint64, ctypes.c_uint,
                             ctypes.POINTER(ctypes.c_uint64)]
        function.restype = ctypes.c_int
        total = 0
        for fmt, value, unused in CASES:
            if not isinstance(value, float) or 'f' not in fmt.lower() or '*' in fmt:
                continue
            precision = int(fmt.split('.')[1].split('f')[0].split('F')[0]) if '.' in fmt else 6
            if precision > 17 or not (value == value) or value in (float('inf'),-float('inf')):
                continue
            bits = struct.unpack('<Q',struct.pack('<d',abs(value)))[0]
            scaled = ctypes.c_uint64()
            status = function(bits,precision,ctypes.byref(scaled))
            if status != 0 or len(str(scaled.value)) > 17:
                continue
            digits = str(scaled.value).rjust(precision+1,'0')
            actual = digits if precision == 0 else digits[:-precision]+'.'+digits[-precision:]
            if value < 0 or struct.unpack('<Q',struct.pack('<d',value))[0] >> 63:
                actual = '-' + actual
            expected = format_factory(fmt,value)
            # Only formats without field flags are direct scaled-text checks.
            if fmt.startswith('%.'):
                assert actual == expected, (fmt,value,actual,expected,hex(scaled.value))
                total += 1
        print(f"fixed scaling: {total} composed factory-output cases passed")


if __name__ == '__main__':
    main()
