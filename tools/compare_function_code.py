#!/usr/bin/env python3
"""Conservative function-level machine-code identity audit.

This deliberately reports only exact byte matches. A function counts as a
match if its complete recovered instruction byte sequence occurs anywhere in
the candidate image, so link order and placement do not matter. Calls and
literal addresses are not normalized: changing an encoded target is a machine-
code difference and is therefore not called identical here.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import subprocess
from dataclasses import dataclass
from pathlib import Path


APP_BASE = 0x00020000
RAM_CODE_BASE = 0x1FFF8000
RAM_CODE_LOAD = 0x00028680
RAM_CODE_LENGTH = 0x2510


@dataclass(frozen=True)
class Function:
    address: int
    size: int
    name: str


def reference_functions(path: Path) -> list[Function]:
    result: list[Function] = []
    with path.open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream, delimiter="\t"):
            result.append(Function(int(row["address"], 16), int(row["size"]), row["name"]))
    return result


def reference_bytes(image: bytes, function: Function) -> bytes | None:
    address = function.address
    if APP_BASE <= address and address + function.size <= APP_BASE + len(image):
        offset = address - APP_BASE
    elif RAM_CODE_BASE <= address and address + function.size <= RAM_CODE_BASE + RAM_CODE_LENGTH:
        offset = (RAM_CODE_LOAD - APP_BASE) + (address - RAM_CODE_BASE)
    else:
        return None
    return image[offset : offset + function.size]


def candidate_symbols(elf: Path, nm: Path) -> list[Function]:
    output = subprocess.run(
        [str(nm), "-S", "--defined-only", str(elf)],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    ).stdout
    functions: list[Function] = []
    for line in output.splitlines():
        fields = line.split(maxsplit=3)
        if len(fields) != 4 or fields[2] not in ("T", "t", "W"):
            continue
        functions.append(Function(int(fields[0], 16), int(fields[1], 16), fields[3]))
    return functions


def candidate_bytes(image: bytes, function: Function) -> bytes | None:
    if APP_BASE <= function.address and function.address + function.size <= APP_BASE + len(image):
        offset = function.address - APP_BASE
        return image[offset : offset + function.size]
    return None


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--reference", type=Path,
        default=root / "reference" / "official" /
        "APP_DM4310_V3_V5017_04.decrypted.bin",
    )
    parser.add_argument("--functions", type=Path,
                        default=root / "recovered/raw/app_functions.tsv")
    parser.add_argument("--candidate-bin", type=Path,
                        default=root / "build/dm4310_app.bin")
    parser.add_argument("--candidate-elf", type=Path,
                        default=root / "build/dm4310_app.elf")
    parser.add_argument("--nm", type=Path,
                        default=root / "tools/arm-gnu-toolchain/bin/arm-none-eabi-nm")
    parser.add_argument("--minimum-size", type=int, default=16)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()

    reference = args.reference.read_bytes()
    candidate = args.candidate_bin.read_bytes()
    recovered = reference_functions(args.functions)
    symbols = candidate_symbols(args.candidate_elf, args.nm)

    eligible: list[tuple[Function, bytes]] = []
    for function in recovered:
        code = reference_bytes(reference, function)
        if code is not None and function.size >= args.minimum_size:
            eligible.append((function, code))
    matched = [
        {
            "address": f"0x{function.address:08x}",
            "size": function.size,
            "name": function.name,
            "sha256": hashlib.sha256(code).hexdigest(),
            "candidate_offset": candidate.find(code),
        }
        for function, code in eligible
        if candidate.find(code) >= 0
    ]

    candidate_eligible: list[tuple[Function, bytes]] = []
    reference_search = reference + reference[
        RAM_CODE_LOAD - APP_BASE : RAM_CODE_LOAD - APP_BASE + RAM_CODE_LENGTH
    ]
    for function in symbols:
        code = candidate_bytes(candidate, function)
        if code is not None and function.size >= args.minimum_size:
            candidate_eligible.append((function, code))
    candidate_matched = [
        {
            "address": f"0x{function.address:08x}",
            "size": function.size,
            "name": function.name,
            "reference_offset": reference_search.find(code),
        }
        for function, code in candidate_eligible
        if reference_search.find(code) >= 0
    ]

    report = {
        "definition": "complete function bytes occur verbatim at any image position",
        "minimum_function_size": args.minimum_size,
        "reference_functions_audited": len(eligible),
        "reference_function_bytes_audited": sum(len(code) for _, code in eligible),
        "reference_functions_found_verbatim": len(matched),
        "reference_function_bytes_found_verbatim": sum(item["size"] for item in matched),
        "candidate_functions_audited": len(candidate_eligible),
        "candidate_functions_found_verbatim": len(candidate_matched),
        "reference_matches": matched,
        "candidate_matches": candidate_matched,
        "status": "function-code-identical" if len(matched) == len(eligible) else "not-function-code-identical",
    }
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print(f"status: {report['status']}")
        print("reference functions found verbatim: "
              f"{len(matched)}/{len(eligible)}")
        print("reference function bytes found verbatim: "
              f"{report['reference_function_bytes_found_verbatim']}/"
              f"{report['reference_function_bytes_audited']}")
        if matched:
            print("matches: " + ", ".join(item["name"] for item in matched))


if __name__ == "__main__":
    main()
