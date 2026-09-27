#!/usr/bin/env python3
"""Account for every function exported from the two Ghidra analyses.

The audit classifies provenance and disposition.  It deliberately does not
claim target electrical equivalence; that remains a hardware validation gate.
"""

from __future__ import annotations

import argparse
import csv
import json
from collections import Counter
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ALLOWED_CLASSES = {
    "product_logic",
    "vendor_driver",
    "compiler_runtime",
    "library_algorithm",
    "relocation_copy",
    "veneer",
    "analysis_artifact",
    "intentional_deviation",
}
FUNCTION_COLUMNS = {"address", "size", "name", "calling_convention"}
RULE_COLUMNS = {
    "image",
    "start",
    "end",
    "classification",
    "semantic_ids",
    "rationale",
}
FUNCTION_MAP_COLUMNS = {"image", "address", "name", "confidence", "evidence"}
ALLOWED_CONFIDENCE = {"high", "medium", "low"}


@dataclass(frozen=True)
class Function:
    image: str
    address: int
    size: int
    name: str

    @property
    def end(self) -> int:
        return self.address + self.size - 1


@dataclass(frozen=True)
class Rule:
    image: str
    start: int
    end: int
    classification: str
    semantic_ids: tuple[str, ...]
    rationale: str
    line: int

    @property
    def span(self) -> int:
        return self.end - self.start


def parse_hex(value: str, *, field: str) -> int:
    try:
        return int(value, 16)
    except ValueError as error:
        raise ValueError(f"invalid {field} address {value!r}") from error


def load_functions(image: str, path: Path) -> list[Function]:
    with path.open(newline="", encoding="utf-8") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        if reader.fieldnames is None or set(reader.fieldnames) != FUNCTION_COLUMNS:
            raise ValueError(f"{path}: unexpected columns {reader.fieldnames!r}")
        functions = [
            Function(
                image=image,
                address=parse_hex(row["address"], field="function"),
                size=int(row["size"], 10),
                name=row["name"],
            )
            for row in reader
        ]
    if any(function.size <= 0 for function in functions):
        raise ValueError(f"{path}: every function must have positive size")
    addresses = [function.address for function in functions]
    if len(addresses) != len(set(addresses)):
        raise ValueError(f"{path}: duplicate function start address")
    return functions


def load_semantic_ids(path: Path) -> dict[str, str]:
    with path.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream, delimiter="\t"))
    return {row["id"]: row["image"] for row in rows}


def load_function_map(
    path: Path, functions: list[Function]
) -> dict[tuple[str, int], dict[str, str]]:
    inventory = {(item.image, item.address) for item in functions}
    aliases: dict[tuple[str, int], dict[str, str]] = {}
    errors: list[str] = []
    with path.open(newline="", encoding="utf-8") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        if reader.fieldnames is None or set(reader.fieldnames) != FUNCTION_MAP_COLUMNS:
            raise ValueError(f"{path}: unexpected columns {reader.fieldnames!r}")
        for line, row in enumerate(reader, start=2):
            prefix = f"{path}:{line}:"
            try:
                address = parse_hex(row["address"], field="mapped function")
            except ValueError as error:
                errors.append(f"{prefix} {error}")
                continue
            key = (row["image"], address)
            if key not in inventory:
                errors.append(
                    f"{prefix} {row['image']}:0x{address:08x} is absent "
                    "from the Ghidra inventories"
                )
            if key in aliases:
                errors.append(f"{prefix} duplicate function-map entry")
            if not row["name"] or row["name"].startswith("FUN_"):
                errors.append(f"{prefix} proposed name is not descriptive")
            if row["confidence"] not in ALLOWED_CONFIDENCE:
                errors.append(
                    f"{prefix} invalid confidence {row['confidence']!r}"
                )
            if not row["evidence"].strip():
                errors.append(f"{prefix} evidence is empty")
            aliases[key] = row
    if errors:
        raise ValueError("invalid recovered function map:\n" + "\n".join(errors))
    return aliases


