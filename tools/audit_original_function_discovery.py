#!/usr/bin/env python3
"""Fail closed when a reachable original function entry is missing from Ghidra.

This complements Ghidra's function discovery with two independent checks:

* every immediate Thumb BL/BLX destination in the exported assembly must be a
  function start; and
* every aligned, odd Thumb code pointer stored in the immutable images or
  their initialized-data images must resolve to a function start.

The second check is important for interrupt and callback routines: they may
never be the destination of a direct call and therefore can be missed by an
ordinary call-graph-only audit.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


ROOT = Path(__file__).resolve().parents[1]
RAW = ROOT / "recovered/raw"
APP_PATH = ROOT / "reference/official/APP_DM4310_V3_V5017_04.decrypted.bin"
BOOT_PATH = ROOT / "bootloader.bin"

EXPECTED_HASHES = {
    "app": "65aab219268e9159b196d4578d3cd530e6fa90a731a82670a0d3d3be609b59d4",
    "bootloader": "4bee47463774574683d1b5a96ba63b1f231cedcdc422d589acb230c0835fa463",
}

# These ranges contain executable bytes in the Ghidra memory maps. The upper
# bound is exclusive. APP/boot runtime code is copied from Flash at startup.
EXECUTABLE_RANGES = {
    "app": ((0x00020250, 0x00028680), (0x1FFF8000, 0x1FFFA510)),
    "bootloader": ((0x00000250, 0x000073E8), (0x1FFF8000, 0x1FFF829C)),
}

# Exact word locations which happen to look like Thumb pointers. They are
# protocol/string/table data, not addresses. Hash-pinning the images above
# prevents this list from silently applying to a different binary.
FALSE_POINTER_WORDS = {
    "app": set(),
    "bootloader": {
        0x000064A4,  # CR/LF text bytes: 0x00000a0d
        0x00006B74,  # CR/LF text bytes: 0x00000a0d
        0x00006BF0,  # AES/table bytes: 0x0000736f
        0x00006BF4,  # AES/table bytes: 0x0000734f
        0x00006FF4,  # CR/LF text bytes: 0x00000a0d
        0x00007394,  # CAN/protocol constant: 0x000007ff
        0x000073B0,  # CAN/protocol constant: 0x000007ff
        0x1FFF8308,  # initialized ASCII/data bytes: 0x00006973
    },
}

# These three callback pointers exposed the blind spot in the previous
# 401-entry inventory. Keep them as named regression checks.
REQUIRED_CALLBACK_POINTERS = {
    ("bootloader", 0x00000BB4): (0x00005C30, "debug_uart_receive_irq"),
    ("bootloader", 0x00000BB8): (0x00005BEC, "debug_uart_error_irq"),
    ("bootloader", 0x00005E18): (0x000064A8, "debug_putchar"),
}

FUNCTION_COLUMNS = {"address", "size", "name", "calling_convention"}
ASM_COLUMNS = {
    "function_address",
    "function",
    "instruction_address",
    "bytes",
    "instruction",
}
DIRECT_CALL = re.compile(r"(?:bl|blx)\s+(0x[0-9a-fA-F]+)\Z")


@dataclass(frozen=True)
class MemoryImage:
    label: str
    base: int
    data: bytes


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def load_tsv(path: Path, columns: set[str]) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        if reader.fieldnames is None or set(reader.fieldnames) != columns:
            raise ValueError(f"{path}: unexpected columns {reader.fieldnames!r}")
        return list(reader)


def load_functions(image: str) -> dict[int, str]:
    path = RAW / f"{image}_functions.tsv"
    functions: dict[int, str] = {}
    for row in load_tsv(path, FUNCTION_COLUMNS):
        address = int(row["address"], 16)
        if address in functions:
            raise ValueError(f"{path}: duplicate function 0x{address:08x}")
        functions[address] = row["name"]
    return functions


def decode_arm_scatter(data: bytes, output_length: int) -> bytes:
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
                raise ValueError("invalid APP scatter back-reference")
            for _ in range(match + 2):
                if output_pos >= output_length:
                    break
                output[output_pos] = output[source]
                output_pos += 1
                source += 1
    return bytes(output)


def decode_iar_init(data: bytes, prefix: bytes, output_length: int) -> bytes:
    work = bytearray(prefix) + bytearray(output_length)
    input_pos = 0
    output_pos = len(prefix)
    end = len(work)
    while output_pos < end:
        token = data[input_pos]
        input_pos += 1
        literals = token & 7
        if literals == 0:
            literals = data[input_pos]
            input_pos += 1
        run = token >> 4
        if run == 0:
            run = data[input_pos]
            input_pos += 1
        for _ in range(1, literals):
            if output_pos >= end:
                break
            work[output_pos] = data[input_pos]
            output_pos += 1
            input_pos += 1
        if (token & 8) == 0:
            for _ in range(run):
                if output_pos >= end:
                    break
                work[output_pos] = 0
                output_pos += 1
        else:
            distance = data[input_pos]
            input_pos += 1
            source = output_pos - distance
            if source < 0:
                raise ValueError("invalid bootloader IAR back-reference")
            for _ in range(run + 2):
                if output_pos >= end:
                    break
                work[output_pos] = work[source]
                output_pos += 1
                source += 1
    return bytes(work[len(prefix):])


def memory_images(image: str, raw: bytes) -> tuple[MemoryImage, ...]:
    if image == "app":
        initialized = decode_arm_scatter(raw[0xAB90:0xC84C], 0x2268)
        return (
            MemoryImage("flash", 0x00020000, raw),
            MemoryImage("initialized-data", 0x1FFFA510, initialized),
        )
    prefix = raw[0x73E8:0x7684]
    initialized = decode_iar_init(raw[0x7684:0x7884], prefix, 0x190)
    return (
        MemoryImage("flash", 0x00000000, raw),
        MemoryImage("initialized-data", 0x1FFF829C, initialized),
    )


def in_executable_range(image: str, address: int) -> bool:
    return any(start <= address < end for start, end in EXECUTABLE_RANGES[image])


def scan_pointer_words(
    image: str,
    memories: Iterable[MemoryImage],
    functions: dict[int, str],
) -> tuple[list[dict[str, object]], list[dict[str, object]]]:
    resolved: list[dict[str, object]] = []
    false_positives: list[dict[str, object]] = []
    seen_false_words: set[int] = set()
    for memory in memories:
        for offset in range(0, len(memory.data) - 3, 4):
            value = struct.unpack_from("<I", memory.data, offset)[0]
            target = value & ~1
            if not (value & 1) or not in_executable_range(image, target):
                continue
            source = memory.base + offset
            item = {
                "memory": memory.label,
                "source": f"0x{source:08x}",
                "value": f"0x{value:08x}",
                "target": f"0x{target:08x}",
            }
            if source in FALSE_POINTER_WORDS[image]:
                false_positives.append(item)
                seen_false_words.add(source)
            elif target in functions:
                item["function"] = functions[target]
                resolved.append(item)
            else:
                raise ValueError(
                    f"{image}:{memory.label}: aligned Thumb pointer at "
                    f"0x{source:08x} targets missing function 0x{target:08x}"
                )
    missing_false_words = FALSE_POINTER_WORDS[image] - seen_false_words
    if missing_false_words:
        values = ", ".join(f"0x{item:08x}" for item in sorted(missing_false_words))
        raise ValueError(f"{image}: stale false-pointer exceptions: {values}")
    return resolved, false_positives


def scan_direct_calls(image: str, functions: dict[int, str]) -> dict[str, object]:
    path = RAW / f"{image}_asm.tsv"
    direct_calls = 0
    indirect_calls = 0
    direct_targets: set[int] = set()
    for row in load_tsv(path, ASM_COLUMNS):
        instruction = row["instruction"].strip()
        opcode = instruction.partition(" ")[0]
        if opcode not in {"bl", "blx"}:
            continue
        match = DIRECT_CALL.fullmatch(instruction)
        if match is None:
            indirect_calls += 1
            continue
        direct_calls += 1
        target = int(match.group(1), 16) & ~1
        direct_targets.add(target)
        if target not in functions:
            raise ValueError(
                f"{path}:{row['instruction_address']}: direct call targets "
                f"missing function 0x{target:08x}"
            )
    return {
        "direct_call_sites": direct_calls,
        "unique_direct_targets": len(direct_targets),
        "indirect_call_sites": indirect_calls,
        "missing_direct_targets": 0,
    }


def validate_required_callbacks(
    images: dict[str, bytes], functions: dict[str, dict[int, str]]
) -> list[dict[str, str]]:
    result: list[dict[str, str]] = []
    for (image, source), (expected_target, expected_name) in sorted(
        REQUIRED_CALLBACK_POINTERS.items()
    ):
        raw = images[image]
        base = 0x00020000 if image == "app" else 0
        offset = source - base
        if not 0 <= offset <= len(raw) - 4:
            raise ValueError(f"{image}: callback source 0x{source:08x} is out of range")
        value = struct.unpack_from("<I", raw, offset)[0]
        target = value & ~1
        actual_name = functions[image].get(target)
        if not value & 1 or target != expected_target or actual_name != expected_name:
            raise ValueError(
                f"{image}: callback at 0x{source:08x}: expected "
                f"{expected_name}@0x{expected_target:08x}, got "
                f"{actual_name}@0x{target:08x}"
            )
        result.append(
            {
                "image": image,
                "source": f"0x{source:08x}",
                "target": f"0x{target:08x}",
                "function": actual_name,
            }
        )
    return result


def markdown(report: dict[str, object]) -> str:
    images = report["images"]
    lines = [
        "# Original function discovery audit",
        "",
        "This fail-closed audit supplements Ghidra function discovery with raw "
        "direct-call and aligned Thumb-pointer scans. It is intended to catch "
        "address-taken IRQ/callback routines that do not appear as ordinary calls.",
        "",
        f"Result: **PASS**, {report['total_functions']} function entries "
        f"(APP {images['app']['function_count']}, bootloader "
        f"{images['bootloader']['function_count']}).",
        "",
        "| Image | Functions | Direct call sites | Unique direct targets | "
        "Indirect call sites | Resolved pointer words | Reviewed data lookalikes |",
        "|---|---:|---:|---:|---:|---:|---:|",
    ]
    for image in ("app", "bootloader"):
        item = images[image]
        lines.append(
            f"| {image} | {item['function_count']} | "
            f"{item['calls']['direct_call_sites']} | "
            f"{item['calls']['unique_direct_targets']} | "
            f"{item['calls']['indirect_call_sites']} | "
            f"{item['resolved_pointer_words']} | "
            f"{item['reviewed_data_lookalikes']} |"
        )
    lines += [
        "",
        "The bootloader pointer scan found the three callbacks that were absent "
        "from the former 401-entry inventory:",
        "",
    ]
    for callback in report["required_callbacks"]:
        lines.append(
            f"- `{callback['source']}` -> `{callback['target']}` "
            f"`{callback['function']}`"
        )
    lines += [
        "",
        "All immediate BL/BLX destinations resolve to exported function starts. "
        "All non-exempt aligned Thumb pointers in the raw Flash images and decoded "
        "initialized-data images resolve as well. Exempt words are exact, reviewed "
        "text/protocol/table values under SHA-256-pinned inputs.",
        "",
        "This cannot mathematically prove the absence of intentionally hidden, "
        "computed/encoded, unaligned or completely unreachable code. There is no "
        "evidence of those patterns here. Any normally reachable routine should be "
        "exposed by a vector, immediate call, stored callback pointer or Ghidra's "
        "control-flow analysis, all of which are covered by this evidence bundle.",
        "",
    ]
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", type=Path)
    parser.add_argument("--markdown", type=Path)
    args = parser.parse_args()

    paths = {"app": APP_PATH, "bootloader": BOOT_PATH}
    images = {name: path.read_bytes() for name, path in paths.items()}
    for image, data in images.items():
        actual_hash = sha256(data)
        if actual_hash != EXPECTED_HASHES[image]:
            raise SystemExit(
                f"{paths[image]}: expected SHA-256 {EXPECTED_HASHES[image]}, "
                f"got {actual_hash}"
            )

    functions = {image: load_functions(image) for image in images}
    report_images: dict[str, object] = {}
    try:
        for image, raw in images.items():
            calls = scan_direct_calls(image, functions[image])
            pointers, false_positives = scan_pointer_words(
                image, memory_images(image, raw), functions[image]
            )
            report_images[image] = {
                "function_count": len(functions[image]),
                "calls": calls,
                "resolved_pointer_words": len(pointers),
                "reviewed_data_lookalikes": len(false_positives),
                "pointer_words": pointers,
                "data_lookalikes": false_positives,
            }
        callbacks = validate_required_callbacks(images, functions)
    except (IndexError, OSError, ValueError) as error:
        raise SystemExit(f"original function discovery audit failed: {error}") from error

    report: dict[str, object] = {
        "schema": 1,
        "result": "PASS",
        "total_functions": sum(len(item) for item in functions.values()),
        "images": report_images,
        "required_callbacks": callbacks,
        "residual_limit": (
            "Cannot prove absence of deliberately computed/encoded, unaligned, "
            "or unreachable entry points; no such pattern was observed."
        ),
    }
    rendered = markdown(report)
    if args.json is not None:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    if args.markdown is not None:
        args.markdown.parent.mkdir(parents=True, exist_ok=True)
        args.markdown.write_text(rendered, encoding="utf-8")
    print(
        "original function discovery: PASS; "
        f"functions={report['total_functions']}; "
        f"app pointers={report_images['app']['resolved_pointer_words']}; "
        "bootloader pointers="
        f"{report_images['bootloader']['resolved_pointer_words']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
