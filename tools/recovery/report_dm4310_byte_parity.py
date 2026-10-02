#!/usr/bin/env python3
"""Report source-built DM4310 byte parity against the factory APP image.

This is an offline recovery tool.  It is deliberately not part of CMake or
the normal firmware build.  Function comparisons use the exact, potentially
non-contiguous address sets exported from the factory Ghidra program rather
than guessing a contiguous range from each entry point.
"""

from __future__ import annotations

import argparse
from collections import defaultdict
import csv
from dataclasses import dataclass
from hashlib import sha256
from io import BytesIO
from pathlib import Path
import re
import struct

from elftools.elf.constants import SH_FLAGS
from elftools.elf.elffile import ELFFile


ROOT = Path(__file__).resolve().parents[2]

FLASH_BASE = 0x00020000
FLASH_END = 0x00030000
VECTOR_SIZE = 0x240
FACTORY_FIXED_SOURCE = 0x8680
FACTORY_FIXED_BASE = 0x1FFF8000
FACTORY_FIXED_SIZE = 0x2510
FACTORY_FIXED_END = FACTORY_FIXED_BASE + FACTORY_FIXED_SIZE
FACTORY_SHA256 = (
    "65aab219268e9159b196d4578d3cd530e6fa90a731a82670a0d3d3be609b59d4"
)


@dataclass(frozen=True)
class FunctionResult:
    entry: int
    name: str
    region: str
    body_bytes: int
    ranges: str
    status: str
    missing_bytes: int
    expected_sha256: str
    current_sha256: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--factory",
        type=Path,
        default=ROOT / "reference/APP_DM4310_V3_V5017_04.decrypted.bin",
    )
    parser.add_argument(
        "--elf", type=Path, default=ROOT / "build/dm4310.elf"
    )
    parser.add_argument(
        "--binary", type=Path, default=ROOT / "build/dm4310.app.bin"
    )
    parser.add_argument(
        "--map", type=Path, default=ROOT / "build/dm4310.map"
    )
    parser.add_argument(
        "--hashes",
        type=Path,
        default=(
            ROOT / "recovered/dm4310/tables/factory_function_hashes.tsv"
        ),
    )
    parser.add_argument(
        "--ranges",
        type=Path,
        default=(
            ROOT
            / "recovered/dm4310/tables/factory_function_body_ranges.tsv"
        ),
    )
    parser.add_argument(
        "--details-output",
        type=Path,
        help="write the per-function comparison as TSV",
    )
    return parser.parse_args()


def digest(data: bytes) -> str:
    return sha256(data).hexdigest()


def read_tsv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as source:
        return list(csv.DictReader(source, delimiter="\t"))


def symbol_values(elf: ELFFile) -> dict[str, int]:
    table = elf.get_section_by_name(".symtab")
    if table is None:
        raise ValueError("ELF has no symbol table")
    return {symbol.name: symbol["st_value"] for symbol in table.iter_symbols()}


def elf_vma_bytes(elf: ELFFile) -> dict[int, int]:
    memory: dict[int, int] = {}
    for section in elf.iter_sections():
        if not section["sh_size"]:
            continue
        if section["sh_type"] == "SHT_NOBITS":
            continue
        if not section["sh_flags"] & SH_FLAGS.SHF_ALLOC:
            continue
        start = section["sh_addr"]
        for offset, value in enumerate(section.data()):
            address = start + offset
            previous = memory.setdefault(address, value)
            if previous != value:
                raise ValueError(
                    f"conflicting ELF bytes at {address:#010x}: "
                    f"{previous:#04x} != {value:#04x}"
                )
    return memory


def factory_byte(factory: bytes, address: int) -> int:
    if FLASH_BASE <= address < FLASH_BASE + len(factory):
        return factory[address - FLASH_BASE]
    if FACTORY_FIXED_BASE <= address < FACTORY_FIXED_END:
        offset = FACTORY_FIXED_SOURCE + address - FACTORY_FIXED_BASE
        return factory[offset]
    raise ValueError(f"factory address is outside mapped image: {address:#010x}")


