#!/usr/bin/env python3
"""Validate the reproducible Ghidra function, call-graph and ASM exports."""

from __future__ import annotations

import csv
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RAW = ROOT / "recovered/raw"

FUNCTION_COLUMNS = {"address", "size", "name", "calling_convention"}
CALL_COLUMNS = {"caller_address", "caller", "callee_address", "callee"}
ASM_COLUMNS = {
    "function_address",
    "function",
    "instruction_address",
    "bytes",
    "instruction",
}

REQUIRED_ROOTS = {
    "app": {
        0x000252F4: "main",
        0x00022244: "debug_uart_receive_irq",
        0x1FFF8136: "adc_foc_control_irq",
        0x1FFF8744: "position_sensor_timer_irq",
        0x1FFF8770: "position_sensor_dma_irq",
        0x1FFF88B8: "mcan1_receive_irq",
    },
    "bootloader": {
        0x0000684C: "main",
        0x00002E68: "mcan1_receive_irq",
        0x000064C4: "debug_command_handler",
        0x00006608: "firmware_update_frame_handler",
    },
}

# Ghidra preserves the factory image's one-byte entry at erased Flash
# 0x12000, but 0xff is not a Thumb instruction and therefore has no listing.
# The semantic matrix tracks this separately as an intentional deviation.
ERASED_FUNCTION_ENTRIES = {"app": set(), "bootloader": {0x00012000}}


def rows(path: Path, columns: set[str]) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        if reader.fieldnames is None or set(reader.fieldnames) != columns:
            raise ValueError(f"{path}: unexpected columns {reader.fieldnames!r}")
        return list(reader)


def address(value: str, path: Path) -> int:
    try:
        return int(value, 16)
    except ValueError as error:
        raise ValueError(f"{path}: invalid address {value!r}") from error


def verify_image(image: str) -> tuple[int, int, int]:
    functions_path = RAW / f"{image}_functions.tsv"
    calls_path = RAW / f"{image}_calls.tsv"
    assembly_path = RAW / f"{image}_asm.tsv"

    function_rows = rows(functions_path, FUNCTION_COLUMNS)
    functions: dict[int, str] = {}
    for row in function_rows:
        entry = address(row["address"], functions_path)
        if entry in functions:
            raise ValueError(f"{functions_path}: duplicate function 0x{entry:08x}")
        functions[entry] = row["name"]

    for entry, expected_name in REQUIRED_ROOTS[image].items():
        if functions.get(entry) != expected_name:
            raise ValueError(
                f"{functions_path}: expected {expected_name}@0x{entry:08x}, "
                f"found {functions.get(entry)!r}"
            )

    call_rows = rows(calls_path, CALL_COLUMNS)
    seen_edges: set[tuple[int, int]] = set()
    for row in call_rows:
        caller = address(row["caller_address"], calls_path)
        callee = address(row["callee_address"], calls_path)
        if functions.get(caller) != row["caller"]:
            raise ValueError(f"{calls_path}: caller name/address mismatch")
        if functions.get(callee) != row["callee"]:
            raise ValueError(f"{calls_path}: callee name/address mismatch")
        edge = (caller, callee)
        if edge in seen_edges:
            raise ValueError(
                f"{calls_path}: duplicate edge 0x{caller:08x}->0x{callee:08x}"
            )
        seen_edges.add(edge)

    assembly_rows = rows(assembly_path, ASM_COLUMNS)
    instruction_counts: Counter[int] = Counter()
    for row in assembly_rows:
        owner = address(row["function_address"], assembly_path)
        address(row["instruction_address"], assembly_path)
        if functions.get(owner) != row["function"]:
            raise ValueError(f"{assembly_path}: function name/address mismatch")
        if not row["bytes"] or len(row["bytes"]) % 2 != 0:
            raise ValueError(f"{assembly_path}: invalid instruction bytes")
        if not row["instruction"].strip():
            raise ValueError(f"{assembly_path}: empty instruction text")
        instruction_counts[owner] += 1

    missing_assembly = sorted(
        set(functions) - set(instruction_counts) - ERASED_FUNCTION_ENTRIES[image]
    )
    if missing_assembly:
        formatted = ", ".join(f"0x{item:08x}" for item in missing_assembly)
        raise ValueError(f"{assembly_path}: functions without ASM: {formatted}")

    return len(functions), len(seen_edges), len(assembly_rows)


def main() -> None:
    totals = Counter()
    summaries: list[str] = []
    for image in ("app", "bootloader"):
        function_count, edge_count, instruction_count = verify_image(image)
        totals.update(
            functions=function_count,
            edges=edge_count,
            instructions=instruction_count,
        )
        summaries.append(
            f"{image}={function_count} functions/{edge_count} edges/"
            f"{instruction_count} instructions"
        )
    print(
        "Ghidra snapshot verified: " + "; ".join(summaries) + "; "
        f"total={totals['functions']} functions/{totals['edges']} edges/"
        f"{totals['instructions']} instructions"
    )


if __name__ == "__main__":
    main()
