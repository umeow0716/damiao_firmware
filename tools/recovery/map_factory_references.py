#!/usr/bin/env python3
"""Infer relocated factory data addresses from matched instruction streams."""

import base64
from collections import Counter, defaultdict
import csv
from difflib import SequenceMatcher
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def read_rows(path):
    with path.open(newline="", encoding="utf-8") as source:
        return list(csv.DictReader(source, delimiter="\t"))


def decode_lines(value):
    decoded = base64.b64decode(value).decode("utf-8")
    return decoded.split("\n") if decoded else []


def load_functions(model):
    path = ROOT / f"recovered/{model}/tables/factory_function_tokens.tsv"
    result = {}
    for row in read_rows(path):
        result[row["entry"]] = {
            "tokens": decode_lines(row["tokens_base64"]),
            "refs": decode_lines(row["refs_base64"]),
        }
    return result


def parse_targets(value):
    targets = []
    for item in value.split(",") if value else ():
        _kind, address = item.rsplit("=", 1)
        if ":" in address:
            continue
        try:
            targets.append(int(address, 16))
        except ValueError:
            continue
    return targets


def is_runtime_sram(address):
    return 0x1FFF8000 <= address < 0x20010000


def main():
    left = load_functions("dm4310")
    right = load_functions("dm8009")
    matches = read_rows(
        ROOT / "recovered/dm8009/tables/dm4310_function_matches.tsv"
    )
    mappings = defaultdict(Counter)
    evidence = defaultdict(list)
    for match in matches:
        left_entry = match["left_entry"]
        right_entry = match["right_entry"]
        left_function = left[left_entry]
        right_function = right[right_entry]
        sequence = SequenceMatcher(
            None,
            left_function["tokens"],
            right_function["tokens"],
            autojunk=False,
        )
        for block in sequence.get_matching_blocks():
            for offset in range(block.size):
                left_index = block.a + offset
                right_index = block.b + offset
                left_targets = [
                    address for address in parse_targets(
                        left_function["refs"][left_index]
                    ) if is_runtime_sram(address)
                ]
                right_targets = [
                    address for address in parse_targets(
                        right_function["refs"][right_index]
                    ) if is_runtime_sram(address)
                ]
                if len(left_targets) != len(right_targets):
                    continue
                for left_address, right_address in zip(
                        left_targets, right_targets):
                    mappings[left_address][right_address] += 1
                    evidence[(left_address, right_address)].append(
                        f"{left_entry}:{left_index}->{right_entry}:{right_index}"
                    )

    output_path = (
        ROOT / "recovered/dm8009/tables/dm4310_sram_address_map.tsv"
    )
    with output_path.open("w", newline="", encoding="utf-8") as target:
        fieldnames = (
            "dm4310_address", "dm8009_address", "evidence_count",
            "alternatives", "example",
        )
        writer = csv.DictWriter(target, fieldnames=fieldnames, delimiter="\t")
        writer.writeheader()
        for left_address in sorted(mappings):
            choices = mappings[left_address].most_common()
            right_address, count = choices[0]
            writer.writerow({
                "dm4310_address": f"0x{left_address:08x}",
                "dm8009_address": f"0x{right_address:08x}",
                "evidence_count": count,
                "alternatives": ",".join(
                    f"0x{address:08x}:{choice_count}"
                    for address, choice_count in choices[1:]
                ),
                "example": evidence[(left_address, right_address)][0],
            })
    ambiguous = sum(len(choices) > 1 for choices in mappings.values())
    print(
        f"mapped {len(mappings)} DM4310 SRAM reference targets; "
        f"{ambiguous} have lower-ranked alternatives"
    )


if __name__ == "__main__":
    main()
