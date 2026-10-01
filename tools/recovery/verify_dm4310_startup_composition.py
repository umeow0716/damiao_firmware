#!/usr/bin/env python3
"""Run the real Reset/SystemInit/scatter/runtime-to-main composition matrix."""

from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
VERIFY = ROOT / "tools/recovery/regressions/dm4310_startup_copy_verify.py"


def main():
    cases = []
    for clock_source in range(8):
        arguments = ["--real-system", f"--clock-source={clock_source}"]
        if clock_source & 1:
            arguments.append("--masked")
        if clock_source & 2:
            arguments.append("--hrc16")
        if clock_source & 4:
            arguments.append("--internal-pll")
        cases.append(arguments)

    for index, arguments in enumerate(cases):
        print(f"RUN: startup composition {index}: {' '.join(arguments)}",
              flush=True)
        subprocess.run([sys.executable, str(VERIFY), *arguments],
                       cwd=ROOT, check=True)
    print(f"PASS: {len(cases)} real reset-to-main composition cases")


if __name__ == "__main__":
    main()
