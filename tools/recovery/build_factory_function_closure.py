#!/usr/bin/env python3
"""Join Ghidra functions, recovery status and direct branch-target coverage."""

from __future__ import annotations

import argparse
import csv
import pathlib
import re
import struct
import subprocess


BRANCH = re.compile(
    r"^\s*([0-9a-f]+):.*\s(bl|blx|b\.w)\s+(?:0x)?([0-9a-f]+)(?:\s|$)",
    re.I,
)
SCATTER_FLASH_START = 0x00028680
SCATTER_FLASH_END = 0x0002AB90
SCATTER_SRAM_START = 0x1FFF8000
SYNTHETIC_FUNCTIONS = [
    {
        "entry": "0x00023852", "end": "0x0002386b", "body_bytes": "0x1a",
        "name": "factory_nvic_enable_irq", "thunk_target": "",
        "callers": "", "callees": "",
    },
    {
        "entry": "0x00020250", "end": "0x00020257", "body_bytes": "0x8",
        "name": "seed_00020250", "thunk_target": "", "callers": "",
        "callees": "00020258,00020368",
    },
    {
        "entry": "0x0002028c", "end": "0x000202ef", "body_bytes": "0x64",
        "name": "factory_scatterload_decompress", "thunk_target": "",
        "callers": "", "callees": "",
    },
    {
        "entry": "0x000202f0", "end": "0x00020309", "body_bytes": "0x1a",
        "name": "factory_scatterload_copy", "thunk_target": "",
        "callers": "", "callees": "",
    },
    {
        "entry": "0x0002030c", "end": "0x00020327", "body_bytes": "0x1c",
        "name": "factory_scatterload_zeroinit", "thunk_target": "",
        "callers": "", "callees": "",
    },
    {
        "entry": "0x00020388", "end": "0x0002038f", "body_bytes": "0x8",
        "name": "Reset_Handler", "thunk_target": "", "callers": "",
        "callees": "00020250,00023554",
    },
    {
        "entry": "0x00020390", "end": "0x00020391", "body_bytes": "0x2",
        "name": "NMI_Handler", "thunk_target": "", "callers": "", "callees": "",
    },
    {
        "entry": "0x00020392", "end": "0x00020393", "body_bytes": "0x2",
        "name": "HardFault_Handler", "thunk_target": "", "callers": "", "callees": "",
    },
    {
        "entry": "0x00020394", "end": "0x00020395", "body_bytes": "0x2",
        "name": "MemManage_Handler", "thunk_target": "", "callers": "", "callees": "",
    },
    {
        "entry": "0x00020396", "end": "0x00020397", "body_bytes": "0x2",
        "name": "BusFault_Handler", "thunk_target": "", "callers": "", "callees": "",
    },
    {
        "entry": "0x00020398", "end": "0x00020399", "body_bytes": "0x2",
        "name": "UsageFault_Handler", "thunk_target": "", "callers": "", "callees": "",
    },
    {
        "entry": "0x0002039a", "end": "0x0002039b", "body_bytes": "0x2",
        "name": "SVC_Handler", "thunk_target": "", "callers": "", "callees": "",
    },
    {
        "entry": "0x0002039c", "end": "0x0002039d", "body_bytes": "0x2",
        "name": "DebugMon_Handler", "thunk_target": "", "callers": "", "callees": "",
    },
    {
        "entry": "0x0002039e", "end": "0x0002039f", "body_bytes": "0x2",
        "name": "PendSV_Handler", "thunk_target": "", "callers": "", "callees": "",
    },
    {
        "entry": "0x000203a0", "end": "0x000203a1", "body_bytes": "0x2",
        "name": "SysTick_Handler", "thunk_target": "", "callers": "", "callees": "",
    },
    {
        "entry": "0x000203a2", "end": "0x000203a3", "body_bytes": "0x2",
        "name": "Default_Handler", "thunk_target": "", "callers": "", "callees": "",
    },
    {
        "entry": "0x00020edc", "end": "0x00020ee5", "body_bytes": "0xa",
        "name": "stream_reader", "thunk_target": "",
        "callers": "00020ee6", "callees": "",
    },
    {
        "entry": "0x00024ea8", "end": "0x00024eb9", "body_bytes": "0x12",
        "name": "factory_debug_uart_character_writer", "thunk_target": "",
        "callers": "00020de0", "callees": "",
    },
    {
        "entry": "0x000219f6", "end": "0x000219ff", "body_bytes": "0xa",
        "name": "thunk_FUN_1fff8640", "thunk_target": "1fff8640",
        "callers": "00026dfc,00026e18,00026eac", "callees": "1fff8640",
    },
    {
        "entry": "0x1fffa3de", "end": "0x1fffa413", "body_bytes": "0x36",
        "name": "factory_wrap_angle", "thunk_target": "",
        "callers": "", "callees": "",
    },
    {
        "entry": "0x1fff9a38", "end": "0x1fff9abf", "body_bytes": "0x88",
        "name": "factory_erase_sector_only", "thunk_target": "",
        "callers": "", "callees": "",
    },
    {
        "entry": "0x1fff9b0e", "end": "0x1fff9b9f", "body_bytes": "0x92",
        "name": "factory_send_variable_length_fd", "thunk_target": "",
        "callers": "", "callees": "",
    },
    {
        "entry": "0x1fff9d5c", "end": "0x1fff9dc7", "body_bytes": "0x6c",
        "name": "factory_clear_loop_states", "thunk_target": "",
        "callers": "", "callees": "",
    },
    {
        "entry": "0x1fff9ea6", "end": "0x1fff9fdd", "body_bytes": "0x138",
        "name": "factory_derive_drive_controller_states", "thunk_target": "",
        "callers": "", "callees": "",
    },
    {
        "entry": "0x1fff8640", "end": "0x1fff8723", "body_bytes": "0xe4",
        "name": "factory_boot_record_writer", "thunk_target": "",
        "callers": "000219f6", "callees": "",
    },
]