def parse_ranges(value: str) -> list[tuple[int, int]]:
    result = []
    for item in value.split(","):
        start_text, end_text = item.split("-", 1)
        start = int(start_text, 16)
        end = int(end_text, 16)
        if end < start:
            raise ValueError(f"invalid function body range: {item}")
        result.append((start, end + 1))
    return result


def compare_functions(
    factory: bytes,
    current_memory: dict[int, int],
    hashes_path: Path,
    ranges_path: Path,
) -> list[FunctionResult]:
    hashes = {row["entry"]: row for row in read_tsv(hashes_path)}
    ranges = {row["entry"]: row for row in read_tsv(ranges_path)}
    if hashes.keys() != ranges.keys():
        missing_ranges = sorted(hashes.keys() - ranges.keys())
        missing_hashes = sorted(ranges.keys() - hashes.keys())
        raise ValueError(
            "factory function evidence differs: "
            f"missing ranges={missing_ranges}, missing hashes={missing_hashes}"
        )

    results = []
    for entry_text in sorted(hashes, key=lambda value: int(value, 16)):
        expected = hashes[entry_text]
        ownership = ranges[entry_text]
        if expected["body_bytes"] != ownership["body_bytes"]:
            raise ValueError(f"body size differs for {entry_text}")

        addresses = [
            address
            for start, end in parse_ranges(ownership["ranges"])
            for address in range(start, end)
        ]
        body_bytes = int(expected["body_bytes"], 16)
        if len(addresses) != body_bytes:
            raise ValueError(
                f"range byte count differs for {entry_text}: "
                f"{len(addresses)} != {body_bytes}"
            )

        factory_body = bytes(factory_byte(factory, address) for address in addresses)
        if digest(factory_body) != expected["sha256"]:
            raise ValueError(f"factory hash evidence is stale for {entry_text}")

        missing = sum(address not in current_memory for address in addresses)
        if missing:
            current_hash = ""
            status = "missing"
        else:
            current_body = bytes(current_memory[address] for address in addresses)
            current_hash = digest(current_body)
            status = "exact" if current_hash == expected["sha256"] else "different"

        entry = int(entry_text, 16)
        results.append(
            FunctionResult(
                entry=entry,
                name=expected["name"],
                region="fixed_sram" if entry >= FACTORY_FIXED_BASE else "flash",
                body_bytes=body_bytes,
                ranges=ownership["ranges"],
                status=status,
                missing_bytes=missing,
                expected_sha256=expected["sha256"],
                current_sha256=current_hash,
            )
        )
    return results


def same_byte_runs(left: bytes, right: bytes) -> list[tuple[int, int]]:
    runs = []
    start = None
    for offset, (left_byte, right_byte) in enumerate(zip(left, right)):
        if left_byte == right_byte and start is None:
            start = offset
        elif left_byte != right_byte and start is not None:
            runs.append((start, offset))
            start = None
    if start is not None:
        runs.append((start, min(len(left), len(right))))
    return runs


def fixed_section_results(
    elf: ELFFile, factory: bytes
) -> list[tuple[str, int, int, bool]]:
    results = []
    for section in elf.iter_sections():
        start = section["sh_addr"]
        end = start + section["sh_size"]
        if not section["sh_size"] or section["sh_type"] == "SHT_NOBITS":
            continue
        if not section["sh_flags"] & SH_FLAGS.SHF_ALLOC:
            continue
        low = max(start, FACTORY_FIXED_BASE)
        high = min(end, FACTORY_FIXED_END)
        if low >= high:
            continue
        actual = section.data()[low - start : high - start]
        expected = bytes(factory_byte(factory, address) for address in range(low, high))
        matching = sum(left == right for left, right in zip(actual, expected))
        results.append((section.name, len(actual), matching, actual == expected))
    return results


def normalize_map_owner(value: str) -> str:
    marker = "CMakeFiles/dm4310.dir/"
    if marker in value:
        return value.split(marker, 1)[1]
    for library in ("libgcc.a", "libg_nano.a", "libc_nano.a"):
        if library in value:
            return f"toolchain/{library}:{value.rsplit('(', 1)[-1]}"
    return value


