#!/usr/bin/env python3
"""Fail when a local link in the workspace Markdown documentation is missing."""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys
from urllib.parse import unquote


LINK_RE = re.compile(r"!?\[[^\]]*\]\(([^)]+)\)")
SCHEME_RE = re.compile(r"^[A-Za-z][A-Za-z0-9+.-]*:")


def markdown_files(root: Path) -> list[Path]:
    files = []
    top_level = root / "README.md"
    if top_level.is_file():
        files.append(top_level)
    docs = root / "docs"
    if docs.is_dir():
        files.extend(sorted(docs.rglob("*.md")))
    return files


def local_target(source: Path, raw_target: str) -> Path | None:
    target = raw_target.strip()
    if target.startswith("<") and target.endswith(">"):
        target = target[1:-1]
    target = target.split("#", 1)[0]
    if not target or SCHEME_RE.match(target):
        return None
    return (source.parent / unquote(target)).resolve()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parent.parent,
        help="workspace root (default: parent of tools/)",
    )
    args = parser.parse_args()
    root = args.root.resolve()

    checked = 0
    failures: list[str] = []
    files = markdown_files(root)
    for source in files:
        text = source.read_text(encoding="utf-8")
        for match in LINK_RE.finditer(text):
            destination = local_target(source, match.group(1))
            if destination is None:
                continue
            checked += 1
            if not destination.exists():
                failures.append(
                    f"{source.relative_to(root)} -> {match.group(1)}"
                )

    print(f"checked {checked} local Markdown links across {len(files)} files")
    if failures:
        print("missing local documentation targets:", file=sys.stderr)
        for failure in failures:
            print(f"  {failure}", file=sys.stderr)
        return 1
    print("all local Markdown links resolve")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
