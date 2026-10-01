#!/usr/bin/env python3
"""Verify every initialized DM4310 SRAM section has a startup copy source."""

from __future__ import annotations

import argparse
import pathlib
import re
import subprocess
import sys


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--elf", type=pathlib.Path, required=True)
    parser.add_argument("--linker", type=pathlib.Path, required=True)
    parser.add_argument("--startup", type=pathlib.Path, required=True)
    parser.add_argument("--readelf", default="arm-none-eabi-readelf")
    args = parser.parse_args()

    sections = subprocess.run(
        [args.readelf, "-SW", str(args.elf)],
        check=True,
        capture_output=True,
        text=True,
    ).stdout
    linker = args.linker.read_text()
    startup = args.startup.read_text()
    missing: list[str] = []
    checked = 0
    descriptor_initializer_called = (
        "bl dm4310_initialize_shared_literals" in startup
        and "__dm4310_literal_copies_start__" in linker
        and "__dm4310_literal_copies_end__" in linker
    )
    pattern = re.compile(
        r"\]\s+(\.dm4310_\S+)\s+PROGBITS\s+([0-9a-fA-F]+)\s+"
        r"[0-9a-fA-F]+\s+([0-9a-fA-F]+)"
    )
    for match in pattern.finditer(sections):
        section = match.group(1)
        address = int(match.group(2), 16)
        size = int(match.group(3), 16)
        if not 0x1FFF8000 <= address < 0x20000000:
            continue
        checked += 1
        load_symbol = f"__{section[1:]}_load__"
        direct_copy = load_symbol in startup
        descriptor = re.search(
            rf"LONG\({re.escape(load_symbol)}\);\s*"
            rf"LONG\(ADDR\({re.escape(section)}\)\);\s*"
            r"LONG\((0[xX][0-9a-fA-F]+|[0-9]+)\);",
            linker,
        )
        descriptor_copy = False
        if descriptor_initializer_called and descriptor is not None:
            descriptor_copy = int(descriptor.group(1), 0) * 4 == size
        if load_symbol not in linker or not (direct_copy or descriptor_copy):
            missing.append(f"{section} ({load_symbol})")

    if checked == 0:
        print("no initialized DM4310 SRAM sections found", file=sys.stderr)
        return 1
    if missing:
        print("DM4310 SRAM sections missing startup copies:", file=sys.stderr)
        for section in missing:
            print(f"  {section}", file=sys.stderr)
        return 1
    print(f"verified startup copies for {checked} initialized SRAM sections")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
