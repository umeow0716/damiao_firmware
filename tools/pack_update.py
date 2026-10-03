#!/usr/bin/env python3
"""Build Damiao bootloader update artifacts from a plaintext app image.

The original loader receives descending 8 KiB chunks.  It validates a
CRC-8/MAXIM over each ciphertext chunk, decrypts the stream with AES-256-CTR,
and writes the resulting plaintext at 0x00020000.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path


APP_BASE = 0x00020000
APP_END = 0x00030000
SRAM_BASE = 0x1FFF8000
SRAM_END = 0x20008000
CHUNK_SIZE = 0x2000
CAN_ID = 0x7FF
BOOTLOADER_SHA256 = "4bee47463774574683d1b5a96ba63b1f231cedcdc422d589acb230c0835fa463"
KEY_OFFSET = 0x734F
KEY_SIZE = 32
COUNTER_OFFSET = 0x736F
COUNTER_SIZE = 16


SBOX = (
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5,
    0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
    0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0,
    0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
    0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC,
    0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A,
    0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
    0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0,
    0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
    0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B,
    0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85,
    0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
    0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5,
    0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
    0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17,
    0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88,
    0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
    0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C,
    0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
    0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9,
    0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6,
    0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
    0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E,
    0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
    0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94,
    0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68,
    0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16,
)


def _xtime(value: int) -> int:
    return ((value << 1) ^ (0x1B if value & 0x80 else 0)) & 0xFF


def _expand_aes256_key(key: bytes) -> bytes:
    if len(key) != 32:
        raise ValueError("AES-256 requires a 32-byte key")
    expanded = bytearray(key)
    rcon = 1
    while len(expanded) < 240:
        temp = list(expanded[-4:])
        position = len(expanded) % 32
        if position == 0:
            temp = temp[1:] + temp[:1]
            temp = [SBOX[value] for value in temp]
            temp[0] ^= rcon
            rcon = _xtime(rcon)
        elif position == 16:
            temp = [SBOX[value] for value in temp]
        base = len(expanded) - 32
        for value in temp:
            expanded.append(expanded[base] ^ value)
            base += 1
    return bytes(expanded)


def _mix_column(state: bytearray, base: int) -> None:
    a0, a1, a2, a3 = state[base:base + 4]
    total = a0 ^ a1 ^ a2 ^ a3
    state[base] ^= total ^ _xtime(a0 ^ a1)
    state[base + 1] ^= total ^ _xtime(a1 ^ a2)
    state[base + 2] ^= total ^ _xtime(a2 ^ a3)
    state[base + 3] ^= total ^ _xtime(a3 ^ a0)


def _aes256_encrypt_block(key_schedule: bytes, block: bytes) -> bytes:
    if len(key_schedule) != 240 or len(block) != 16:
        raise ValueError("invalid AES block or expanded key length")
    state = bytearray(block)
    for index in range(16):
        state[index] ^= key_schedule[index]
    for round_index in range(1, 15):
        state[:] = bytes(SBOX[value] for value in state)
        state[:] = bytes((
            state[0], state[5], state[10], state[15],
            state[4], state[9], state[14], state[3],
            state[8], state[13], state[2], state[7],
            state[12], state[1], state[6], state[11],
        ))
        if round_index != 14:
            for base in range(0, 16, 4):
                _mix_column(state, base)
        round_key = key_schedule[round_index * 16:(round_index + 1) * 16]
        for index in range(16):
            state[index] ^= round_key[index]
    return bytes(state)


def aes256_ctr_transform(data: bytes, key: bytes, initial_counter: bytes) -> bytes:
    """Encrypt or decrypt using the loader's big-endian AES-CTR counter."""
    if len(initial_counter) != 16:
        raise ValueError("AES-CTR requires a 16-byte initial counter")
    schedule = _expand_aes256_key(key)
    counter = int.from_bytes(initial_counter, "big")
    output = bytearray()
    for offset in range(0, len(data), 16):
        stream = _aes256_encrypt_block(schedule, counter.to_bytes(16, "big"))
        part = data[offset:offset + 16]
        output.extend(left ^ right for left, right in zip(part, stream))
        counter = (counter + 1) & ((1 << 128) - 1)
    return bytes(output)


def crc8_maxim(data: bytes) -> int:
    value = 0
    for item in data:
        value ^= item
        for _ in range(8):
            value = (value >> 1) ^ (0x8C if value & 1 else 0)
    return value


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def extract_bootloader_material(bootloader: bytes) -> tuple[bytes, bytes]:
    actual_hash = sha256(bootloader)
    if actual_hash != BOOTLOADER_SHA256:
        raise ValueError(
            "bootloader SHA-256 does not match the analyzed image; refusing "
            "to use fixed key/counter offsets"
        )
    key = bootloader[KEY_OFFSET:KEY_OFFSET + KEY_SIZE]
    counter = bootloader[COUNTER_OFFSET:COUNTER_OFFSET + COUNTER_SIZE]
    if len(key) != KEY_SIZE or len(counter) != COUNTER_SIZE:
        raise ValueError("bootloader is truncated before key/counter material")
    return key, counter


