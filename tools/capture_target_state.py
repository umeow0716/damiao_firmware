#!/usr/bin/env python3
"""Capture the known DM4310 register/RAM ranges through a GDB server.

The tool never programs, erases or resets the target.  A live capture still
requires halting the core, so execution is only attempted after the operator
confirms that the bridge is isolated.  Historical evidence in bins/ is never
used as an output directory.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from analysis.decode_dumps import CAPTURES, Capture  # noqa: E402


DEFAULT_GDB = ROOT / "tools" / "arm-gnu-toolchain" / "bin" / "arm-none-eabi-gdb"


def gdb_quote(path: Path) -> str:
    """Return a GDB-compatible quoted filename."""
    return json.dumps(str(path.resolve()))


def build_gdb_script(
    target: str,
    capture_dir: Path,
    captures: Iterable[Capture] = CAPTURES,
    resume: bool = False,
) -> str:
    lines = [
        "set pagination off",
        "set confirm off",
        "set verbose off",
        "set mem inaccessible-by-default off",
        f"target extended-remote {target}",
        "monitor halt",
        "info registers",
        "x/1wx 0xe000ed08",
    ]
    for capture in captures:
        output = capture_dir / capture.filename
        end = capture.base + capture.length
        lines.append(
            f"dump binary memory {gdb_quote(output)} "
            f"0x{capture.base:08x} 0x{end:08x}"
        )
    if resume:
        lines.append("monitor resume")
    lines.extend(["disconnect", "quit", ""])
    return "\n".join(lines)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(65536), b""):
            digest.update(block)
    return digest.hexdigest()


def validate_captures(capture_dir: Path) -> list[dict[str, object]]:
    files: list[dict[str, object]] = []
    for capture in CAPTURES:
        path = capture_dir / capture.filename
        if not path.is_file():
            raise ValueError(f"GDB did not create {path}")
        actual_size = path.stat().st_size
        if actual_size != capture.length:
            raise ValueError(
                f"{path}: expected {capture.length:#x} bytes, got {actual_size:#x}"
            )
        files.append(
            {
                "file": capture.filename,
                "base": f"0x{capture.base:08x}",
                "length": capture.length,
                "sha256": sha256(path),
            }
        )
    return files


def ensure_new_output_dir(output_dir: Path) -> None:
    resolved = output_dir.resolve()
    if resolved == (ROOT / "bins").resolve():
        raise ValueError("refusing to overwrite immutable historical bins/")
    if output_dir.exists() and any(output_dir.iterdir()):
        raise ValueError(f"output directory is not empty: {output_dir}")
    output_dir.mkdir(parents=True, exist_ok=True)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--target",
        default="tcp:127.0.0.1:3333",
        help="GDB remote target (default: tcp:127.0.0.1:3333)",
    )
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--gdb", type=Path, default=DEFAULT_GDB)
    parser.add_argument(
        "--resume",
        action="store_true",
        help="resume the target after a successful capture; default leaves it halted",
    )
    parser.add_argument(
        "--yes-isolated",
        action="store_true",
        help="confirm motor power/bridge is isolated and permit a live connection",
    )
    parser.add_argument(
        "--emit-script",
        action="store_true",
        help="write capture.gdb only; do not connect to the target",
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        ensure_new_output_dir(arguments.output_dir)
        capture_dir = arguments.output_dir / "bins"
        capture_dir.mkdir()
        script = build_gdb_script(
            arguments.target, capture_dir, resume=arguments.resume
        )
        script_path = arguments.output_dir / "capture.gdb"
        script_path.write_text(script, encoding="utf-8")

        if arguments.emit_script:
            print(f"GDB script written to {script_path}; target was not contacted")
            return 0
        if not arguments.yes_isolated:
            raise ValueError(
                "live capture requires --yes-isolated after motor power and bridge isolation"
            )
        if not arguments.gdb.is_file():
            raise ValueError(f"GDB executable not found: {arguments.gdb}")

        result = subprocess.run(
            [str(arguments.gdb), "--batch", "--nx", "--quiet", "-x", str(script_path)],
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        (arguments.output_dir / "gdb.log").write_text(
            result.stdout, encoding="utf-8"
        )
        if result.returncode != 0:
            raise ValueError(
                f"GDB capture failed with status {result.returncode}; target may remain halted; "
                f"see {arguments.output_dir / 'gdb.log'}"
            )

        files = validate_captures(capture_dir)
        decoded = arguments.output_dir / "decoded.md"
        subprocess.run(
            [
                sys.executable,
                str(ROOT / "analysis" / "decode_dumps.py"),
                "--bins",
                str(capture_dir),
                "--output",
                str(decoded),
            ],
            cwd=ROOT,
            check=True,
        )
        manifest = {
            "captured_utc": datetime.now(timezone.utc).isoformat(),
            "gdb_target": arguments.target,
            "target_resumed": arguments.resume,
            "safety_confirmation": "motor power and bridge isolated",
            "files": files,
        }
        (arguments.output_dir / "manifest.json").write_text(
            json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
        )
        print(f"captured {len(files)} ranges into {arguments.output_dir}")
        print(f"decoded report: {decoded}")
        if not arguments.resume:
            print("target intentionally left halted")
        return 0
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
