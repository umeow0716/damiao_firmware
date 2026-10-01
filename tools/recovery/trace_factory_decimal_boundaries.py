#!/usr/bin/env python3
"""Capture factory intermediate state for the four decimal-cap counterexamples."""
import json
from run_factory_formatter import format_factory

CASES = [
    ("0x1.851eb851eb853p-4", 17),
    ("0x1.fd70a3d70a3d7p-1", 2),
    ("0x1.f212d77318fc5p-11", 4),
    ("0x1.1eca0a9a10f41p-50", 17),
]

if __name__ == "__main__":
    for hexadecimal, precision in CASES:
        trace = []
        output = format_factory(f"%.{precision}f", float.fromhex(hexadecimal), trace)
        print(json.dumps({"input": hexadecimal, "precision": precision,
                          "output": output, "trace": trace}))
