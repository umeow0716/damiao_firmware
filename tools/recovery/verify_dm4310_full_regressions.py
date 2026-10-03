#!/usr/bin/env python3
"""Run every persistent DM4310 factory/source differential regression."""

from argparse import ArgumentParser
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
import os
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
REGRESSION_DIR = Path(__file__).with_name("regressions")
MODELS = (
    "dm10010", "dm3507", "dm3507_48v", "dm4310", "dm4310_48v",
    "dm4340", "dm4340_48v", "dm8006", "dm8009",
)


def run_script(script, model):
    environment = os.environ.copy()
    environment["DAMIAO_RECOVERY_MODEL"] = model
    result = subprocess.run(
        [sys.executable, str(script)],
        cwd=ROOT,
        env=environment,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    return script.name, result.returncode, result.stdout


def main():
    parser = ArgumentParser()
    parser.add_argument(
        "--model",
        choices=MODELS,
        default="dm4310",
        help="factory/source target to verify (default: %(default)s)",
    )
    parser.add_argument(
        "-j",
        "--jobs",
        type=int,
        default=min(8, os.cpu_count() or 1),
        help="maximum number of concurrent verifier processes (default: %(default)s)",
    )
    parser.add_argument(
        "--quiet",
        action="store_true",
        help="report pass/fail names without replaying failed verifier output",
    )
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be at least one")

    scripts = sorted(REGRESSION_DIR.glob("dm4310_*_verify.py"))
    if not scripts:
        raise SystemExit(f"no regressions found in {REGRESSION_DIR}")

    failures = []
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = [
            pool.submit(run_script, script, args.model) for script in scripts
        ]
        for future in as_completed(futures):
            name, returncode, output = future.result()
            print(f"{'PASS' if returncode == 0 else 'FAIL'}: {name}", flush=True)
            if returncode:
                failures.append((name, output))

    print(
        f"RESULT {args.model}: "
        f"{len(scripts) - len(failures)}/{len(scripts)} passed"
    )
    if not args.quiet:
        for name, output in failures:
            print(f"\n--- {name} ---\n{output}", end="")
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())
