#!/usr/bin/env python3
"""Validate and summarize the original-to-source semantic coverage matrix."""

from __future__ import annotations

import argparse
import csv
import json
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ALLOWED_STATUS = {
    "host_verified",
    "source_implemented",
    "partial",
    "missing",
    "toolchain_replaced",
    "intentional_deviation",
}
ALLOWED_GATES = {"host", "logic", "power", "recovery"}
REQUIRED_COLUMNS = {
    "id",
    "layer",
    "image",
    "original_anchors",
    "status",
    "source_evidence",
    "test_evidence",
    "target_gate",
    "notes",
}


def load_inventory(path: Path) -> set[int]:
    with path.open(newline="", encoding="utf-8") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        return {int(row["address"], 16) for row in reader}


def split_list(value: str) -> list[str]:
    return [] if value == "-" else [item for item in value.split(";") if item]


def load_rows(path: Path) -> tuple[list[dict[str, str]], list[str]]:
    errors: list[str] = []
    with path.open(newline="", encoding="utf-8") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        if reader.fieldnames is None or set(reader.fieldnames) != REQUIRED_COLUMNS:
            return [], [f"unexpected columns: {reader.fieldnames!r}"]
        rows = list(reader)

    inventories = {
        "app": load_inventory(ROOT / "recovered/raw/app_functions.tsv"),
        "bootloader": load_inventory(ROOT / "recovered/raw/bootloader_functions.tsv"),
    }
    seen_ids: set[str] = set()
    for line, row in enumerate(rows, start=2):
        prefix = f"{path}:{line}:"
        if not row["id"] or row["id"] in seen_ids:
            errors.append(f"{prefix} missing or duplicate id {row['id']!r}")
        seen_ids.add(row["id"])
        if row["image"] not in inventories:
            errors.append(f"{prefix} invalid image {row['image']!r}")
            continue
        if row["status"] not in ALLOWED_STATUS:
            errors.append(f"{prefix} invalid status {row['status']!r}")
        if row["target_gate"] not in ALLOWED_GATES:
            errors.append(f"{prefix} invalid target gate {row['target_gate']!r}")
        for anchor in split_list(row["original_anchors"]):
            try:
                address = int(anchor, 0)
            except ValueError:
                errors.append(f"{prefix} invalid original anchor {anchor!r}")
                continue
            if address not in inventories[row["image"]]:
                errors.append(
                    f"{prefix} anchor {anchor} is absent from {row['image']} inventory"
                )
        for evidence_column in ("source_evidence", "test_evidence"):
            for evidence in split_list(row[evidence_column]):
                candidate = ROOT / evidence
                if not candidate.is_file():
                    errors.append(
                        f"{prefix} {evidence_column} path does not exist: {evidence}"
                    )
        if not row["notes"].strip():
            errors.append(f"{prefix} notes must explain the coverage/deviation")
    return rows, errors


def report(rows: list[dict[str, str]]) -> dict[str, object]:
    status = Counter(row["status"] for row in rows)
    gates = Counter(row["target_gate"] for row in rows)
    source_gaps = [
        {
            "id": row["id"],
            "status": row["status"],
            "target_gate": row["target_gate"],
            "notes": row["notes"],
        }
        for row in rows
        if row["status"] in {"partial", "missing"}
    ]
    target_pending = [
        {
            "id": row["id"],
            "target_gate": row["target_gate"],
            "status": row["status"],
            "notes": row["notes"],
        }
        for row in rows
        if row["target_gate"] != "host"
    ]
    return {
        "schema": 1,
        "total_subsystems": len(rows),
        "status": dict(sorted(status.items())),
        "target_gates": dict(sorted(gates.items())),
        # Keep known_gaps for existing report consumers, but state precisely
        # that it covers source traceability only.  A row with a source and a
        # host test is not thereby target-verified.
        "known_gaps": source_gaps,
        "source_traceability_gaps": source_gaps,
        "source_traceability_complete": not source_gaps,
        "target_pending_count": len(target_pending),
        "target_pending": target_pending,
        "hardware_pending": [item["id"] for item in target_pending],
    }


def markdown(rows: list[dict[str, str]], summary: dict[str, object]) -> str:
    lines = [
        "# Semantic coverage report",
        "",
        "This is a traceability report, not proof of target electrical equivalence.",
        "",
        f"Subsystems: **{summary['total_subsystems']}**",
        "",
        "| Subsystem | Layer | Image | Status | Final gate |",
        "|---|---|---|---|---|",
    ]
    for row in rows:
        lines.append(
            f"| `{row['id']}` | {row['layer']} | {row['image']} | "
            f"{row['status']} | {row['target_gate']} |"
        )
    lines.extend(["", "## Source traceability gaps", ""])
    gaps = summary["known_gaps"]
    if not gaps:
        lines.append("No row is marked partial or missing.")
    else:
        for gap in gaps:  # type: ignore[assignment]
            lines.append(
                f"- `{gap['id']}` ({gap['status']}): {gap['notes']}"
            )
    lines.extend([
        "",
        "## Target verification still pending",
        "",
        (f"**{summary['target_pending_count']} subsystem(s)** still require "
         "logic, power, or recovery observations on the device.  These are "
         "not counted as PASS by this traceability report."),
        "",
        "| Subsystem | Gate | Current source status |",
        "|---|---|---|",
    ])
    for item in summary["target_pending"]:  # type: ignore[assignment]
        lines.append(
            f"| `{item['id']}` | {item['target_gate']} | {item['status']} |"
        )
    lines.append("")
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--matrix",
        type=Path,
        default=ROOT / "recovered/notes/semantic_coverage.tsv",
    )
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--markdown", type=Path)
    parser.add_argument("--fail-on-missing", action="store_true")
    args = parser.parse_args()

    rows, errors = load_rows(args.matrix)
    if errors:
        raise SystemExit("semantic coverage matrix is invalid:\n" + "\n".join(errors))
    summary = report(rows)
    rendered = markdown(rows, summary)
    if args.markdown is not None:
        args.markdown.parent.mkdir(parents=True, exist_ok=True)
        args.markdown.write_text(rendered, encoding="utf-8")
    if args.json:
        print(json.dumps(summary, indent=2, sort_keys=True))
    else:
        print(
            f"semantic traceability: {summary['total_subsystems']} subsystems; "
            f"status={summary['status']}; target_gates={summary['target_gates']}"
        )
        print(
            f"traceability only: {summary['target_pending_count']} subsystems "
            "still require on-device acceptance"
        )
        for gap in summary["known_gaps"]:  # type: ignore[index]
            print(f"known gap: {gap['id']}: {gap['notes']}")
    if args.fail_on_missing and summary["status"].get("missing", 0):  # type: ignore[union-attr]
        raise SystemExit("semantic coverage contains missing subsystems")


if __name__ == "__main__":
    main()
