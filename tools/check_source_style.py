#!/usr/bin/env python3
"""Check small readability rules for project-owned C source."""

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOTS = ("app", "board", "bootloader", "common", "startup", "tests")
SOURCE_SUFFIXES = {".c", ".h"}

# A discarded call is clearer in this workspace as `function();`. Casts of
# unused parameters such as `(void)parameter;` remain allowed.
VOID_CALL = re.compile(
    r"\(\s*void\s*\)\s*(?!__attribute__\b)([A-Za-z_][A-Za-z0-9_]*)\s*\("
)
DECOMPILER_NAME = re.compile(
    r"\b(?:FUN_|DAT_|LAB_|local_[0-9a-f]+|[piuf]Var[0-9]+|undefined[1248]?)"
    r"[A-Za-z0-9_]*\b"
)
CONFIDENCE_MACRO = re.compile(r"\bRECOVERY_(?:HIGH|MEDIUM|LOW)\b")


def source_files() -> list[Path]:
    files: list[Path] = []
    for root_name in SOURCE_ROOTS:
        root = ROOT / root_name
        if not root.exists():
            continue
        files.extend(
            path for path in root.rglob("*") if path.suffix in SOURCE_SUFFIXES
        )
    return sorted(files)


def main() -> None:
    failures: list[str] = []
    for path in source_files():
        for line_number, line in enumerate(
            path.read_text(encoding="utf-8").splitlines(), start=1
        ):
            match = VOID_CALL.search(line)
            if match is not None:
                relative = path.relative_to(ROOT)
                failures.append(
                    f"{relative}:{line_number}: call {match.group(1)}() "
                    "directly instead of casting its result to void"
                )
            match = DECOMPILER_NAME.search(line)
            if match is not None:
                relative = path.relative_to(ROOT)
                failures.append(
                    f"{relative}:{line_number}: replace decompiler name "
                    f"{match.group(0)!r} with a semantic C name"
                )
            match = CONFIDENCE_MACRO.search(line)
            if match is not None:
                relative = path.relative_to(ROOT)
                failures.append(
                    f"{relative}:{line_number}: keep recovery confidence in "
                    "the evidence ledger instead of product C"
                )

    if failures:
        raise SystemExit("\n".join(failures))

    print(f"source style verified: {len(source_files())} C headers/sources")


if __name__ == "__main__":
    main()
