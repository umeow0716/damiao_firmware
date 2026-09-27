#!/usr/bin/env python3
"""Run cppcheck and clang-tidy against every project-owned ARM C build."""

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
OWNED_DIRECTORIES = {"app", "board", "bootloader", "common"}
CLANG_CHECKS = ",".join(
    (
        "-*",
        "clang-analyzer-core.*",
        "clang-analyzer-unix.*",
        "clang-analyzer-deadcode.*",
        "clang-analyzer-security.insecureAPI.UncheckedReturn",
        "bugprone-infinite-loop",
        "bugprone-misplaced-widening-cast",
        "bugprone-narrowing-conversions",
        "bugprone-not-null-terminated-result",
        "bugprone-sizeof-expression",
        "bugprone-suspicious-memory-comparison",
        "bugprone-suspicious-memset-usage",
        "bugprone-undefined-memory-manipulation",
    )
)
EXPECTED_CPPCHECK_SUPPRESSIONS = {
    "*:*third_party/HC32F448_DDL_Rev1.3.0/*",
    "unsignedLessThanZero:*board/src/board_mcan_hc32f448.c",
    "duplicateExpression:*bootloader/src/platform.c",
    "redundantInitialization:*app/src/commissioning_math.c",
    "knownConditionTrueFalse:*app/src/parameter_protocol.c",
}


