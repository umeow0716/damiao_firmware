#!/usr/bin/env python3
"""Standalone differential test; run with /tmp/dm4310-recovery-venv/bin/python.

Compiles only the sidecar into a temporary host shared library. Exact rational
reference is independent of libc formatting. Factory evidence comes exclusively
from run_factory_formatter's hash-checked original flash at base 0x20000.
Factory mismatches are reported rather than hidden or treated as exact rounding
failures. This is not a target stack/runtime compatibility certification.
"""
import argparse
from collections import Counter
import ctypes
import math
from pathlib import Path
import random
import struct
import subprocess
import tempfile

from run_factory_formatter import format_factory

PRECISIONS = (0, 1, 2, 4, 6, 8, 17)


class Workspace(ctypes.Structure):
    _fields_ = [("limbs", ctypes.c_uint32 * 36), ("digits", ctypes.c_char * 327)]


def exact_reference(value, precision):
    numerator, denominator = value.as_integer_ratio()
    quotient, remainder = divmod(numerator * 10**precision, denominator)
    if 2 * remainder > denominator or (
        2 * remainder == denominator and quotient & 1
    ):
        quotient += 1
    digits = str(quotient).rjust(precision + 1, "0")
    return digits if not precision else digits[:-precision] + "." + digits[-precision:]


def cases(random_count):
    seen = set()

    def add(domain, value):
        bits = struct.pack("<d", value)
        if bits not in seen and math.isfinite(value) and value >= 0:
            seen.add(bits)
            return [(domain, value)]
        return []

    result = []
    for value in (0.0, math.ulp(0.0), 2 * math.ulp(0.0),
                  float.fromhex("0x0.fffffffffffffp-1022"),
                  float.fromhex("0x1p-1022")):
        result += add("zero/subnormal", value)
    for value in (0.5, 1.5, 2.5, 3.5, 1.25, 1.75, 0.125, 0.375,
                  1.2344499826431274, 0.1, 0.01, 9.999999999999998):
        for neighbor in (math.nextafter(value, 0), value,
                         math.nextafter(value, math.inf)):
            result += add("tie/neighbor", neighbor)
    for precision in PRECISIONS:
        for integer in (0, 1, 2, 9, 99, 999):
            value = (integer + 0.5) / 10**precision
            for neighbor in (math.nextafter(value, 0), value,
                             math.nextafter(value, math.inf)):
                result += add("decimal-boundary", neighbor)
    for exponent in (-1074, -1022, -100, -53, -1, 0, 52, 53, 100, 512, 1023):
        value = math.ldexp(1.0, exponent)
        for neighbor in (math.nextafter(value, 0), value,
                         math.nextafter(value, math.inf)):
            result += add("binary-boundary", neighbor)
    for value in (1e16, 1e20, 1e50, 1e100, 1e200, 1e300,
                  float.fromhex("0x1.fffffffffffffp+1023")):
        result += add("large", value)
    rng = random.Random(43105017)
    for _ in range(random_count):
        bits = rng.getrandbits(63)
        value = struct.unpack("<d", struct.pack("<Q", bits))[0]
        result += add("random", value)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--random-count", type=int, default=128)
    parser.add_argument("--show-mismatches", type=int, default=12)
    args = parser.parse_args()
    source = Path(__file__).with_name("factory_decimal_prototype.c")
    with tempfile.TemporaryDirectory(prefix="factory-decimal-") as temporary:
        library = Path(temporary) / "prototype.so"
        subprocess.run(["cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                        "-fPIC", "-shared", "-fstack-usage", str(source),
                        "-o", str(library)], check=True)
        converter = ctypes.CDLL(str(library)).factory_decimal_fixed
        converter.argtypes = [ctypes.c_double, ctypes.c_uint, ctypes.c_void_p,
                              ctypes.c_size_t, ctypes.POINTER(Workspace)]
        converter.restype = ctypes.c_int
        workspace = Workspace()
        output = ctypes.create_string_buffer(328)
        api_checks = 0
        for value, precision in ((-0.0, 0), (-1.0, 0), (math.inf, 0),
                                 (math.nan, 0), (1.0, 18)):
            output.value = b"sentinel"
            assert converter(value, precision, output, 328, ctypes.byref(workspace)) == -1
            assert output.value == b"sentinel"
            api_checks += 1
        assert converter(1.0, 0, None, 328, ctypes.byref(workspace)) == -1
        assert converter(1.0, 0, output, 328, None) == -1
        api_checks += 2
        mismatches, errors, totals = Counter(), Counter(), Counter()
        domain_totals, domain_examples = Counter(), {}
        examples = []
        for domain, value in cases(args.random_count):
            for precision in PRECISIONS:
                expected = exact_reference(value, precision)
                assert converter(value, precision, output, 328, ctypes.byref(workspace)) == 0
                actual = output.value.decode("ascii")
                assert actual == expected, (value.hex(), precision, actual, expected)
                # Exact fit and one byte too small; no partial output on error.
                assert converter(value, precision, output, len(expected) + 1,
                                 ctypes.byref(workspace)) == 0
                output.value = b"sentinel"
                assert converter(value, precision, output, len(expected),
                                 ctypes.byref(workspace)) == -2
                assert output.value == b"sentinel"
                api_checks += 2
                totals[precision] += 1
                domain_totals[domain] += 1
                try:
                    factory = format_factory(f"%.{precision}f", value)
                except RuntimeError as error:
                    errors[(domain, precision)] += 1
                    if len(examples) < args.show_mismatches:
                        examples.append((domain, value.hex(), precision, str(error)))
                    continue
                if actual != factory:
                    mismatches[(domain, precision)] += 1
                    domain_examples.setdefault(domain, (value.hex(), precision, actual, factory))
                    if len(examples) < args.show_mismatches:
                        examples.append((domain, value.hex(), precision, actual, factory))
        total = sum(totals.values())
        print(f"Exact rational: {total}/{total} passed; bounded API checks: {api_checks}")
        print(f"Factory: {total-sum(mismatches.values())-sum(errors.values())}/{total} matched; "
              f"{sum(mismatches.values())} mismatches; {sum(errors.values())} execution errors")
        for precision in PRECISIONS:
            mismatch = sum(n for (_, p), n in mismatches.items() if p == precision)
            error = sum(n for (_, p), n in errors.items() if p == precision)
            print(f"precision {precision}: {totals[precision]-mismatch-error}/{totals[precision]} "
                  f"matched, {mismatch} mismatches, {error} errors")
        print("Mismatch domains:", dict(sorted(mismatches.items())))
        print("Execution-error domains:", dict(sorted(errors.items())))
        for domain, count in sorted(domain_totals.items()):
            mismatch = sum(n for (d, _), n in mismatches.items() if d == domain)
            error = sum(n for (d, _), n in errors.items() if d == domain)
            print(f"domain {domain}: {count-mismatch-error}/{count} matched")
        for domain, example in sorted(domain_examples.items()):
            print("First domain mismatch:", domain, example)
        for example in examples:
            print("Evidence:", example)
        print(f"Workspace: {ctypes.sizeof(Workspace)} bytes; maximum output: 328 bytes")
        for usage in sorted(Path(temporary).glob("*.su")):
            print("Host stack (not target certification):", usage.read_text().strip())


if __name__ == "__main__":
    main()
