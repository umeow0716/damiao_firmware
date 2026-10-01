#!/usr/bin/env python3
"""Parameterize live DM43xx C address literals for the DM8009 SRAM ABI.

This is a guarded mechanical recovery tool, not part of the firmware build.
It only rewrites C integer expressions that carry an explicit ``UL`` suffix
or are wrapped in ``UINT32_C``.  Plain addresses in audit comments remain the
canonical DM4310 disassembly coordinates.  Files containing hand-authored
per-model literal arrays are intentionally excluded.
"""

from argparse import ArgumentParser
from pathlib import Path
import os
import re
import sys


ROOT = Path(__file__).resolve().parents[2]
REGRESSION_DIR = Path(__file__).with_name("regressions")
sys.path.insert(0, str(REGRESSION_DIR))
os.environ["DAMIAO_RECOVERY_MODEL"] = "dm8009"

from dm4310_model_layout import A  # noqa: E402


SOURCES = (
    "app/src/app_commands.c",
    "app/src/can_protocol.c",
    "app/src/commissioning.c",
    "app/src/debug_console.c",
    "app/src/interrupts.c",
    "app/src/main.c",
    "app/src/motor_control.c",
    "app/src/output_sensor.c",
    "app/src/parameter_protocol.c",
    "app/src/platform.c",
    "app/src/position_sensor.c",
    "app/src/safety.c",
    "app/src/sensor_calibration.c",
    "board/src/board_adc_hc32f448.c",
    "board/src/board_flash_hc32f448.c",
    "board/src/board_mcan_hc32f448.c",
    "board/src/board_position_hc32f448.c",
    "board/src/board_sampling_timer_hc32f448.c",
)

ADDRESS = re.compile(
    r"UINT32_C\((0x1fff[0-9a-f]+)\)|"
    r"(0x1fff[0-9a-f]+UL)\b",
    re.IGNORECASE,
)


def parameterize_line(line):
    if "FACTORY_SRAM_ADDRESS(" in line:
        return line

    def replace(match):
        token = match.group(1) or match.group(2)
        old = int(token.removesuffix("UL").removesuffix("ul"), 16)
        new = A(old)
        if new == old:
            return match.group(0)
        if match.group(1):
            return (
                f"FACTORY_SRAM_ADDRESS(UINT32_C(0x{old:08x}), "
                f"UINT32_C(0x{new:08x}))"
            )
        return (
            f"FACTORY_SRAM_ADDRESS(0x{old:08X}UL, 0x{new:08X}UL)"
        )

    return ADDRESS.sub(replace, line)


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument(
        "--write", action="store_true",
        help="apply the guarded mechanical rewrite (default: report only)",
    )
    args = parser.parse_args()

    changed = []
    for relative in SOURCES:
        path = ROOT / relative
        original = path.read_text()
        updated = "".join(parameterize_line(line)
                          for line in original.splitlines(keepends=True))
        if updated == original:
            continue
        changed.append(relative)
        if args.write:
            path.write_text(updated)
    action = "updated" if args.write else "would update"
    print(f"{action} {len(changed)} files")
    for relative in changed:
        print(relative)


if __name__ == "__main__":
    main()