def load_update_profile(profile_path: Path) -> tuple[bytes, bytes, dict[str, object]]:
    """Load the source-owned update contract used by both build and loader."""
    profile = json.loads(profile_path.read_text(encoding="utf-8"))
    if profile.get("format") != "damiao-can-update-v1":
        raise ValueError("unsupported update profile format")
    if int(str(profile.get("app_base")), 0) != APP_BASE:
        raise ValueError("update profile app_base does not match the linker contract")
    if int(str(profile.get("app_end")), 0) != APP_END:
        raise ValueError("update profile app_end does not match the linker contract")
    if int(str(profile.get("update_can_id")), 0) != CAN_ID:
        raise ValueError("update profile CAN ID does not match the framing code")
    if int(profile.get("chunk_size", 0)) != CHUNK_SIZE:
        raise ValueError("update profile chunk size does not match the framing code")
    try:
        key = bytes.fromhex(str(profile["key_hex"]))
        counter = bytes.fromhex(str(profile["initial_counter_hex"]))
    except (KeyError, ValueError) as error:
        raise ValueError("update profile has invalid key/counter hex") from error
    if len(key) != KEY_SIZE or len(counter) != COUNTER_SIZE:
        raise ValueError("update profile must contain a 32-byte key and 16-byte counter")
    return key, counter, profile


def validate_plain_app(image: bytes) -> tuple[int, int]:
    if len(image) < 8 or len(image) > (APP_END - APP_BASE):
        raise ValueError("app image length is outside the 64 KiB source app region")
    if len(image) % 4:
        raise ValueError("app image length must be a multiple of four bytes")
    stack, reset = struct.unpack_from("<II", image)
    if not (SRAM_BASE <= stack <= SRAM_END and stack % 8 == 0):
        raise ValueError(f"invalid initial MSP 0x{stack:08x}")
    reset_address = reset & ~1
    if not (reset & 1) or not (APP_BASE <= reset_address < APP_BASE + len(image)):
        raise ValueError(f"invalid Thumb reset vector 0x{reset:08x}")
    return stack, reset


def build_update_records(ciphertext: bytes) -> list[bytes]:
    """Build the loader's transport-neutral ``#seq#`` chunk records.

    The installed firmware has parallel UART and MCAN receive paths for this
    record contract.  ``app_update.enc.bin`` remains the flat ciphertext
    consumed by the vendor UART updater; this helper models the records that
    updater sends and that the CAN exporter splits into frames.
    """
    chunks = [ciphertext[offset:offset + CHUNK_SIZE]
              for offset in range(0, len(ciphertext), CHUNK_SIZE)]
    if not chunks:
        raise ValueError("update payload must not be empty")
    if len(chunks) > 256:
        raise ValueError("update protocol sequence field only supports 256 chunks")
    records: list[bytes] = []
    first_sequence = len(chunks) - 1
    for chunk_index, chunk in enumerate(chunks):
        sequence = first_sequence - chunk_index
        record = (b"#" + bytes((sequence,)) + b"#" +
                  struct.pack("<H", len(chunk)) + chunk +
                  bytes((crc8_maxim(chunk),)))
        records.append(record)
    return records


def build_wire_frames(ciphertext: bytes, *, can_fd: bool = False,
                      bit_rate_switch: bool = False) -> list[dict[str, object]]:
    if bit_rate_switch and not can_fd:
        raise ValueError("bit-rate switching requires CAN FD")
    frames: list[dict[str, object]] = []
    records = build_update_records(ciphertext)
    for chunk_index, record in enumerate(records):
        sequence = record[1]
        for frame_index, offset in enumerate(range(0, len(record), 8)):
            data = record[offset:offset + 8]
            frames.append({
                "can_id": CAN_ID,
                "chunk": chunk_index,
                "sequence": sequence,
                "frame": frame_index,
                "can_fd": can_fd,
                "bit_rate_switch": bit_rate_switch,
                "data": data.hex(),
            })
    return frames


