#!/usr/bin/env python3
"""Validate generated firmware artifacts without enforcing model equivalence.

For every model this proves that:
- the distributed plaintext is byte-identical to the source-built APP image
- the APP image fits the 64 KiB application partition and has valid vectors
- AES-256-CTR decryption reproduces the plaintext byte-for-byte
- the package manifest hashes and sizes describe the emitted artifacts

It does not compare models against each other.  Model-specific profiles may
legitimately produce different images.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

from pack_update import (APP_BASE, APP_END, aes256_ctr_transform,
                         load_update_profile, validate_plain_app)

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


def check_manifest(package_root: Path, model: str, plain: bytes,
                   encrypted: bytes, expected_variant: str) -> None:
    path = package_root / model / "manifest.json"
    try:
        manifest = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        raise SystemExit(f"missing package manifest: {path}")
    except json.JSONDecodeError as error:
        raise SystemExit(f"invalid package manifest {path}: {error}") from error

    expected = {
        "format": "damiao-can-update-v1",
        "firmware_variant": expected_variant,
        "plaintext_size": len(plain),
        "plaintext_sha256": hashlib.sha256(plain).hexdigest(),
        "ciphertext_sha256": hashlib.sha256(encrypted).hexdigest(),
    }
    for field, expected_value in expected.items():
        if manifest.get(field) != expected_value:
            raise SystemExit(
                f"{path}: {field} mismatch: "
                f"{manifest.get(field)!r} != {expected_value!r}"
            )


def check_model(dist: Path, build_dir: Path, package_root: Path, model: str,
                key: bytes, counter: bytes, build_suffix: str,
                expected_variant: str) -> None:
    plain_path = dist / f"{model}_plain.bin"
    enc_path = dist / f"{model}_enc.bin"
    build_path = build_dir / f"{model}{build_suffix}.app.bin"

    plain = read_file(plain_path)
    enc = read_file(enc_path)
    built = read_file(build_path)

    if len(plain) > APP_END - APP_BASE:
        raise SystemExit(
            f"{model}: {len(plain)}-byte image exceeds the 64 KiB APP partition"
        )
    if plain != built:
        raise SystemExit(
            f"{model}: distributed plaintext differs from source build {build_path}"
        )

    try:
        validate_plain_app(plain)
    except ValueError as error:
        raise SystemExit(f"{plain_path}: {error}") from error
    check_plain_image(model, plain_path, plain)

    if len(plain) != len(enc):
        raise SystemExit(
            f"{model}: encrypted length mismatch: plain={len(plain)} enc={len(enc)}"
        )
    if plain == enc:
        raise SystemExit(f"{model}: encrypted output is byte-identical to plaintext")
    if aes256_ctr_transform(enc, key, counter) != plain:
        raise SystemExit(
            f"{model}: AES-256-CTR decrypt does not reproduce plaintext"
        )

    check_manifest(package_root, model, plain, enc, expected_variant)
    print(
        f"{model}: encrypted round-trip and manifest ok, {len(enc)} bytes, "
        f"sha256={sha256_short(enc)}…"
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate expected firmware outputs without cross-model equality checks."
    )
    parser.add_argument(
        "--dist",
        type=Path,
        default=Path("dist/development/factory"),
        help="directory containing per-model plain and encrypted firmware outputs",
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=Path("build"),
        help="directory containing source-built MODEL.app.bin images",
    )
    parser.add_argument(
        "--build-suffix",
        default="",
        help="suffix appended to the model when locating source-built APP images",
    )
    parser.add_argument(
        "--package-root",
        type=Path,
        default=Path("build/package/factory"),
        help="directory containing per-model package manifests",
    )
    parser.add_argument(
        "--profile",
        type=Path,
        default=Path("config/update_profile.json"),
        help="audited update profile used to decrypt the encrypted outputs",
    )
    parser.add_argument(
        "--expected-variant",
        choices=("factory", "raw", "no_response", "raw_no_response"),
        default="factory",
        help="firmware variant expected in every package manifest",
    )
    args = parser.parse_args()

    try:
        key, counter, _ = load_update_profile(args.profile)
    except (OSError, ValueError) as error:
        raise SystemExit(f"invalid update profile {args.profile}: {error}") from error

    for model in MODELS:
        check_model(args.dist, args.build_dir, args.package_root, model,
                    key, counter, args.build_suffix, args.expected_variant)

    print(
        f"{args.expected_variant} firmware outputs verified: source, size, "
        "AES round-trip and manifests passed"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
