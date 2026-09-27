#!/usr/bin/env python3
"""Fail-closed comparison gate for a reconstructed DM4310 app image."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path


def describe(path: Path) -> dict[str, object]:
    data = path.read_bytes()
    if len(data) < 8:
        raise ValueError(f"{path} is too short to contain a vector table")
    stack, reset = struct.unpack_from("<II", data)
    return {
        "path": str(path),
        "size": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "initial_msp": f"0x{stack:08x}",
        "reset_vector": f"0x{reset:08x}",
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--require-exact", action="store_true")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()

    reference_data = args.reference.read_bytes()
    candidate_data = args.candidate.read_bytes()
    exact = reference_data == candidate_data
    common = min(len(reference_data), len(candidate_data))
    differing = sum(left != right for left, right in
                    zip(reference_data[:common], candidate_data[:common]))
    differing += abs(len(reference_data) - len(candidate_data))
    report = {
        "reference": describe(args.reference),
        "candidate": describe(args.candidate),
        "byte_exact": exact,
        "differing_or_missing_bytes": differing,
        "status": "exact" if exact else "not-equivalent",
        "note": (
            "Byte identity is a sufficient execution-identity gate. A failed "
            "byte comparison does not by itself prove semantic inequality, but "
            "this project must not claim equivalence until hardware traces and "
            "all recovered paths also pass."
        ),
    }
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print(f"reference: {report['reference']}")
        print(f"candidate: {report['candidate']}")
        print(f"status: {report['status']}")
        print(f"differing_or_missing_bytes: {differing}")
    if args.require_exact and not exact:
        raise SystemExit(2)


if __name__ == "__main__":
    main()