def runtime_address(value: int) -> int:
    if SCATTER_FLASH_START <= value < SCATTER_FLASH_END:
        return SCATTER_SRAM_START + value - SCATTER_FLASH_START
    return value


def read_tsv(path: pathlib.Path) -> list[dict[str, str]]:
    with path.open(newline="") as stream:
        return list(csv.DictReader(stream, delimiter="\t"))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--inventory", type=pathlib.Path, required=True)
    parser.add_argument("--status", type=pathlib.Path, required=True)
    parser.add_argument("--sram", type=pathlib.Path, required=True)
    parser.add_argument("--factory", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--objdump", default="arm-none-eabi-objdump")
    args = parser.parse_args()

    inventory = read_tsv(args.inventory)
    known_entries = {row["entry"].lower() for row in inventory}
    inventory.extend(row for row in SYNTHETIC_FUNCTIONS
                     if row["entry"] not in known_entries)
    inventory.sort(key=lambda row: int(row["entry"], 16))
    status = {row["entry"].lower(): row for row in read_tsv(args.status)}
    sram = {row["entry"].lower(): row for row in read_tsv(args.sram)}
    entries = {row["entry"].lower() for row in inventory}
    ranges = [
        (int(row["entry"], 16), int(row["end"], 16)) for row in inventory
    ]

    disassembly = subprocess.run(
        [args.objdump, "-D", "-b", "binary", "-marm", "-Mforce-thumb",
         "--adjust-vma=0x20000", str(args.factory)],
        check=True, capture_output=True, text=True,
    ).stdout
    direct_callers: dict[str, list[str]] = {}
    pointer_refs: dict[str, list[str]] = {}
    uncovered_targets: dict[str, list[str]] = {}
    for line in disassembly.splitlines():
        match = BRANCH.match(line)
        if match is None:
            continue
        caller_value = runtime_address(int(match.group(1), 16))
        mnemonic = match.group(2).lower()
        caller = f"0x{caller_value:08x}"
        target_value = runtime_address(int(match.group(3), 16))
        target = f"0x{target_value:08x}"
        if not (0x00020000 <= int(target, 16) < 0x00040000 or
                0x1FFF8000 <= int(target, 16) < 0x20000000):
            continue
        # A wide unconditional branch within the current body is a basic
        # block edge, not a tail-called function. Keep only cross-body B.W.
        if mnemonic == "b.w" and any(
            start <= caller_value <= end and start <= target_value <= end
            for start, end in ranges
        ):
            continue
        direct_callers.setdefault(target, []).append(caller)
        # Ghidra may model a shared tail or local subroutine as a basic block
        # inside an existing function rather than as a separate Function.
        if target not in entries and not any(
            start <= target_value <= end for start, end in ranges
        ):
            uncovered_targets.setdefault(target, []).append(caller)

    instruction_addresses = {
        runtime_address(int(match.group(1), 16))
        for line in disassembly.splitlines()
        if (match := re.match(r"^\s*([0-9a-f]+):\s", line, re.I))
    }
    factory_data = args.factory.read_bytes()
    for offset in range(0, len(factory_data) - 3, 4):
        word = struct.unpack_from("<I", factory_data, offset)[0]
        if (word & 1) == 0:
            continue
        target_value = runtime_address(word & ~1)
        if target_value not in instruction_addresses:
            continue
        reference = f"0x{0x00020000 + offset:08x}"
        target = f"0x{target_value:08x}"
        pointer_refs.setdefault(target, []).append(reference)
        if target not in entries and not any(
            start <= target_value <= end for start, end in ranges
        ):
            uncovered_targets.setdefault(target, []).append(
                "literal@" + reference)

    fields = [
        "entry", "end", "body_bytes", "factory_name", "callers", "callees",
        "coverage", "source_mapping", "evidence", "direct_branch_callers",
        "literal_pointer_refs",
    ]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, delimiter="\t",
                                lineterminator="\n")
        writer.writeheader()
        for function in inventory:
            entry = function["entry"].lower()
            known = status.get(entry)
            ram = sram.get(entry)
            if known is not None:
                coverage = known["status"]
                mapping = known["proposed_name"]
                evidence = known["source_map"]
                if known["notes"]:
                    evidence += "; " + known["notes"]
            elif ram is not None:
                coverage = ("UNMAPPED" if ram["classification"].startswith(
                    "unrestored_") else "source_built_fixed_entry")
                mapping = ram["classification"]
                evidence = "SRAM_ENTRY_AUDIT.md"
            elif entry == "0x000266e8":
                coverage = "source_behavior_mapped"
                mapping = "platform.c:halt_on_bus_overvoltage"
                evidence = "factory disassembly and fixed VBUS state"
            elif entry == "0x00028cc0":
                coverage = "source_built_fixed_entry"
                mapping = "board_flash:write_boot_record_from_sram"
                evidence = "direct branch closure via veneer 0x219f6"
            elif entry == "0x000219f6":
                coverage = "source_built_fixed_veneer"
                mapping = "veneer to write_boot_record_from_sram@0x1fff8640"
                evidence = "three direct factory tail calls"
            else:
                coverage = "UNMAPPED"
                mapping = ""
                evidence = ""
            writer.writerow({
                "entry": entry,
                "end": function["end"],
                "body_bytes": function["body_bytes"],
                "factory_name": function["name"],
                "callers": function["callers"],
                "callees": function["callees"],
                "coverage": coverage,
                "source_mapping": mapping,
                "evidence": evidence,
                "direct_branch_callers": ",".join(direct_callers.get(entry, [])),
                "literal_pointer_refs": ",".join(pointer_refs.get(entry, [])),
            })

    if uncovered_targets:
        for target, callers in sorted(uncovered_targets.items()):
            print(f"uncovered direct target {target}: {','.join(callers)}")
        raise SystemExit(1)
    print(f"factory function closure: {len(inventory)} functions, "
          "all direct branch targets covered")


if __name__ == "__main__":
    main()
