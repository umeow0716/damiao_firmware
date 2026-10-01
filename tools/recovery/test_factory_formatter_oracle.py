#!/usr/bin/env python3
"""Factory execution regression cases, not a claim of C equivalence.

Run with the analysis venv containing Unicorn. No production build dependency.
"""
import math
from run_factory_formatter import format_factory


CASES = [
    ("%*2d", (5, 2, 4), "2d"),
    ("%.*2d", (5, 2, 4), "2d"),
    ("%*.*2d", (5, 2, 4), "2d"),
    ("%*0d", (5, 2, 4), "0d"),
    ("%+8.2f", 1.25, "   +1.25"),
    ("%-8.2f", 1.25, "1.25    "),
    ("%#.0f", 1.0, "1."),
    ("%08f", math.inf, "     inf"),
    ("%+08F", math.nan, "    +NAN"),
    ("% 8.2f", 1.25, "    1.25"),
    ("%#8.0f", 1.0, "      1."),
    ("%.17f", float.fromhex("0x1.851eb851eb853p-4"), "0.09500000000000001"),
    ("%.2f", float.fromhex("0x1.fd70a3d70a3d7p-1"), "0.99"),
    ("%.4f", float.fromhex("0x1.f212d77318fc5p-11"), "0.0009"),
    ("%.17f", float.fromhex("0x1.1eca0a9a10f41p-50"), "0.00000000000000099"),
    ("%*.*d", (5, 2, 4), "   04"),
    ("%*.*d", (-5, 2, 4), "04   "),
    ("%0*.*d", (5, -1, 4), "00004"),
    ("%#x", 18, "0x12"),
    ("%#08x", 18, "0x000012"),
    ("%#x", 0, "0"),
    ("%X", 171, "AB"),
    ("%D", 4, "4"),
    ("%S", "abc", "abc"),
    ("%Q", 0, "q"),
    ("%+x", 18, "12"),
    ("% x", 18, "12"),
    ("%05s", "ab", "000ab"),
    ("%-05s", "ab", "ab   "),
    ("%#.0x", 0, ""),
    ("%3d", 4, "  4"),
    ("%03d", -4, "-04"),
    ("%+4d", 4, "  +4"),
    ("%-4d", 4, "4   "),
    ("%04.2d", 4, "  04"),
    ("%.0d", 0, ""),
    ("%03x", 18, "012"),
    ("%5.2s", "abcd", "   ab"),
    ("%-5.2s", "abcd", "ab   "),
    ("%q", 0, "q"),
    ("%.0f", 0.5, "0"),
    ("%.0f", math.nextafter(0.5, 0.0), "0"),
    ("%.0f", math.nextafter(0.5, 1.0), "1"),
    ("%.0f", 1.5, "2"),
    ("%.0f", 2.5, "2"),
    ("%.0f", 3.5, "4"),
    ("%.0f", -2.5, "-2"),
    ("%.1f", 1.25, "1.2"),
    ("%.1f", 1.75, "1.8"),
    ("%.4f", 1.2344499826431274, "1.2344"),
    ("%f", -0.0, "-0.000000"),
    ("%f", math.inf, "inf"),
    ("%f", -math.inf, "-inf"),
    ("%f", math.nan, "nan"),
    ("%8.2f", 1.25, "    1.25"),
    ("%08.2f", -1.25, "-0001.25"),
    ("%.8f", 1.25, "1.25000000"),
]

if __name__ == "__main__":
    for fmt, value, expected in CASES:
        actual = format_factory(fmt, value)
        assert actual == expected, (fmt, value, expected, actual)
    print(f"Factory formatter: {len(CASES)} executed reference cases passed.")
