#!/usr/bin/env python3
"""Fail-closed whole-program static checks for the source firmware targets."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shlex
import subprocess
from dataclasses import dataclass
from pathlib import Path


STACK_BUDGET = 4096
SINGLE_FRAME_LIMIT = 1024
# 8 core registers + 18 floating-point words + one alignment word.
CORTEX_M4F_EXCEPTION_FRAME = 108
# One final basic exception frame keeps HardFault/default handling from
# consuming the ordinary nested-IRQ margin.
CORTEX_M4_FAULT_FRAME = 36
REQUIRED_ANALYSIS_FLAGS = {
    "-fanalyzer",
    "-fstack-usage",
    "-Werror",
    "-Wconversion",
    "-Wformat=2",
    "-Wmissing-prototypes",
    "-Wstack-usage=1024",
    "-Wvla",
}
FORBIDDEN_RUNTIME_SYMBOLS = {
    "calloc",
    "free",
    "gets",
    "longjmp",
    "malloc",
    "printf",
    "realloc",
    "setjmp",
    "sprintf",
    "strcat",
    "strcpy",
    "system",
    "vsprintf",
}
EXPECTED_INDIRECT_CALLS = {
    "app": {"Reset_Handler": 2},
    "bootloader": {"Reset_Handler": 2, "app_image_jump": 1},
}
EXPECTED_LITERAL_VENEERS = {
    "__erase_and_program_from_sram_veneer": "erase_and_program_from_sram",
}


@dataclass(frozen=True)
class StackEntry:
    source: str
    function: str
    size: int
    qualifier: str


def run(*command: str) -> str:
    result = subprocess.run(
        command,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    return result.stdout


def parse_init_array_targets(
    readelf: str,
    elf: Path,
    symbols: dict[str, tuple[int, int]],
) -> list[str]:
    symbols_by_address = {
        address & ~1: name for name, (address, _size) in symbols.items()
    }
    targets: list[str] = []
    for section in (".preinit_array", ".init_array"):
        result = subprocess.run(
            (readelf, "-x", section, str(elf)),
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if result.returncode != 0:
            if "was not dumped because it does not exist" in result.stderr:
                continue
            raise ValueError(f"could not inspect {section} in {elf}: {result.stderr}")
        raw = bytearray()
        for line in result.stdout.splitlines():
            match = re.match(
                r"^\s*0x[0-9a-fA-F]+\s+((?:[0-9a-fA-F]{8}\s*)+)", line
            )
            if match is not None:
                for word in match.group(1).split():
                    raw.extend(bytes.fromhex(word))
        if len(raw) % 4 != 0:
            raise ValueError(f"{section} size is not a multiple of four bytes")
        for offset in range(0, len(raw), 4):
            address = int.from_bytes(raw[offset:offset + 4], "little") & ~1
            target = symbols_by_address.get(address)
            if target is None:
                raise ValueError(
                    f"cannot resolve {section} target 0x{address:08x}"
                )
            targets.append(target)
    return targets


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(65536), b""):
            digest.update(block)
    return digest.hexdigest()


def parse_stack_usage(paths: list[Path]) -> list[StackEntry]:
    entries: list[StackEntry] = []
    location_pattern = re.compile(r"^(.*):(\d+):(\d+):(.+)$")
    for path in paths:
        for line in path.read_text(encoding="utf-8").splitlines():
            fields = line.split("\t")
            if len(fields) != 3:
                raise ValueError(f"unrecognized stack-usage row in {path}: {line}")
            location, size_text, qualifier = fields
            match = location_pattern.fullmatch(location)
            if match is None:
                raise ValueError(f"unrecognized stack-usage location: {location}")
            entries.append(
                StackEntry(
                    source=match.group(1),
                    function=match.group(4),
                    size=int(size_text),
                    qualifier=qualifier,
                )
            )
    return entries


def verify_compile_commands(build_dir: Path, source_root: Path) -> dict[str, int]:
    commands_path = build_dir / "compile_commands.json"
    commands = json.loads(commands_path.read_text(encoding="utf-8"))
    c_commands = []
    owned = set()
    vendor = set()
    for entry in commands:
        file_path = Path(entry["file"]).resolve()
        if file_path.suffix.lower() != ".c":
            continue
        arguments = entry.get("arguments")
        if arguments is None:
            arguments = shlex.split(entry["command"])
        missing_flags = sorted(REQUIRED_ANALYSIS_FLAGS.difference(arguments))
        if missing_flags:
            raise ValueError(
                f"static-analysis flags missing for {file_path}: "
                f"{', '.join(missing_flags)}"
            )
        c_commands.append(file_path)
        relative = file_path.relative_to(source_root)
        if relative.parts[0] == "third_party":
            vendor.add(relative.as_posix())
        else:
            owned.add(relative.as_posix())
    if not c_commands:
        raise ValueError("static build contains no C translation units")
    expected_owned = {
        path.relative_to(source_root).as_posix()
        for directory in ("app", "board", "bootloader", "common")
        for path in (source_root / directory).rglob("*.c")
    }
    missing_owned = sorted(expected_owned.difference(owned))
    if missing_owned:
        raise ValueError(
            "project-owned C sources missing from analyzed targets: "
            + ", ".join(missing_owned)
        )
    return {
        "compile_commands": len(c_commands),
        "owned_translation_units": len(owned),
        "owned_sources_discovered": len(expected_owned),
        "vendor_translation_units": len(vendor),
    }


def parse_symbols(nm: str, elf: Path) -> dict[str, tuple[int, int]]:
    result: dict[str, tuple[int, int]] = {}
    output = run(nm, "-n", "-S", "--defined-only", str(elf))
    for line in output.splitlines():
        fields = line.split()
        if len(fields) != 4:
            continue
        address, size, kind, name = fields
        if kind.lower() not in {"t", "w"}:
            continue
        value = (int(address, 16), int(size, 16))
        if name in result and result[name] != value:
            raise ValueError(
                f"ambiguous text symbol {name}: {result[name]} and {value}"
            )
        result[name] = value
    return result


def parse_cfi_frames(readelf: str, elf: Path) -> dict[int, int]:
    output = run(readelf, "--debug-dump=frames", str(elf))
    frames: dict[int, int] = {}
    current_start: int | None = None
    current_max = 0
    for line in output.splitlines():
        match = re.search(r"FDE cie=.* pc=([0-9a-fA-F]+)\.\.([0-9a-fA-F]+)", line)
        if match is not None:
            if current_start is not None:
                frames[current_start] = max(frames.get(current_start, 0), current_max)
            current_start = int(match.group(1), 16)
            current_max = 0
            continue
        match = re.search(r"DW_CFA_def_cfa_offset:\s*(\d+)", line)
        if match is not None and current_start is not None:
            current_max = max(current_max, int(match.group(1)))
    if current_start is not None:
        frames[current_start] = max(frames.get(current_start, 0), current_max)
    return frames


def register_list_size(registers: str, bytes_per_register: int) -> int:
    count = 0
    for item in registers.split(","):
        item = item.strip()
        if "-" not in item:
            count += 1
            continue
        first, last = item.split("-", maxsplit=1)
        first_number = re.search(r"\d+", first)
        last_number = re.search(r"\d+", last)
        if first_number is None or last_number is None:
            raise ValueError(f"unrecognized register range: {item}")
        count += int(last_number.group()) - int(first_number.group()) + 1
    return count * bytes_per_register


def instruction_stack_decrement(mnemonic: str, operands: str) -> int:
    operands = operands.split("@", maxsplit=1)[0].strip()
    register_list = re.search(r"\{([^}]+)\}", operands)
    if (mnemonic.startswith("push") or mnemonic == "stmdb") and register_list is not None:
        if mnemonic == "stmdb" and not operands.startswith("sp!"):
            return 0
        return register_list_size(register_list.group(1), 4)
    if (mnemonic.startswith("vpush") or mnemonic == "vstmdb") and register_list is not None:
        if mnemonic == "vstmdb" and not operands.startswith("sp!"):
            return 0
        return register_list_size(register_list.group(1), 8)
    if mnemonic.startswith("sub"):
        match = re.match(r"sp,\s*(?:sp,\s*)?#(0x[0-9a-fA-F]+|\d+)", operands)
        if match is not None:
            return int(match.group(1), 0)
    if mnemonic.startswith("str"):
        match = re.search(r"\[sp,\s*#-(0x[0-9a-fA-F]+|\d+)\]!", operands)
        if match is not None:
            return int(match.group(1), 0)
    return 0


def parse_disassembly(
    objdump: str,
    elf: Path,
    symbols: dict[str, tuple[int, int]],
) -> tuple[dict[str, set[str]], dict[str, int], dict[str, str], dict[str, int]]:
    output = run(objdump, "-d", str(elf))
    calls: dict[str, set[str]] = {}
    indirect: dict[str, int] = {}
    literal_veneers: dict[str, str] = {}
    instruction_frames: dict[str, int] = {}
    symbols_by_address = {
        address & ~1: name for name, (address, _size) in symbols.items()
    }
    current: str | None = None
    pending_veneer: str | None = None
    header = re.compile(r"^[0-9a-fA-F]+ <([^>]+)>:$")
    direct_call = re.compile(r"\bbl(?:\.w)?\s+[0-9a-fA-F]+\s+<([^>]+)>")
    tail_call = re.compile(r"\bb(?:\.n|\.w)?\s+[0-9a-fA-F]+\s+<([^>+\-]+)>")
    instruction = re.compile(
        r"^\s*[0-9a-fA-F]+:\s+(?:[0-9a-fA-F]{4}\s+)+"
        r"([a-z][a-z0-9.]*)\s*(.*)$"
    )
    literal_word = re.compile(
        r"^\s*[0-9a-fA-F]+:\s+[0-9a-fA-F]+\s+\.word\s+0x([0-9a-fA-F]+)"
    )
    for line in output.splitlines():
        match = header.match(line)
        if match is not None:
            current = match.group(1)
            calls.setdefault(current, set())
            instruction_frames.setdefault(current, 0)
            pending_veneer = None
            continue
        if current is None:
            continue
        match = literal_word.match(line)
        if match is not None and pending_veneer is not None:
            target_address = int(match.group(1), 16) & ~1
            target = symbols_by_address.get(target_address)
            if target is None:
                raise ValueError(
                    f"cannot resolve literal veneer {pending_veneer} target "
                    f"0x{target_address:08x}"
                )
            calls[pending_veneer].add(target)
            literal_veneers[pending_veneer] = target
            pending_veneer = None
            continue
        decoded = instruction.match(line)
        if decoded is not None:
            mnemonic = decoded.group(1)
            operands = decoded.group(2)
            instruction_frames[current] += instruction_stack_decrement(
                mnemonic, operands
            )
            if mnemonic.startswith("ldr") and re.match(
                r"pc,\s*\[pc(?:,\s*#0)?\]", operands
            ):
                pending_veneer = current
            elif (
                mnemonic == "bx"
                and re.fullmatch(r"r(?:1[0-3]|[0-9])", operands)
            ):
                indirect[current] = indirect.get(current, 0) + 1
            elif mnemonic.startswith("ldr") and operands.startswith("pc, ["):
                if not operands.startswith("pc, [sp"):
                    indirect[current] = indirect.get(current, 0) + 1
            elif mnemonic == "mov" and operands.startswith("pc,"):
                indirect[current] = indirect.get(current, 0) + 1
        if re.search(r"\bblx\s+r(?:1[0-5]|[0-9])\b", line):
            indirect[current] = indirect.get(current, 0) + 1
        match = direct_call.search(line)
        if match is not None:
            calls[current].add(match.group(1))
            continue
        match = tail_call.search(line)
        if match is not None and match.group(1) != current:
            calls[current].add(match.group(1))
    if pending_veneer is not None:
        raise ValueError(f"literal veneer {pending_veneer} has no target word")
    return calls, indirect, literal_veneers, instruction_frames


def find_cycles(calls: dict[str, set[str]]) -> list[list[str]]:
    state: dict[str, int] = {}
    stack: list[str] = []
    cycles: list[list[str]] = []

    def visit(function: str) -> None:
        mark = state.get(function, 0)
        if mark == 2:
            return
        if mark == 1:
            start = stack.index(function)
            cycles.append(stack[start:] + [function])
            return
        state[function] = 1
        stack.append(function)
        for callee in calls.get(function, set()):
            if callee in calls:
                visit(callee)
        stack.pop()
        state[function] = 2

    for function in calls:
        if state.get(function, 0) == 0:
            visit(function)
    return cycles


def longest_stack_path(
    root: str,
    calls: dict[str, set[str]],
    frames: dict[str, int],
) -> tuple[int, list[str]]:
    cache: dict[str, tuple[int, list[str]]] = {}

    def visit(function: str) -> tuple[int, list[str]]:
        if function in cache:
            return cache[function]
        best_size = 0
        best_path: list[str] = []
        for callee in calls.get(function, set()):
            if callee == function:
                continue
            size, path = visit(callee) if callee in calls else (0, [callee])
            if size > best_size:
                best_size = size
                best_path = path
        result = (frames.get(function, 0) + best_size, [function] + best_path)
        cache[function] = result
        return result

    return visit(root)


def verify_link_inputs(build_dir: Path, target: str) -> None:
    link_path = build_dir / "CMakeFiles" / f"{target}.dir" / "link.txt"
    link_text = link_path.read_text(encoding="utf-8")
    forbidden = ("reference/", "bins/", "exact/", ".bin", ".enc")
    for token in forbidden:
        if token in link_text:
            raise ValueError(f"{target} link command contains forbidden input {token!r}")


def audit_image(
    *,
    kind: str,
    elf: Path,
    binary: Path,
    normal_binary: Path,
    build_dir: Path,
    nm: str,
    objdump: str,
    readelf: str,
) -> dict[str, object]:
    target = "dm4310_app" if kind == "app" else "dm4310_bootloader"
    verify_link_inputs(build_dir, target)
    symbols = parse_symbols(nm, elf)
    undefined = [
        line.strip() for line in run(nm, "-u", str(elf)).splitlines()
        if line.strip()
    ]
    if undefined:
        raise ValueError(f"{kind} contains undefined symbols: {', '.join(undefined)}")
    forbidden = sorted(FORBIDDEN_RUNTIME_SYMBOLS.intersection(symbols))
    if forbidden:
        raise ValueError(f"{kind} links forbidden runtime APIs: {', '.join(forbidden)}")

    su_paths = sorted((build_dir / "CMakeFiles" / f"{target}.dir").rglob("*.su"))
    su_paths += sorted((build_dir / "CMakeFiles" / "dm4310_common.dir").rglob("*.su"))
    stack_entries = parse_stack_usage(su_paths)
    non_static = [entry for entry in stack_entries if entry.qualifier != "static"]
    if non_static:
        names = ", ".join(entry.function for entry in non_static[:8])
        raise ValueError(f"{kind} has dynamic or unbounded stack use: {names}")
    oversized = [entry for entry in stack_entries if entry.size > SINGLE_FRAME_LIMIT]
    if oversized:
        names = ", ".join(f"{entry.function}={entry.size}" for entry in oversized)
        raise ValueError(f"{kind} exceeds the single-frame stack limit: {names}")

    calls, indirect, literal_veneers, instruction_frames = parse_disassembly(
        objdump, elf, symbols
    )
    init_array_targets = parse_init_array_targets(readelf, elf, symbols)
    calls["Reset_Handler"].update(init_array_targets)
    cycles = find_cycles(calls)
    if cycles:
        rendered = "; ".join(" -> ".join(cycle) for cycle in cycles)
        raise ValueError(f"{kind} direct call graph is recursive: {rendered}")
    if indirect != EXPECTED_INDIRECT_CALLS[kind]:
        raise ValueError(
            f"{kind} indirect-call inventory changed: "
            f"actual={indirect}, expected={EXPECTED_INDIRECT_CALLS[kind]}"
        )
    if literal_veneers != EXPECTED_LITERAL_VENEERS:
        raise ValueError(
            f"{kind} linker-veneer inventory changed: "
            f"actual={literal_veneers}, expected={EXPECTED_LITERAL_VENEERS}"
        )

    cfi_by_address = parse_cfi_frames(readelf, elf)
    stack_usage_by_function: dict[str, int] = {}
    for entry in stack_entries:
        stack_usage_by_function[entry.function] = max(
            stack_usage_by_function.get(entry.function, 0), entry.size
        )
    frames = {
        name: max(
            cfi_by_address.get(address, 0),
            stack_usage_by_function.get(name, 0),
            instruction_frames.get(name, 0),
        )
        for name, (address, _size) in symbols.items()
    }
    roots = ["Reset_Handler", "main", "Default_Handler"]
    if kind == "app":
        roots += [f"IRQ{index:03d}_Handler" for index in range(5)]
    else:
        roots += ["SysTick_Handler"]
    missing_roots = [root for root in roots if root not in calls]
    if missing_roots:
        raise ValueError(f"{kind} stack roots missing: {', '.join(missing_roots)}")
    root_paths = {
        root: longest_stack_path(root, calls, frames) for root in roots
    }

    if kind == "app":
        nested_stack = max(
            root_paths["Reset_Handler"][0], root_paths["main"][0]
        )
        for index in reversed(range(5)):
            nested_stack += root_paths[f"IRQ{index:03d}_Handler"][0]
            nested_stack += CORTEX_M4F_EXCEPTION_FRAME
    else:
        nested_stack = max(
            root_paths["Reset_Handler"][0], root_paths["main"][0]
        )
        nested_stack += root_paths["SysTick_Handler"][0]
        nested_stack += CORTEX_M4F_EXCEPTION_FRAME
    nested_stack += root_paths["Default_Handler"][0]
    nested_stack += CORTEX_M4_FAULT_FRAME
    if nested_stack > STACK_BUDGET:
        raise ValueError(
            f"{kind} conservative nested stack {nested_stack} exceeds "
            f"the {STACK_BUDGET}-byte linker reservation"
        )

    image_hash = sha256(binary)
    normal_hash = sha256(normal_binary)
    if image_hash != normal_hash:
        raise ValueError(
            f"{kind} analyzer build differs from normal build: "
            f"{image_hash} != {normal_hash}"
        )
    largest = sorted(stack_entries, key=lambda entry: entry.size, reverse=True)[:10]
    return {
        "elf": str(elf),
        "binary": str(binary),
        "sha256": image_hash,
        "stack_usage_files": len(su_paths),
        "stack_functions": len(stack_entries),
        "largest_static_frames": [
            {
                "function": entry.function,
                "bytes": entry.size,
                "source": entry.source,
            }
            for entry in largest
        ],
        "direct_call_functions": len(calls),
        "direct_recursion_cycles": 0,
        "indirect_calls": indirect,
        "literal_veneers": literal_veneers,
        "init_array_targets": init_array_targets,
        "cfi_functions": sum(
            1
            for name, (address, _size) in symbols.items()
            if cfi_by_address.get(address, 0) != 0
        ),
        "stack_frame_functions": sum(1 for size in frames.values() if size != 0),
        "exception_frame_bytes": CORTEX_M4F_EXCEPTION_FRAME,
        "fault_reserve_bytes": CORTEX_M4_FAULT_FRAME,
        "root_stack_paths": {
            root: {"bytes": size, "path": path}
            for root, (size, path) in root_paths.items()
        },
        "conservative_nested_stack_bytes": nested_stack,
        "stack_budget_bytes": STACK_BUDGET,
        "stack_margin_bytes": STACK_BUDGET - nested_stack,
        "forbidden_runtime_symbols": [],
        "undefined_symbols": [],
    }


def write_markdown(path: Path, report: dict[str, object]) -> None:
    lines = [
        "# Whole-program static analysis",
        "",
        "This report is generated by `make static-audit`.",
        "",
        "## Compiler coverage",
        "",
        f"- C compile commands: {report['compiler']['compile_commands']}",
        f"- project-owned translation units: {report['compiler']['owned_translation_units']}",
        f"- project-owned C files discovered: "
        f"{report['compiler']['owned_sources_discovered']} (all analyzed)",
        f"- vendor translation units: {report['compiler']['vendor_translation_units']}",
        "- strict embedded-C warnings, GCC `-fanalyzer`, `-Werror`, and "
        "`-fstack-usage`: passed",
        "",
    ]
    for kind in ("app", "bootloader"):
        image = report[kind]
        lines += [
            f"## {kind}",
            "",
            f"- SHA-256: `{image['sha256']}`",
            f"- direct-call functions: {image['direct_call_functions']}",
            "- direct recursion cycles: 0",
            f"- conservative nested stack: {image['conservative_nested_stack_bytes']} / "
            f"{image['stack_budget_bytes']} bytes",
            f"- stack margin: {image['stack_margin_bytes']} bytes",
            f"- exception accounting: {image['exception_frame_bytes']} bytes per "
            f"nested M4F IRQ + {image['fault_reserve_bytes']} bytes final fault reserve",
            f"- indirect calls: `{json.dumps(image['indirect_calls'], sort_keys=True)}`",
            f"- resolved linker veneers: "
            f"`{json.dumps(image['literal_veneers'], sort_keys=True)}`",
            f"- preinit/init-array targets: "
            f"`{json.dumps(image['init_array_targets'])}`",
            "- undefined symbols: none",
            "- forbidden hosted/runtime APIs: none",
            "",
            "Largest compiler-reported frames:",
            "",
            "| Function | Bytes | Source |",
            "|---|---:|---|",
        ]
        for frame in image["largest_static_frames"]:
            source = Path(frame["source"]).name
            lines.append(f"| `{frame['function']}` | {frame['bytes']} | `{source}` |")
        lines.append("")
    lines += [
        "## Boundary",
        "",
        "This proves the checked source/ELF properties and conservative software stack "
        "bounds. It cannot prove physical ADC scaling, peripheral timing, PWM polarity, "
        "electrical phase order, Flash behavior under power loss, or loader handoff on a "
        "real HC32F448 target.",
        "",
    ]
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--normal-build-dir", type=Path, required=True)
    parser.add_argument("--nm", required=True)
    parser.add_argument("--objdump", required=True)
    parser.add_argument("--readelf", required=True)
    parser.add_argument("--output-json", type=Path, required=True)
    parser.add_argument("--output-markdown", type=Path, required=True)
    args = parser.parse_args()

    source_root = args.source_root.resolve()
    build_dir = args.build_dir.resolve()
    normal_build_dir = args.normal_build_dir.resolve()
    compiler = verify_compile_commands(build_dir, source_root)
    report: dict[str, object] = {"compiler": compiler}
    for kind, target in (
        ("app", "dm4310_app"),
        ("bootloader", "dm4310_bootloader"),
    ):
        report[kind] = audit_image(
            kind=kind,
            elf=build_dir / f"{target}.elf",
            binary=build_dir / f"{target}.bin",
            normal_binary=normal_build_dir / f"{target}.bin",
            build_dir=build_dir,
            nm=args.nm,
            objdump=args.objdump,
            readelf=args.readelf,
        )

    args.output_json.parent.mkdir(parents=True, exist_ok=True)
    args.output_json.write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    write_markdown(args.output_markdown, report)
    print(
        "whole-program static analysis passed: "
        f"{compiler['owned_translation_units']} owned translation units; "
        f"APP stack {report['app']['conservative_nested_stack_bytes']}/4096; "
        "no recursion or unexpected indirect calls"
    )


if __name__ == "__main__":
    main()
