#!/usr/bin/env python3
"""Check local links in the maintained Markdown documentation."""

from __future__ import annotations

import re
import sys
from pathlib import Path
from urllib.parse import unquote


ROOT = Path(__file__).resolve().parents[1]
DOCUMENTS = (
    ROOT / "README.md",
    ROOT / "docs" / "README.md",
    ROOT / "docs" / "SOURCE_MAP.md",
    ROOT / "docs" / "DEVELOPMENT.md",
    ROOT / "docs" / "PORTING_CHECKLIST.md",
    ROOT / "docs" / "UPDATE_FORMAT.md",
)
LINK_PATTERN = re.compile(r"(?<!!)\[[^\]]+\]\((<[^>]+>|[^)\s]+)(?:\s+['\"][^)]*['\"])?\)")


def maintained_documents() -> list[Path]:
    return [*DOCUMENTS, *sorted((ROOT / "docs" / "book").glob("*.md"))]


def link_path(document: Path, raw_target: str) -> Path | None:
    target = raw_target[1:-1] if raw_target.startswith("<") else raw_target
    if target.startswith(("http://", "https://", "mailto:", "data:")):
        return None
    target = unquote(target.split("#", 1)[0])
    if not target:
        return None
    return (document.parent / target).resolve()


def main() -> int:
    errors: list[str] = []
    documents = maintained_documents()
    for document in documents:
        if not document.is_file():
            errors.append(f"missing maintained document: {document.relative_to(ROOT)}")
            continue
        text = document.read_text(encoding="utf-8")
        for match in LINK_PATTERN.finditer(text):
            target = link_path(document, match.group(1))
            if target is not None and not target.exists():
                line = text.count("\n", 0, match.start()) + 1
                errors.append(
                    f"{document.relative_to(ROOT)}:{line}: missing link target "
                    f"{match.group(1)}"
                )

    if errors:
        for error in errors:
            print(f"FAIL: {error}")
        return 1

    print(f"Markdown links verified: {len(documents)} maintained documents")
    return 0


if __name__ == "__main__":
    sys.exit(main())
