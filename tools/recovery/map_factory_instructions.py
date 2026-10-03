#!/usr/bin/env python3
"""Map canonical DM4310 instruction addresses into another factory image.

Function-entry matching is not sufficient for verifier control points when a
model inserts or removes instructions inside an otherwise matched function.
This tool aligns the normalized Ghidra instruction streams and persists every
unambiguous instruction-address pair for the differential regression layer.
"""

import argparse
import base64
import csv
from difflib import SequenceMatcher
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MODELS = (
    "dm10010",
    "dm3507",
    "dm3507_48v",
    "dm4310_48v",
    "dm4340",
    "dm4340_48v",
    "dm8006",
    "dm8009",
)


def read_rows(path):
    with path.open(newline="", encoding="utf-8") as source:
        return list(csv.DictReader(source, delimiter="\t"))


def decode_lines(value):
    decoded = base64.b64decode(value).decode("utf-8")
    return decoded.split("\n") if decoded else []


def load_functions(model):
    path = ROOT / f"recovered/{model}/tables/factory_function_tokens.tsv"
    functions = {}
    for row in read_rows(path):
        tokens = decode_lines(row["tokens_base64"])
        addresses = [
            int(address, 16)
            for address in decode_lines(row["addresses_base64"])
        ]
        if len(tokens) != len(addresses):
            raise ValueError(
                f"{model} {row['entry']}: {len(tokens)} tokens but "
                f"{len(addresses)} instruction addresses"
            )
        functions[row["entry"]] = (tokens, addresses)
    return functions


def main():
    parser = argparse.ArgumentParser(
        description="Infer DM4310-to-target instruction addresses"
    )
    parser.add_argument("target", choices=MODELS)
    args = parser.parse_args()

    canonical = load_functions("dm4310")
    target = load_functions(args.target)
    matches = read_rows(
        ROOT / f"recovered/{args.target}/tables/dm4310_function_matches.tsv"
    )
    mapped = {}
    for match in matches:
        left_tokens, left_addresses = canonical[match["left_entry"]]
        right_tokens, right_addresses = target[match["right_entry"]]
        sequence = SequenceMatcher(
            None, left_tokens, right_tokens, autojunk=False
        )
        for block in sequence.get_matching_blocks():
            for offset in range(block.size):
                left_index = block.a + offset
                right_index = block.b + offset
                left_address = left_addresses[left_index]
                right_address = right_addresses[right_index]
                previous = mapped.setdefault(left_address, right_address)
                if previous != right_address:
                    raise ValueError(
                        f"ambiguous instruction mapping {left_address:#x}: "
                        f"{previous:#x} or {right_address:#x}"
                    )

    output_path = (
        ROOT / f"recovered/{args.target}/tables/"
        "dm4310_instruction_address_map.tsv"
    )
    with output_path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(
            output,
            fieldnames=("canonical_address", "target_address"),
            delimiter="\t",
            lineterminator="\n",
        )
        writer.writeheader()
        for canonical_address, target_address in sorted(mapped.items()):
            writer.writerow({
                "canonical_address": f"0x{canonical_address:08x}",
                "target_address": f"0x{target_address:08x}",
            })
    print(
        f"mapped {len(mapped)} dm4310-to-{args.target} instruction addresses"
    )


if __name__ == "__main__":
    main()