def map_flash_owners(
    path: Path, image_end: int
) -> tuple[dict[str, int], int, int]:
    """Account file-backed ordinary Flash bytes from a GNU linker map."""
    lines = path.read_text(encoding="utf-8").splitlines()
    stop = next(
        index
        for index, line in enumerate(lines)
        if line.startswith(".data ") and "0x1fff8000" in line
    )
    input_pattern = re.compile(
        r"^\s+(?:\S+\s+)?0x([0-9a-fA-F]+)\s+"
        r"0x([0-9a-fA-F]+)\s+(.+)$"
    )
    fill_pattern = re.compile(
        r"\*fill\*\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)"
    )
    owners: dict[str, int] = defaultdict(int)
    fills: set[tuple[int, int]] = set()
    records: set[tuple[int, int, str]] = set()
    for line in lines[:stop]:
        fill = fill_pattern.search(line)
        if fill is not None:
            address = int(fill.group(1), 16)
            size = int(fill.group(2), 16)
            if FLASH_BASE <= address < image_end:
                fills.add((address, size))
            continue

        match = input_pattern.match(line)
        if match is None:
            continue
        address = int(match.group(1), 16)
        size = int(match.group(2), 16)
        owner = match.group(3).strip()
        if not FLASH_BASE <= address < image_end or not size:
            continue
        if not (".obj" in owner or ".a(" in owner or owner == "linker stubs"):
            continue
        records.add((address, size, owner))

    for _address, size, owner in records:
        owners[normalize_map_owner(owner)] += size
    owner_bytes = sum(owners.values())
    fill_bytes = sum(size for _address, size in fills)
    manual_bytes = image_end - FLASH_BASE - owner_bytes - fill_bytes
    if manual_bytes < 0:
        raise ValueError(
            f"linker map accounting exceeds ordinary Flash by {-manual_bytes} bytes"
        )
    return dict(owners), fill_bytes, manual_bytes


