#!/usr/bin/env python3
"""Validate generated firmware artifacts without enforcing model equivalence.

This is intentionally a lightweight build-artifact sanity check:
- each expected output exists and is non-empty
- plain images look like HC32/Cortex-M app images at offset 0
- encrypted images are length-preserving and differ from their plain input

It does not compare DM4310 and DM8009 against each other.  During development the
models may legitimately diverge.
"""

from __future__ import annotations

import argparse
import hashlib
import struct
import sys
from pathlib import Path

MODELS = ("dm4310", "dm8009")


def sha256_short(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()[:16]


def read_file(path: Path) -> bytes:
    try:
        data = path.read_bytes()
    except FileNotFoundError:
        raise SystemExit(f"missing output: {path}")
    if not data:
        raise SystemExit(f"empty output: {path}")
    return data


def check_plain_image(model: str, path: Path, data: bytes) -> None:
    if len(data) < 0x54:
        raise SystemExit(f"{path}: too small for vector table ({len(data)} bytes)")

    initial_sp, reset_vector = struct.unpack_from("<II", data, 0)
    if (reset_vector & 1) == 0:
        raise SystemExit(
            f"{path}: reset vector 0x{reset_vector:08x} is not Thumb-aligned"
        )

    # DM factory images observed so far use SRAM addresses in either 0x1FFF_xxxx
    # or 0x2000_xxxx ranges, depending on linker/script generation.
    if not (0x1FFF0000 <= initial_sp <= 0x20010000):
        raise SystemExit(
            f"{path}: initial SP 0x{initial_sp:08x} is outside expected SRAM range"
        )

    print(
        f"{model}: plain ok, {len(data)} bytes, "
        f"sp=0x{initial_sp:08x}, reset=0x{reset_vector:08x}, sha256={sha256_short(data)}…"
    )


def check_model(dist: Path, model: str) -> None:
    plain_path = dist / f"{model}_plain.bin"
    enc_path = dist / f"{model}_enc.bin"

    plain = read_file(plain_path)
    enc = read_file(enc_path)

    check_plain_image(model, plain_path, plain)

    if len(plain) != len(enc):
        raise SystemExit(
            f"{model}: encrypted length mismatch: plain={len(plain)} enc={len(enc)}"
        )
    if plain == enc:
        raise SystemExit(f"{model}: encrypted output is byte-identical to plaintext")

    print(f"{model}: encrypted ok, {len(enc)} bytes, sha256={sha256_short(enc)}…")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate expected firmware outputs without cross-model equality checks."
    )
    parser.add_argument(
        "--dist",
        type=Path,
        default=Path("dist/development"),
        help="directory containing dm4310_* and dm8009_* firmware outputs",
    )
    args = parser.parse_args()

    for model in MODELS:
        check_model(args.dist, model)

    print("firmware outputs verified: artifact sanity checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