def load_rules(path: Path, semantic_ids: dict[str, str]) -> list[Rule]:
    errors: list[str] = []
    rules: list[Rule] = []
    with path.open(newline="", encoding="utf-8") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        if reader.fieldnames is None or set(reader.fieldnames) != RULE_COLUMNS:
            raise ValueError(f"{path}: unexpected columns {reader.fieldnames!r}")
        for line, row in enumerate(reader, start=2):
            prefix = f"{path}:{line}:"
            try:
                start = parse_hex(row["start"], field="rule start")
                end = parse_hex(row["end"], field="rule end")
            except ValueError as error:
                errors.append(f"{prefix} {error}")
                continue
            classification = row["classification"]
            ids = tuple(item for item in row["semantic_ids"].split(";") if item)
            if row["image"] not in {"app", "bootloader"}:
                errors.append(f"{prefix} invalid image {row['image']!r}")
            if end < start:
                errors.append(f"{prefix} end precedes start")
            if classification not in ALLOWED_CLASSES:
                errors.append(f"{prefix} invalid classification {classification!r}")
            if not ids:
                errors.append(f"{prefix} at least one semantic id is required")
            for semantic_id in ids:
                if semantic_id not in semantic_ids:
                    errors.append(f"{prefix} unknown semantic id {semantic_id!r}")
                elif semantic_ids[semantic_id] != row["image"]:
                    errors.append(
                        f"{prefix} semantic id {semantic_id!r} belongs to "
                        f"{semantic_ids[semantic_id]}, not {row['image']}"
                    )
            if not row["rationale"].strip():
                errors.append(f"{prefix} rationale is empty")
            rules.append(
                Rule(
                    image=row["image"],
                    start=start,
                    end=end,
                    classification=classification,
                    semantic_ids=ids,
                    rationale=row["rationale"].strip(),
                    line=line,
                )
            )
    if errors:
        raise ValueError("invalid original-function rules:\n" + "\n".join(errors))
    return rules


def classify(
    functions: list[Function], rules: list[Rule]
) -> tuple[list[tuple[Function, Rule]], list[str]]:
    classified: list[tuple[Function, Rule]] = []
    errors: list[str] = []
    used_rules: set[int] = set()
    for function in functions:
        matches = [
            rule
            for rule in rules
            if rule.image == function.image
            and rule.start <= function.address <= rule.end
        ]
        if not matches:
            errors.append(
                f"{function.image}:0x{function.address:08x} {function.name}: "
                "no classification rule"
            )
            continue
        matches.sort(key=lambda rule: (rule.span, rule.line))
        if len(matches) > 1 and matches[0].span == matches[1].span:
            errors.append(
                f"{function.image}:0x{function.address:08x} {function.name}: "
                f"ambiguous rules on lines {matches[0].line} and {matches[1].line}"
            )
            continue
        chosen = matches[0]
        used_rules.add(chosen.line)
        classified.append((function, chosen))
    for rule in rules:
        if rule.line not in used_rules:
            errors.append(
                f"{rule.image}:0x{rule.start:08x}-0x{rule.end:08x}: "
                f"rule on line {rule.line} matches no function"
            )
    return classified, errors


def overlapping_functions(functions: list[Function]) -> list[Function]:
    overlaps: list[Function] = []
    for image in ("app", "bootloader"):
        highest_end = -1
        for function in sorted(
            (item for item in functions if item.image == image),
            key=lambda item: item.address,
        ):
            if function.address <= highest_end:
                overlaps.append(function)
            highest_end = max(highest_end, function.end)
    return overlaps


def build_report(
    classified: list[tuple[Function, Rule]],
    overlaps: list[Function],
    aliases: dict[tuple[str, int], dict[str, str]],
) -> dict[str, object]:
    by_image = Counter(function.image for function, _ in classified)
    by_class = Counter(rule.classification for _, rule in classified)
    return {
        "schema": 1,
        "total_functions": len(classified),
        "by_image": dict(sorted(by_image.items())),
        "by_classification": dict(sorted(by_class.items())),
        "overlapping_entries": [
            {
                "image": function.image,
                "address": f"0x{function.address:08x}",
                "name": function.name,
            }
            for function in overlaps
        ],
        "functions": [
            {
                "image": function.image,
                "address": f"0x{function.address:08x}",
                "size": function.size,
                "original_name": function.name,
                "proposed_name": aliases.get(
                    (function.image, function.address), {}
                ).get("name", function.name),
                "name_confidence": aliases.get(
                    (function.image, function.address), {}
                ).get("confidence", "original"),
                "classification": rule.classification,
                "semantic_ids": list(rule.semantic_ids),
                "rule_line": rule.line,
            }
            for function, rule in classified
        ],
    }


