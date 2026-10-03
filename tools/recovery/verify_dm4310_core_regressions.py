#!/usr/bin/env python3
"""Run the persistent DM43xx factory/source differential regressions."""

from argparse import ArgumentParser
from pathlib import Path
import os
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
MODELS = (
    "dm10010", "dm3507", "dm3507_48v", "dm4310", "dm4310_48v",
    "dm4340", "dm4340_48v", "dm8006", "dm8009",
)
SCRIPTS = (
    "verify_dm4310_peripheral_init.py",
    "regressions/dm4310_uart_irq_verify.py",
    "verify_dm4310_position_irqs.py",
    "regressions/dm4310_adc_retained_pointer_verify.py",
    "verify_dm4310_adc_main_composition.py",
    "regressions/dm4310_mcan_feedback_verify.py",
    "verify_dm4310_irq_main_composition.py",
    "verify_dm4310_uart_main_composition.py",
    "verify_dm4310_boot_main_composition.py",
    "regressions/dm4310_alignment_cluster_verify.py",
    "regressions/dm4310_main_loop_verify.py",
    "regressions/dm4310_observer_slice_verify.py",
    "regressions/dm4310_offset_sweep_cluster_verify.py",
    "regressions/dm4310_motor_id_outer_verify.py",
    "regressions/dm4310_output_calibration_outer_verify.py",
)


def main():
    parser = ArgumentParser()
    parser.add_argument("--model", choices=MODELS,
                        default="dm4310")
    args = parser.parse_args()
    environment = os.environ.copy()
    environment["DAMIAO_RECOVERY_MODEL"] = args.model
    recovery = ROOT / "tools/recovery"
    for script in SCRIPTS:
        print(f"RUN: {script}", flush=True)
        subprocess.run([sys.executable, str(recovery / script)],
                       cwd=ROOT, env=environment, check=True)
    print(
        f"PASS: {len(SCRIPTS)} persistent {args.model} regression scripts"
    )


if __name__ == "__main__":
    main()
