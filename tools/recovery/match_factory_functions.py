#!/usr/bin/env python3
"""Match factory functions by exact body and normalized instruction hashes."""

import argparse
from collections import defaultdict
import csv
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MODELS = (
    "dm10010",
    "dm3507",
    "dm3507_48v",
    "dm4310",
    "dm4310_48v",
    "dm4340",
    "dm4340_48v",
    "dm8006",
    "dm8009",
)


def read_table(path):
    with path.open(newline="", encoding="utf-8") as source:
        return list(csv.DictReader(source, delimiter="\t"))


def keyed(rows, fields):
    result = defaultdict(list)
    for row in rows:
        result[tuple(row[field] for field in fields)].append(row)
    return result


def unique_matches(left_rows, right_rows, fields, excluded_left, excluded_right):
    left = keyed(
        (row for row in left_rows if row["entry"] not in excluded_left), fields
    )
    right = keyed(
        (row for row in right_rows if row["entry"] not in excluded_right), fields
    )
    matches = []
    for key in sorted(set(left) & set(right)):
        if len(left[key]) == len(right[key]) == 1:
            matches.append((left[key][0], right[key][0]))
    return matches


def same_entry_matches(left_rows, right_rows, fields, excluded_left,
                       excluded_right):
    left = {
        row["entry"]: row for row in left_rows
        if row["entry"] not in excluded_left
    }
    right = {
        row["entry"]: row for row in right_rows
        if row["entry"] not in excluded_right
    }
    return [
        (left[entry], right[entry])
        for entry in sorted(set(left) & set(right), key=lambda item: int(item, 16))
        if all(left[entry][field] == right[entry][field] for field in fields)
    ]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("left", choices=MODELS, default="dm4310",
                        nargs="?")
    parser.add_argument("right", choices=MODELS, default="dm8009",
                        nargs="?")
    parser.add_argument("--output", type=Path)
    parser.add_argument(
        "--complete-by-order",
        action="store_true",
        help=(
            "complete unmatched rows by function order, but only when both "
            "inventories have equal length and every automatic match has the "
            "same ordinal"
        ),
    )
    args = parser.parse_args()

    tables = {}
    for model in (args.left, args.right):
        directory = ROOT / f"recovered/{model}/tables"
        hashes = read_table(directory / "factory_function_hashes.tsv")
        signatures = {
            row["entry"]: row
            for row in read_table(directory / "factory_function_signatures.tsv")
        }
        for row in hashes:
            row.update({
                f"signature_{key}": value
                for key, value in signatures[row["entry"]].items()
                if key not in ("entry", "name")
            })
        tables[model] = hashes

    left_rows = tables[args.left]
    right_rows = tables[args.right]
    matches = []
    used_left = set()
    used_right = set()
    stages = (
        ("exact_same_entry", "same", ("body_bytes", "sha256")),
        ("exact", "unique", ("body_bytes", "sha256")),
        ("normalized_same_entry", "same", (
            "signature_instructions", "signature_normalized_sha256")),
        ("normalized", "unique", (
            "signature_instructions", "signature_normalized_sha256")),
        ("shape_same_entry", "same", (
            "signature_instructions", "signature_shape_sha256")),
        ("shape", "unique", (
            "signature_instructions", "signature_shape_sha256")),
    )
    for method, matching, fields in stages:
        matcher = same_entry_matches if matching == "same" else unique_matches
        stage = matcher(left_rows, right_rows, fields, used_left, used_right)
        for left, right in stage:
            used_left.add(left["entry"])
            used_right.add(right["entry"])
            matches.append((method, left, right))

    if args.complete_by_order:
        left_order = sorted(left_rows, key=lambda row: int(row["entry"], 16))
        right_order = sorted(right_rows, key=lambda row: int(row["entry"], 16))
        if len(left_order) != len(right_order):
            parser.error(
                "--complete-by-order requires equal function counts: "
                f"{len(left_order)} != {len(right_order)}"
            )
        left_indices = {row["entry"]: index
                        for index, row in enumerate(left_order)}
        right_indices = {row["entry"]: index
                         for index, row in enumerate(right_order)}
        displaced = [
            (left["entry"], right["entry"])
            for _method, left, right in matches
            if left_indices[left["entry"]] != right_indices[right["entry"]]
        ]
        if displaced:
            parser.error(
                "automatic matches do not preserve function order; first "
                f"displaced pair is {displaced[0][0]} -> {displaced[0][1]}"
            )
        for left, right in zip(left_order, right_order):
            if (left["entry"] in used_left or
                    right["entry"] in used_right):
                continue
            used_left.add(left["entry"])
            used_right.add(right["entry"])
            matches.append(("ordinal_between_anchors", left, right))

    output_rows = []
    for method, left, right in sorted(
            matches, key=lambda item: int(item[1]["entry"], 16)):
        output_rows.append({
            "left_entry": left["entry"],
            "right_entry": right["entry"],
            "method": method,
            "left_bytes": left["body_bytes"],
            "right_bytes": right["body_bytes"],
            "instructions": left["signature_instructions"],
            "left_name": left["name"],
            "right_name": right["name"],
        })
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with args.output.open("w", newline="", encoding="utf-8") as target:
            writer = csv.DictWriter(
                target, fieldnames=output_rows[0].keys(), delimiter="\t",
                lineterminator="\n",
            )
            writer.writeheader()
            writer.writerows(output_rows)

    counts = {method: 0 for method, _matching, _fields in stages}
    counts["ordinal_between_anchors"] = 0
    for method, _left, _right in matches:
        counts[method] += 1
    print(
        f"{args.left}: {len(left_rows)} functions; "
        f"{args.right}: {len(right_rows)} functions"
    )
    print(
        "unique matches: "
        + ", ".join(
            f"{method}={counts[method]}"
            for method, _matching, _fields in stages
        )
        + (f", ordinal_between_anchors={counts['ordinal_between_anchors']}"
           if args.complete_by_order else "")
        + f", total={len(matches)}"
    )
    print(
        f"unmatched: {args.left}={len(left_rows) - len(used_left)}, "
        f"{args.right}={len(right_rows) - len(used_right)}"
    )


if __name__ == "__main__":
    main()