def markdown(
    classified: list[tuple[Function, Rule]],
    report: dict[str, object],
    aliases: dict[tuple[str, int], dict[str, str]],
) -> str:
    counts = report["by_classification"]
    lines = [
        "# Original function accountability",
        "",
        "This report accounts for every function entry exported by the two "
        "Ghidra analyses. Classification proves inventory coverage, not target "
        "electrical equivalence or instruction identity.",
        "",
        f"Functions: **{report['total_functions']}** "
        f"(`app`: {report['by_image']['app']}, "  # type: ignore[index]
        f"`bootloader`: {report['by_image']['bootloader']})",  # type: ignore[index]
        "",
        "| Classification | Count |",
        "|---|---:|",
    ]
    for classification, count in sorted(counts.items()):  # type: ignore[union-attr]
        lines.append(f"| `{classification}` | {count} |")
    lines.extend(
        [
            "",
            "Overlapping entries are retained because they expose Ghidra's "
            "alternative entry-point guesses. The rules file marks the known "
            "umbrella artifact explicitly.",
            "",
            "| Image | Address | Size | Original name | Proposed name | Classification | Semantic owner |",
            "|---|---:|---:|---|---|---|---|",
        ]
    )
    for function, rule in classified:
        owners = ", ".join(f"`{item}`" for item in rule.semantic_ids)
        proposed_name = aliases.get((function.image, function.address), {}).get(
            "name", function.name
        )
        lines.append(
            f"| {function.image} | `0x{function.address:08x}` | "
            f"{function.size} | `{function.name}` | `{proposed_name}` | "
            f"`{rule.classification}` | {owners} |"
        )
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--rules",
        type=Path,
        default=ROOT / "recovered/notes/original_function_rules.tsv",
    )
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--markdown", type=Path)
    args = parser.parse_args()

    try:
        functions = load_functions(
            "app", ROOT / "recovered/raw/app_functions.tsv"
        ) + load_functions(
            "bootloader", ROOT / "recovered/raw/bootloader_functions.tsv"
        )
        semantic_ids = load_semantic_ids(
            ROOT / "recovered/notes/semantic_coverage.tsv"
        )
        aliases = load_function_map(
            ROOT / "recovered/notes/function_map.tsv", functions
        )
        rules = load_rules(args.rules, semantic_ids)
        classified, errors = classify(functions, rules)
        for function, rule in classified:
            if (
                rule.classification == "product_logic"
                and function.name.startswith(
                    ("FUN_", "thunk_FUN_", "thunk_EXT_FUN_")
                )
            ):
                errors.append(
                    f"{function.image}:0x{function.address:08x} "
                    f"{function.name}: product logic is still anonymous in "
                    "the exported Ghidra call graph; update recovered names "
                    "and rerun analysis/run_ghidra.sh"
                )
        if errors:
            raise ValueError(
                "original-function accountability failed:\n" + "\n".join(errors)
            )
    except (OSError, ValueError) as error:
        raise SystemExit(str(error)) from error

    overlaps = overlapping_functions(functions)
    report = build_report(classified, overlaps, aliases)
    rendered = markdown(classified, report, aliases)
    if args.markdown is not None:
        args.markdown.parent.mkdir(parents=True, exist_ok=True)
        args.markdown.write_text(rendered, encoding="utf-8")
    if args.json:
        print(json.dumps(report, indent=2, sort_keys=True))
    else:
        print(
            f"original function accountability: {report['total_functions']} "
            f"functions; images={report['by_image']}; "
            f"classes={report['by_classification']}; "
            f"overlapping_entries={len(overlaps)}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