def write_package(app_path: Path, output_dir: Path, purpose: str = "development",
                  profile_path: Path | None = None,
                  bootloader_path: Path | None = None,
                  plain_name: str = "app_plain.bin",
                  encrypted_name: str = "app_update.enc.bin",
                  firmware_variant: str = "factory") -> None:
    if firmware_variant not in ("factory", "raw", "no_response", "raw_no_response"):
        raise ValueError(f"unsupported firmware variant: {firmware_variant}")
    plaintext = app_path.read_bytes()
    stack, reset = validate_plain_app(plaintext)
    profile: dict[str, object] | None = None
    bootloader: bytes | None = None
    if profile_path is not None:
        key, counter, profile = load_update_profile(profile_path)
    elif bootloader_path is not None:
        bootloader = bootloader_path.read_bytes()
        key, counter = extract_bootloader_material(bootloader)
    else:
        raise ValueError("an update profile or audited bootloader is required")
    ciphertext = aes256_ctr_transform(plaintext, key, counter)
    transport: dict[str, object] = {}
    if profile is not None:
        raw_transport = profile.get("can_transport", {})
        if not isinstance(raw_transport, dict):
            raise ValueError("update profile can_transport must be an object")
        transport = raw_transport
    can_fd = bool(transport.get("can_fd", False))
    bit_rate_switch = bool(transport.get("bit_rate_switch", False))
    frames = build_wire_frames(
        ciphertext, can_fd=can_fd, bit_rate_switch=bit_rate_switch
    )

    output_dir.mkdir(parents=True, exist_ok=True)
    plain_output = output_dir / plain_name
    encrypted_output = output_dir / encrypted_name
    frames_output = output_dir / "app_update.frames.jsonl"
    can_output = output_dir / "app_update.cansend"
    manifest_output = output_dir / "manifest.json"

    plain_output.write_bytes(plaintext)
    encrypted_output.write_bytes(ciphertext)
    frames_output.write_text(
        "".join(json.dumps(frame, separators=(",", ":")) + "\n" for frame in frames),
        encoding="utf-8",
    )
    def cansend_line(frame: dict[str, object]) -> str:
        separator = "##1" if frame["bit_rate_switch"] else (
            "##0" if frame["can_fd"] else "#"
        )
        return (f"cansend can0 {frame['can_id']:03X}{separator}"
                f"{str(frame['data']).upper()}\n")

    can_output.write_text("".join(cansend_line(frame) for frame in frames),
                          encoding="utf-8")
    manifest = {
        "format": "damiao-can-update-v1",
        "artifact_purpose": purpose,
        "firmware_variant": firmware_variant,
        "source": str(app_path),
        "app_base": f"0x{APP_BASE:08x}",
        "plaintext_size": len(plaintext),
        "plaintext_sha256": sha256(plaintext),
        "ciphertext_sha256": sha256(ciphertext),
        "initial_msp": f"0x{stack:08x}",
        "reset_vector": f"0x{reset:08x}",
        "cipher": "AES-256-CTR",
        "counter_increment": "128-bit big-endian",
        "chunk_size": CHUNK_SIZE,
        "chunk_count": (len(plaintext) + CHUNK_SIZE - 1) // CHUNK_SIZE,
        "sequence_order": "descending-to-zero",
        "crc": "CRC-8/MAXIM over ciphertext chunk",
        "uart_payload_artifact": encrypted_output.name,
        "uart_record_contract": "# + sequence + # + uint16_le(length) + ciphertext + crc8",
        "can_id": f"0x{CAN_ID:03x}",
        "can_fd": can_fd,
        "bit_rate_switch": bit_rate_switch,
        "nominal_bit_rate": transport.get("nominal_bit_rate"),
        "data_bit_rate": transport.get("data_bit_rate"),
        "can_frame_count": len(frames),
        "direct_flash_artifact": plain_output.name,
        "bootloader_update_artifact": encrypted_output.name,
        "socketcan_script": can_output.name,
    }
    if profile_path is not None and profile is not None:
        manifest["update_profile"] = str(profile_path)
        manifest["update_profile_name"] = profile.get("profile_name")
        manifest["profile_provenance_bootloader_sha256"] = profile.get(
            "provenance_bootloader_sha256"
        )
    elif bootloader is not None:
        manifest["bootloader_sha256"] = sha256(bootloader)
    manifest_output.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

    if aes256_ctr_transform(ciphertext, key, counter) != plaintext:
        raise RuntimeError("AES-CTR round-trip verification failed")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app", type=Path, required=True,
                        help="plaintext app image linked for 0x00020000")
    material = parser.add_mutually_exclusive_group(required=True)
    material.add_argument("--profile", type=Path,
                          help="source-owned update profile JSON")
    material.add_argument("--bootloader", type=Path,
                          help="audited historical bootloader (reference use only)")
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--purpose", choices=("development", "historical-reference"),
                        default="development")
    parser.add_argument("--firmware-variant",
                        choices=("factory", "raw", "no_response", "raw_no_response"),
                        default="factory",
                        help="runtime behavior variant recorded in the manifest")
    parser.add_argument("--plain-name", default="app_plain.bin",
                        help="plaintext output filename inside --output-dir")
    parser.add_argument("--encrypted-name", default="app_update.enc.bin",
                        help="encrypted output filename inside --output-dir")
    args = parser.parse_args()
    write_package(args.app, args.output_dir, args.purpose,
                  profile_path=args.profile, bootloader_path=args.bootloader,
                  plain_name=args.plain_name, encrypted_name=args.encrypted_name,
                  firmware_variant=args.firmware_variant)


if __name__ == "__main__":
    main()