def run(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        cwd=ROOT,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


def require_tool(name: str) -> Path:
    found = shutil.which(name)
    if found is None:
        raise ValueError(
            f"required analyzer {name!r} is not installed; "
            f"install it before running static-audit"
        )
    return Path(found).resolve()


def version_line(tool: Path) -> str:
    result = run([str(tool), "--version"])
    if result.returncode != 0:
        raise ValueError(f"cannot query {tool}: {result.stderr.strip()}")
    return next(
        (line.strip() for line in result.stdout.splitlines() if line.strip()),
        "unknown",
    )


def compile_inventory(path: Path) -> tuple[list[Path], int]:
    commands = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(commands, list):
        raise ValueError(f"{path}: expected a JSON array")
    files: list[Path] = []
    seen: set[Path] = set()
    configuration_count = 0
    for entry in commands:
        source = Path(entry["file"]).resolve()
        try:
            relative = source.relative_to(ROOT)
        except ValueError:
            continue
        if not relative.parts or relative.parts[0] not in OWNED_DIRECTORIES:
            continue
        if source.suffix != ".c":
            continue
        configuration_count += 1
        if source not in seen:
            seen.add(source)
            files.append(source)
    if not files:
        raise ValueError(f"{path}: no project-owned C translation units")
    return files, configuration_count


def gcc_version_defines(gcc: Path) -> list[str]:
    result = run([str(gcc), "-dumpfullversion", "-dumpversion"])
    if result.returncode != 0:
        raise ValueError(f"cannot query {gcc}: {result.stderr.strip()}")
    parts = result.stdout.strip().split(".")
    if not parts or not parts[0].isdigit():
        raise ValueError(f"unexpected GCC version: {result.stdout!r}")
    values = [int(part) if part.isdigit() else 0 for part in parts[:3]]
    values.extend([0] * (3 - len(values)))
    return [
        f"-D__GNUC__={values[0]}",
        f"-D__GNUC_MINOR__={values[1]}",
        f"-D__GNUC_PATCHLEVEL__={values[2]}",
        "-D__ARM_ARCH_7EM__=1",
    ]


def gcc_system_includes(gcc: Path) -> list[Path]:
    include = run([str(gcc), "-print-file-name=include"])
    include_fixed = run([str(gcc), "-print-file-name=include-fixed"])
    sysroot = run([str(gcc), "-print-sysroot"])
    for result in (include, include_fixed, sysroot):
        if result.returncode != 0:
            raise ValueError(f"cannot query ARM GCC include paths: {result.stderr}")
    paths = [
        Path(include.stdout.strip()).resolve(),
        Path(include_fixed.stdout.strip()).resolve(),
        (Path(sysroot.stdout.strip()) / "include").resolve(),
    ]
    missing = [path for path in paths if not path.is_dir()]
    if missing:
        raise ValueError(f"ARM GCC include directories are missing: {missing}")
    return paths


def diagnostic_text(result: subprocess.CompletedProcess[str]) -> str:
    return "\n".join(
        part.strip() for part in (result.stdout, result.stderr) if part.strip()
    )


def run_cppcheck(
    executable: Path,
    compile_database: Path,
    gcc_defines: list[str],
    suppressions: Path,
) -> dict[str, Any]:
    configured_suppressions = {
        line.strip()
        for line in suppressions.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.lstrip().startswith("#")
    }
    if configured_suppressions != EXPECTED_CPPCHECK_SUPPRESSIONS:
        added = configured_suppressions - EXPECTED_CPPCHECK_SUPPRESSIONS
        removed = EXPECTED_CPPCHECK_SUPPRESSIONS - configured_suppressions
        raise ValueError(
            "cppcheck suppression policy changed without an audit update; "
            f"added={sorted(added)}, removed={sorted(removed)}"
        )
    command = [
        str(executable),
        f"--project={compile_database}",
        "--enable=warning,style,performance,portability",
        "--inconclusive",
        "--check-level=exhaustive",
        "--max-ctu-depth=8",
        "--platform=arm32-wchar_t4",
        "--std=c11",
        *gcc_defines,
        "--suppress=missingIncludeSystem",
        f"--suppressions-list={suppressions}",
        "--error-exitcode=2",
        "--quiet",
        f"-i{ROOT / 'third_party/HC32F448_DDL_Rev1.3.0'}",
        f"-i{ROOT / 'tools/arm-gnu-toolchain'}",
    ]
    result = run(command)
    diagnostics = diagnostic_text(result)
    if result.returncode != 0 or diagnostics:
        raise ValueError(
            "cppcheck failed or emitted an unsuppressed diagnostic:\n" +
            (diagnostics or f"exit status {result.returncode}")
        )
    return {
        "version": version_line(executable),
        "diagnostics": 0,
        "check_level": "exhaustive",
        "ctu_depth": 8,
        "platform": "arm32-wchar_t4",
        "enabled_groups": ["warning", "style", "performance", "portability"],
        "documented_suppressions": len(configured_suppressions),
    }


def run_clang_tidy(
    executable: Path,
    compile_database: Path,
    files: list[Path],
    include_paths: list[Path],
) -> dict[str, Any]:
    header_filter = "^" + re.escape(str(ROOT)) + "/(app|board|bootloader|common)/"
    command = [
        str(executable),
        "-p",
        str(compile_database.parent),
        *[str(path) for path in files],
        f"--checks={CLANG_CHECKS}",
        "--warnings-as-errors=*",
        f"--header-filter={header_filter}",
        "--quiet",
        "--extra-arg=-w",
    ]
    for include_path in include_paths:
        command.extend(("--extra-arg=-isystem", f"--extra-arg={include_path}"))
    result = run(command)
    output = diagnostic_text(result)
    diagnostics = [
        line for line in output.splitlines()
        if re.search(r"\b(?:warning|error):", line)
    ]
    if result.returncode != 0 or diagnostics:
        detail = "\n".join(diagnostics) if diagnostics else output
        raise ValueError(
            "clang-tidy failed or emitted a diagnostic:\n" +
            (detail or f"exit status {result.returncode}")
        )
    enabled_checks = [item for item in CLANG_CHECKS.split(",") if item and item != "-*"]
    return {
        "version": version_line(executable),
        "diagnostics": 0,
        "warnings_as_errors": True,
        "enabled_check_patterns": enabled_checks,
    }


def render_markdown(report: dict[str, Any]) -> str:
    cppcheck = report["cppcheck"]
    clang_tidy = report["clang_tidy"]
    return "\n".join(
        [
            "# Supplemental static analysis",
            "",
            "Result: **PASS**. Both analyzers consumed the ARM compile database "
            "and reported zero unsuppressed project diagnostics.",
            "",
            "| Analyzer | Version | Unique C files | Build configurations | Diagnostics |",
            "|---|---|---:|---:|---:|",
            f"| cppcheck | `{cppcheck['version']}` | {report['unique_c_files']} | "
            f"{report['build_configurations']} | {cppcheck['diagnostics']} |",
            f"| clang-tidy | `{clang_tidy['version']}` | {report['unique_c_files']} | "
            f"{report['build_configurations']} | {clang_tidy['diagnostics']} |",
            "",
            "Cppcheck uses exhaustive value-flow/CTU analysis with the 32-bit "
            "ARM unsigned-char data model. Clang-tidy runs Clang Static Analyzer "
            "core/unix/dead-store checks plus selected bugprone checks; every "
            "reported warning is fatal.",
            "",
            f"Cppcheck has {cppcheck['documented_suppressions']} documented, "
            "narrow suppressions. They cover the pinned third-party DDL/CMSIS "
            "tree and four reviewed embedded-analysis limitations: volatile "
            "IRQ time, DWT hardware time, and intentional IEEE-754 union "
            "type-punning/protocol range reasoning.",
            "",
            "Third-party translation units are excluded as audit targets, but "
            "their headers are still parsed while checking project-owned code. "
            "The existing ARM GCC `-Werror` and `-fanalyzer` build remains an "
            "independent gate.",
            "",
        ]
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build")
    parser.add_argument(
        "--gcc",
        type=Path,
        default=ROOT / "tools/arm-gnu-toolchain/bin/arm-none-eabi-gcc",
    )
    parser.add_argument(
        "--suppressions",
        type=Path,
        default=ROOT / "config/cppcheck_suppressions.txt",
    )
    parser.add_argument("--output-json", type=Path, required=True)
    parser.add_argument("--output-markdown", type=Path, required=True)
    args = parser.parse_args()

    try:
        compile_database = (args.build_dir / "compile_commands.json").resolve()
        files, configuration_count = compile_inventory(compile_database)
        cppcheck_executable = require_tool("cppcheck")
        clang_tidy_executable = require_tool("clang-tidy")
        gcc = args.gcc.resolve()
        cppcheck = run_cppcheck(
            cppcheck_executable,
            compile_database,
            gcc_version_defines(gcc),
            args.suppressions.resolve(),
        )
        clang_tidy = run_clang_tidy(
            clang_tidy_executable,
            compile_database,
            files,
            gcc_system_includes(gcc),
        )
    except (OSError, ValueError, json.JSONDecodeError) as error:
        raise SystemExit(f"supplemental static analysis failed: {error}") from error

    report = {
        "schema": 1,
        "result": "PASS",
        "unique_c_files": len(files),
        "build_configurations": configuration_count,
        "cppcheck": cppcheck,
        "clang_tidy": clang_tidy,
    }
    args.output_json.parent.mkdir(parents=True, exist_ok=True)
    args.output_markdown.parent.mkdir(parents=True, exist_ok=True)
    args.output_json.write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    args.output_markdown.write_text(render_markdown(report), encoding="utf-8")
    print(
        "supplemental static analysis passed: "
        f"{len(files)} project-owned C files/{configuration_count} build "
        "configurations; cppcheck=0; clang-tidy=0"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