def write_details(path: Path, results: list[FunctionResult]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fields = (
        "entry",
        "name",
        "region",
        "body_bytes",
        "ranges",
        "status",
        "missing_bytes",
        "expected_sha256",
        "current_sha256",
    )
    with path.open("w", newline="", encoding="utf-8") as target:
        writer = csv.DictWriter(target, fieldnames=fields, delimiter="\t")
        writer.writeheader()
        for result in results:
            writer.writerow(
                {
                    "entry": f"0x{result.entry:08x}",
                    "name": result.name,
                    "region": result.region,
                    "body_bytes": f"0x{result.body_bytes:x}",
                    "ranges": result.ranges,
                    "status": result.status,
                    "missing_bytes": result.missing_bytes,
                    "expected_sha256": result.expected_sha256,
                    "current_sha256": result.current_sha256,
                }
            )


def main() -> None:
    args = parse_args()
    factory = args.factory.read_bytes()
    current = args.binary.read_bytes()
    factory_hash = digest(factory)
    if factory_hash != FACTORY_SHA256:
        raise ValueError(
            f"unexpected DM4310 factory SHA-256: {factory_hash}; "
            f"expected {FACTORY_SHA256}"
        )

    elf = ELFFile(BytesIO(args.elf.read_bytes()))
    symbols = symbol_values(elf)
    current_memory = elf_vma_bytes(elf)
    function_results = compare_functions(
        factory, current_memory, args.hashes, args.ranges
    )
    fixed_sections = fixed_section_results(elf, factory)

    overlap = min(len(factory), len(current))
    same_offset = sum(
        left == right for left, right in zip(factory[:overlap], current[:overlap])
    )
    runs = same_byte_runs(factory, current)
    common_prefix = 0
    while common_prefix < overlap and factory[common_prefix] == current[common_prefix]:
        common_prefix += 1

    factory_vectors = struct.unpack(
        f"<{VECTOR_SIZE // 4}I", factory[:VECTOR_SIZE]
    )
    current_vectors = struct.unpack(
        f"<{VECTOR_SIZE // 4}I", current[:VECTOR_SIZE]
    )
    exact_vectors = sum(
        left == right for left, right in zip(factory_vectors, current_vectors)
    )

    exact_functions = [result for result in function_results if result.status == "exact"]
    exact_function_bytes = sum(result.body_bytes for result in exact_functions)
    total_function_bytes = sum(result.body_bytes for result in function_results)
    exact_fixed_sections = [row for row in fixed_sections if row[3]]
    fixed_matching_bytes = sum(row[2] for row in fixed_sections)
    fixed_covered_bytes = sum(row[1] for row in fixed_sections)

    current_flash_size = symbols["__etext"] - FLASH_BASE
    current_fixed_size = symbols["__dm4310_aes_sbox_load__"] - symbols["__etext"]
    current_state_size = len(current) - (
        symbols["__dm4310_aes_sbox_load__"] - FLASH_BASE
    )
    factory_state_size = (
        len(factory) - FACTORY_FIXED_SOURCE - FACTORY_FIXED_SIZE
    )
    owners, fill_bytes, manual_bytes = map_flash_owners(
        args.map, symbols["__etext"]
    )

    print("DM4310 factory byte-parity dashboard")
    print(f"factory: {args.factory}")
    print(f"  size={len(factory)} ({len(factory):#x}) sha256={factory_hash}")
    print(f"current: {args.binary}")
    print(f"  size={len(current)} ({len(current):#x}) sha256={digest(current)}")
    print(
        f"image: delta={len(current) - len(factory):+d} "
        f"free_in_64k={FLASH_END - FLASH_BASE - len(current)} "
        f"whole_file_exact={'YES' if current == factory else 'NO'}"
    )
    print(
        f"same-offset bytes: {same_offset}/{len(factory)} "
        f"({same_offset / len(factory):.2%}); common prefix={common_prefix}"
    )
    longest = sorted(runs, key=lambda item: item[1] - item[0], reverse=True)[:5]
    print(
        "longest exact runs: "
        + ", ".join(
            f"{end - start}B@{FLASH_BASE + start:#010x}"
            for start, end in longest
        )
    )
    print(
        f"vectors: {exact_vectors}/{len(factory_vectors)} exact; "
        f"MSP={current_vectors[0]:#010x}/{factory_vectors[0]:#010x}; "
        f"Reset={current_vectors[1]:#010x}/{factory_vectors[1]:#010x}"
    )
    print(
        f"factory function bodies: {len(exact_functions)}/{len(function_results)} "
        f"exact; owned bytes={exact_function_bytes}/{total_function_bytes} "
        "(overlapping ownership counted)"
    )
    for region in ("flash", "fixed_sram"):
        selected = [result for result in function_results if result.region == region]
        exact = [result for result in selected if result.status == "exact"]
        missing = [result for result in selected if result.status == "missing"]
        print(
            f"  {region}: exact={len(exact)}/{len(selected)} "
            f"missing-address={len(missing)}"
        )
    print(
        f"fixed SRAM source sections: {len(exact_fixed_sections)}/{len(fixed_sections)} "
        f"exact; covered bytes={fixed_matching_bytes}/{fixed_covered_bytes}"
    )
    print("load-image composition:")
    print(
        f"  ordinary flash: factory={FACTORY_FIXED_SOURCE} "
        f"current={current_flash_size} "
        f"delta={current_flash_size - FACTORY_FIXED_SOURCE:+d}"
    )
    print(
        f"  fixed SRAM payload: factory={FACTORY_FIXED_SIZE} "
        f"current={current_fixed_size} "
        f"delta={current_fixed_size - FACTORY_FIXED_SIZE:+d}"
    )
    print(
        f"  compressed/generated state tail: factory={factory_state_size} "
        f"current={current_state_size} "
        f"delta={current_state_size - factory_state_size:+d}"
    )
    print(
        f"ordinary Flash accounting: linked-input={sum(owners.values())} "
        f"placement-fill={fill_bytes} linker/manual={manual_bytes}"
    )
    print("largest linked-input owners:")
    for owner, size in sorted(
        owners.items(), key=lambda item: item[1], reverse=True
    )[:12]:
        print(f"  {size:5d}  {owner}")

    if args.details_output is not None:
        write_details(args.details_output, function_results)
        print(f"function details: {args.details_output}")


if __name__ == "__main__":
    main()
