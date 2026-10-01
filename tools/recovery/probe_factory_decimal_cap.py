#!/usr/bin/env python3
"""Test, not assume, the 17-significant-digit factory conversion hypothesis.

Decimal is analysis only; this is not a proposed firmware implementation.
Print every counterexample and fail while the hypothesis is incomplete.
"""
import decimal
from test_factory_decimal_prototype import cases, PRECISIONS
from run_factory_formatter import format_factory


def main():
    total = mismatches = 0
    with decimal.localcontext() as context:
        context.prec = 1200
        for domain, value in cases(0):
            exact = decimal.Decimal.from_float(value)
            capped = exact.quantize(decimal.Decimal(1).scaleb(exact.adjusted() - 16)) if value else exact
            for precision in PRECISIONS:
                predicted = format(capped.quantize(decimal.Decimal(1).scaleb(-precision)), "f")
                factory = format_factory(f"%.{precision}f", value)
                total += 1
                if predicted != factory:
                    mismatches += 1
                    print(domain, value.hex(), precision,
                          f"hypothesis={predicted!r} factory={factory!r}")
    print(f"17-digit hypothesis: {total - mismatches}/{total} match; {mismatches} counterexamples")
    return bool(mismatches)


if __name__ == "__main__":
    raise SystemExit(main())
