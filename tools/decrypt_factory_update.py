#!/usr/bin/env python3
"""Decrypt an official Damiao APP update with an audited loader profile.

The vendor update file is a flat AES-256-CTR ciphertext.  This tool extracts
the key and initial counter from either the hash-pinned bootloader image or a
source-owned update profile, validates the decrypted vector table, re-encrypts
it, and only then writes the plaintext reference image.
"""

from __future__ import annotations

import argparse
import hashlib
import struct
from pathlib import Path

from pack_update import (
    aes256_ctr_transform,
    extract_bootloader_material,
    load_update_profile,
    validate_plain_app,
)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def arm_scatter_decode(data: bytes, output_length: int) -> bytes:
    """Decode the Arm Compiler scatter-compressed initialized RAM block."""
    output = bytearray(output_length)
    input_pos = 0
    output_pos = 0
    while output_pos < output_length:
        token = data[input_pos]
        input_pos += 1
        literals = token & 3
        if literals == 0:
            literals = data[input_pos]
            input_pos += 1
        match = token >> 4
        if match == 0:
            match = data[input_pos]
            input_pos += 1
        for _ in range(1, literals):
            if output_pos >= output_length:
                break
            output[output_pos] = data[input_pos]
            output_pos += 1
            input_pos += 1
        if match and output_pos < output_length:
            low = data[input_pos]
            input_pos += 1
            kind = token & 0x0C
            distance = low + (kind << 6)
            if kind == 0x0C:
                distance = low + (data[input_pos] << 8)
                input_pos += 1
            source = output_pos - distance
            if source < 0:
                raise ValueError("invalid backwards distance in scatter data")
            for _ in range(match + 2):
                if output_pos >= output_length:
                    break
                output[output_pos] = output[source]
                output_pos += 1
                source += 1
    return bytes(output)


def extract_factory_config(plaintext: bytes) -> bytes:
    expected_digest = (
        "65aab219268e9159b196d4578d3cd530e6fa90a731a82670a0d3d3be609b59d4"
    )
    if digest(plaintext) != expected_digest:
        raise ValueError(
            "factory configuration extraction currently supports only "
            "DM4310 V3 V5017.04"
        )
    app_base = 0x00020000
    packed_start = 0x0002AB90 - app_base
    packed_end = 0x0002C84C - app_base
    ram_base = 0x1FFFA510
    config_base = 0x1FFFA5C8
    initialized = arm_scatter_decode(
        plaintext[packed_start:packed_end], 0x2268
    )
    offset = config_base - ram_base
    record = initialized[offset:offset + 37 * 4]
    if len(record) != 37 * 4:
        raise ValueError("official factory configuration is truncated")
    # These three words are precisely the original function's erased-record
    # predicate, so none may be erased in the embedded factory template.
    words = struct.unpack("<37I", record)
    if any(words[index] == 0xFFFFFFFF for index in (0x08, 0x0E, 0x0F)):
        raise ValueError("official factory configuration is invalid")
    return record


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--encrypted", type=Path, required=True)
    key_source = parser.add_mutually_exclusive_group(required=True)
    key_source.add_argument("--bootloader", type=Path)
    key_source.add_argument("--profile", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--factory-config-output", type=Path)
    args = parser.parse_args()

    ciphertext = args.encrypted.read_bytes()
    if args.bootloader is not None:
        key, counter = extract_bootloader_material(args.bootloader.read_bytes())
    else:
        key, counter, _ = load_update_profile(args.profile)
    plaintext = aes256_ctr_transform(ciphertext, key, counter)
    stack, reset = validate_plain_app(plaintext)
    if aes256_ctr_transform(plaintext, key, counter) != ciphertext:
        raise RuntimeError("factory APP AES-CTR round trip failed")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(plaintext)
    if args.factory_config_output is not None:
        factory_config = extract_factory_config(plaintext)
        args.factory_config_output.parent.mkdir(parents=True, exist_ok=True)
        args.factory_config_output.write_bytes(factory_config)
    print(f"decrypted {len(ciphertext)} bytes")
    print(f"ciphertext SHA-256: {digest(ciphertext)}")
    print(f"plaintext  SHA-256: {digest(plaintext)}")
    print(f"initial MSP: 0x{stack:08x}")
    print(f"reset vector: 0x{reset:08x}")
    print(f"wrote: {args.output}")
    if args.factory_config_output is not None:
        print(
            "factory configuration SHA-256: "
            f"{digest(factory_config)}"
        )
        print(f"wrote: {args.factory_config_output}")


if __name__ == "__main__":
    main()
