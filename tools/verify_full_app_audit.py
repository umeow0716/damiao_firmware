#!/usr/bin/env python3
"""Validate the fail-closed APP audit ledger and report release blockers."""

from __future__ import annotations

import argparse
import csv
import json
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REQUIRED_COLUMNS = {
    "id", "risk", "official_anchors", "oracle_evidence", "source_evidence",
    "offline_status", "target_status", "communication_observable",
    "release_blocker", "notes",
}
RISKS = {"critical", "major", "minor"}
OFFLINE_STATUS = {
    "identity_passed", "pending_independent_review", "static_matched",
    "differential_passed", "known_deviation",
}
TARGET_STATUS = {
    "not_required", "not_tested", "observed_partial", "observed_failure",
    "pending_retest", "passed",
}
TRISTATE = {"yes", "no", "partial"}


def split_paths(value: str) -> list[str]:
    return [item for item in value.split(";") if item]


def build_report(rows: list[dict[str, str]]) -> dict[str, object]:
    offline = Counter(row["offline_status"] for row in rows)
    target = Counter(row["target_status"] for row in rows)
    blockers = [row for row in rows if row["release_blocker"] == "yes"]
    unresolved = [
        row for row in blockers
        if row["offline_status"] in {
            "pending_independent_review", "known_deviation"
        }
        or row["target_status"] not in {"passed", "not_required"}
    ]
    return {
        "schema": 1,
        "area_count": len(rows),
        "offline_status": dict(sorted(offline.items())),
        "target_status": dict(sorted(target.items())),
        "release_blocker_count": len(blockers),
        "unresolved_release_blocker_count": len(unresolved),
        "release_ready": not unresolved,
        "unresolved": [
            {
                "id": row["id"],
                "risk": row["risk"],
                "offline_status": row["offline_status"],
                "target_status": row["target_status"],
                "communication_observable": row["communication_observable"],
                "notes": row["notes"],
            }
            for row in unresolved
        ],
        "areas": rows,
    }


def markdown(report: dict[str, object]) -> str:
    ready = "YES" if report["release_ready"] else "NO"
    lines = [
        "# Full APP audit status",
        "",
        "This is the fail-closed release ledger. Source traceability or a "
        "passing host test does not resolve an item by itself.",
        "",
        f"Release ready: **{ready}**",
        "",
        f"Areas: **{report['area_count']}**; unresolved release blockers: "
        f"**{report['unresolved_release_blocker_count']}/"
        f"{report['release_blocker_count']}**.",
        "",
        "| Area | Risk | Offline | Target | Communication-visible |",
        "|---|---|---|---|---|",
    ]
    for item in report["unresolved"]:  # type: ignore[assignment]
        lines.append(
            f"| `{item['id']}` | {item['risk']} | {item['offline_status']} | "
            f"{item['target_status']} | {item['communication_observable']} |"
        )
    lines.extend(["", "## Open evidence", ""])
    for item in report["unresolved"]:  # type: ignore[assignment]
        lines.append(f"- `{item['id']}`: {item['notes']}")
    lines.append("")
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--ledger", type=Path,
        default=ROOT / "recovered/notes/full_app_audit.tsv",
    )
    parser.add_argument("--require-release-ready", action="store_true")
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--markdown", type=Path)
    args = parser.parse_args()

    with args.ledger.open(newline="", encoding="utf-8") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        if reader.fieldnames is None or set(reader.fieldnames) != REQUIRED_COLUMNS:
            raise SystemExit(f"unexpected audit columns: {reader.fieldnames!r}")
        rows = list(reader)

    errors: list[str] = []
    seen: set[str] = set()
    for line, row in enumerate(rows, start=2):
        prefix = f"{args.ledger}:{line}:"
        if not row["id"] or row["id"] in seen:
            errors.append(f"{prefix} missing or duplicate id {row['id']!r}")
        seen.add(row["id"])
        if row["risk"] not in RISKS:
            errors.append(f"{prefix} invalid risk {row['risk']!r}")
        if row["offline_status"] not in OFFLINE_STATUS:
            errors.append(f"{prefix} invalid offline status")
        if row["target_status"] not in TARGET_STATUS:
            errors.append(f"{prefix} invalid target status")
        if row["communication_observable"] not in TRISTATE:
            errors.append(f"{prefix} invalid communication visibility")
        if row["release_blocker"] not in {"yes", "no"}:
            errors.append(f"{prefix} invalid release blocker flag")
        if not row["official_anchors"].strip() or not row["notes"].strip():
            errors.append(f"{prefix} anchors and notes are required")
        for column in ("oracle_evidence", "source_evidence"):
            for item in split_paths(row[column]):
                if not (ROOT / item).is_file():
                    errors.append(f"{prefix} missing {column}: {item}")
    if errors:
        raise SystemExit("full APP audit ledger is invalid:\n" + "\n".join(errors))

    report = build_report(rows)
    rendered = markdown(report)
    if args.markdown is not None:
        args.markdown.parent.mkdir(parents=True, exist_ok=True)
        args.markdown.write_text(rendered, encoding="utf-8")
    if args.json:
        print(json.dumps(report, indent=2, sort_keys=True))
    else:
        print(
            f"full APP audit: {report['area_count']} areas; "
            f"offline={report['offline_status']}; "
            f"target={report['target_status']}"
        )
        print(
            "release blockers unresolved: "
            f"{report['unresolved_release_blocker_count']}/"
            f"{report['release_blocker_count']}"
        )
        for row in report["unresolved"]:  # type: ignore[assignment]
            print(
                f"BLOCK {row['id']}: offline={row['offline_status']}, "
                f"target={row['target_status']}"
            )
    if args.require_release_ready and not report["release_ready"]:
        raise SystemExit("APP is not release-ready under the full-scan gate")


if __name__ == "__main__":
    main()
