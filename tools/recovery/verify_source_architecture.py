#!/usr/bin/env python3
"""Verify target/profile separation without joining the product build."""

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
TARGET_MODELS = {
    "dm10010": "DAMIAO_MODEL_DM10010",
    "dm3507": "DAMIAO_MODEL_DM3507",
    "dm3507_48v": "DAMIAO_MODEL_DM3507_48V",
    "dm4310": "DAMIAO_MODEL_DM4310",
    "dm4310_48v": "DAMIAO_MODEL_DM4310_48V",
    "dm4340": "DAMIAO_MODEL_DM4340",
    "dm4340_48v": "DAMIAO_MODEL_DM4340_48V",
    "dm8006": "DAMIAO_MODEL_DM8006",
    "dm8009": "DAMIAO_MODEL_DM8009",
}
TARGET_LAYOUTS = {
    "dm10010": {"DAMIAO_LAYOUT_SHIFTED_SRAM"},
    "dm3507": set(),
    "dm3507_48v": {"DAMIAO_LAYOUT_SHIFTED_SRAM"},
    "dm4310": set(),
    "dm4310_48v": {"DAMIAO_LAYOUT_SHIFTED_SRAM"},
    "dm4340": set(),
    "dm4340_48v": {"DAMIAO_LAYOUT_SHIFTED_SRAM"},
    "dm8006": {"DAMIAO_LAYOUT_RELOCATED_SRAM"},
    "dm8009": {"DAMIAO_LAYOUT_RELOCATED_SRAM"},
}
MODEL_PATTERN = re.compile(r"\bDAMIAO_MODEL_[A-Z0-9_]+\b")
LAYOUT_PATTERN = re.compile(r"\bDAMIAO_LAYOUT_[A-Z0-9_]+\b")
PRODUCT_ROOTS = ("app", "board", "common", "startup", "third_party")
SOURCE_SUFFIXES = {".c", ".h", ".S"}
OWNED_PRODUCT_ROOTS = ("app", "board", "common", "startup", "linker", "config")
LEGACY_SELECTORS = (
    "DAMIAO_DM4310",
    "DAMIAO_DM8009_LAYOUT",
    "DAMIAO_DM43_48V_LAYOUT",
)
REVERSE_STYLE_PATTERNS = {
    re.compile(r"\b(?:FUN|DAT)_[0-9A-Fa-f]+\b"): "disassembler-generated identifier",
    re.compile(r"\bVector_[0-9]+\b"): "numeric reverse-engineering vector name",
    re.compile(r"\.dm(?:4310|8009)_[A-Za-z0-9_]+"): "model-named shared section",
}


def source_files() -> list[Path]:
    return [
        path
        for directory in PRODUCT_ROOTS
        for path in (ROOT / directory).rglob("*")
        if path.is_file() and path.suffix in SOURCE_SUFFIXES
    ]


def owned_product_files() -> list[Path]:
    suffixes = SOURCE_SUFFIXES | {".ld", ".cmake", ".txt"}
    files = [ROOT / "CMakeLists.txt", ROOT / "Makefile"]
    files.extend(
        path
        for directory in OWNED_PRODUCT_ROOTS
        for path in (ROOT / directory).rglob("*")
        if path.is_file() and path.suffix in suffixes
    )
    return files


def main() -> int:
    errors: list[str] = []
    expected_models = set(TARGET_MODELS.values())

    for target, expected_model in TARGET_MODELS.items():
        path = ROOT / "config" / "targets" / f"{target}.cmake"
        text = path.read_text(encoding="utf-8")
        models = set(MODEL_PATTERN.findall(text))
        layouts = set(LAYOUT_PATTERN.findall(text))
        if models != {expected_model}:
            errors.append(
                f"{path.relative_to(ROOT)}: expected only {expected_model}, "
                f"found {sorted(models)}"
            )
        if layouts != TARGET_LAYOUTS[target]:
            errors.append(
                f"{path.relative_to(ROOT)}: expected layouts "
                f"{sorted(TARGET_LAYOUTS[target])}, found {sorted(layouts)}"
            )

    profile_path = ROOT / "app" / "include" / "app_profile.h"
    profile_models = set(
        MODEL_PATTERN.findall(profile_path.read_text(encoding="utf-8"))
    )
    if profile_models != expected_models:
        errors.append(
            "app/include/app_profile.h: model set differs from target profiles"
        )

    for path in source_files():
        text = path.read_text(encoding="utf-8", errors="replace")
        for selector in LEGACY_SELECTORS:
            if re.search(rf"\b{selector}\b", text):
                errors.append(
                    f"{path.relative_to(ROOT)}: legacy {selector} selector remains"
                )
        if re.search(r"\bDm4310[A-Za-z0-9_]*\b", text):
            errors.append(
                f"{path.relative_to(ROOT)}: model-specific shared C type remains"
            )
        if path != profile_path and MODEL_PATTERN.search(text):
            errors.append(
                f"{path.relative_to(ROOT)}: model selection escaped app_profile.h"
            )

    architecture_files = [
        ROOT / "CMakeLists.txt",
        *(ROOT / "config" / "targets").glob("*.cmake"),
        *(ROOT / "linker").glob("*.ld"),
    ]
    for path in architecture_files:
        text = path.read_text(encoding="utf-8")
        for selector in LEGACY_SELECTORS:
            if re.search(rf"\b{selector}\b", text):
                errors.append(
                    f"{path.relative_to(ROOT)}: legacy {selector} selector remains"
                )

    for path in owned_product_files():
        text = path.read_text(encoding="utf-8", errors="replace")
        for pattern, description in REVERSE_STYLE_PATTERNS.items():
            if pattern.search(text):
                errors.append(f"{path.relative_to(ROOT)}: {description} remains")

    shared_files = (
        "app/include/formatter_arithmetic.h",
        "app/src/softfloat_binary64.c",
        "app/src/decimal_power.c",
        "app/src/extended_arithmetic.c",
        "app/src/extended_arithmetic_core.S",
        "app/src/fixed_decimal.c",
        "linker/hc32f448_v3_app.ld",
    )
    legacy_files = (
        "app/include/dm4310_formatter_arithmetic.h",
        "app/src/dm4310_binary64_extended.c",
        "app/src/dm4310_decimal_power.c",
        "app/src/dm4310_extended_arithmetic.c",
        "app/src/dm4310_extended_core.S",
        "app/src/dm4310_fixed_scaled.c",
        "linker/hc32f448_dm4310_app.ld",
    )
    for relative in shared_files:
        if not (ROOT / relative).is_file():
            errors.append(f"missing shared V3 source: {relative}")
    for relative in legacy_files:
        if (ROOT / relative).exists():
            errors.append(f"legacy model-specific shared path remains: {relative}")

    for relative in ("CMakeLists.txt", "Makefile"):
        text = (ROOT / relative).read_text(encoding="utf-8")
        if "tools/recovery" in text:
            errors.append(
                f"{relative}: recovery-only verification joined the product build"
            )

    if errors:
        for error in errors:
            print(f"FAIL: {error}")
        return 1

    print(
        "source architecture verified: 9 isolated model profiles, shared V3 "
        "implementation, no legacy/model-named shared selectors or reverse-style "
        "identifiers, recovery tools isolated"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
