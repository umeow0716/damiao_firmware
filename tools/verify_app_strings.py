#!/usr/bin/env python3
"""Verify the reconstructed APP's externally visible factory text."""

from __future__ import annotations

import argparse
from pathlib import Path


REQUIRED = (
    b"Error,O-sensor need calibration!\r\n",
    b"O-sensor fail!Max=%.4f\r\n",
    b"u=%.4f v=%.4f  w=%.4f c=%.4f\r\n",
    b"Sensor U broken!U=%.4f\r\n",
    b"Sensor V broken!V=%.4f\r\n",
    b" Commands:\n\r",
    b" m - Motor Mode\n\r",
    b" s - Setup Mode\n\r",
    b" esc - Exit to Menu\n\r",
    b"\n\r Entering Motor Mode \n\r",
    b"CAN Error 1\n\r",
    b"CAN Error 2\n\r",
    b"W  | W  | V  | V  | U  | U ",
    b"MOSFET ERROR,  WH | WL | VH | VL | UH | UL\r\n",
    b"               %s\r\n",
    b"error!/r/n",
    b"V_BUS= %.4f Over Voltage!!!\r\n",
    b"DMBOT Motor Driver",
    b"--V2.0",
    b"--V3.0",
    b"--V4.0",
    b"--V1.0",
    b"\n\r Debug Info:\n\r",
    b"Firmware Version: %d\r\n",
    b"Sub Version: %03d\r\n",
    b"Imax: %f\r\n",
    b" I_U Offset:     %.4f\r\n",
    b" I_V Offset:     %.4f\r\n",
    b" I_W Offset:     %.4f\r\n",
    b" Position Sensor Electrical Offset:   %.4f\n\r",
    b" Mechanical Offset:   %.4f\n\r",
    b" Output Position:  %.4f\n\r",
    b" CAN ID:     0x%03x\n\r",
    b" MASTER ID:  0x%03x\n\r",
    b" CAN Baud: %dKbps\n\r",
    b" CAN Baud: %.2fMbps\n\r",
    b"\n\r Motor Info:\n\r",
    b" Rs  = %.4f m\xA6\xB8\n\r",
    b" Ls  = %.4f \xA6\xCC" b"H\n\r",
    b" \xA6\xB7" b"f = %.4f Wb\n\r",
    b"V_BUS=%.4f\r\n",
    b"\n\r Control Mode : \r\n",
    b"1:MIT Mode <----\n\r",
    b"1:MIT Mode\n\r",
    b"2:position-speed cascade Mode <----\n\r",
    b"2:position-speed cascade Mode\n\r",
    b"3:speed Mode <----\n\r",
    b"3:speed Mode\n\r",
    b"4:Hybrid control Mode <----\n\r",
    b"4:Hybrid control Mode\n\r",
    b"The key verification failed. This is a duplicate!\n",
)

FORBIDDEN = (
    b"source development build",
    b"Device key verification failed; recovery only.",
    b"commissioning blocked:",
    b"commissioning failed:",
    b"Power-stage state:",
)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("binary", type=Path)
    args = parser.parse_args()

    image = args.binary.read_bytes()
    missing = [text for text in REQUIRED if text not in image]
    unexpected = [text for text in FORBIDDEN if text in image]
    if missing or unexpected:
        for text in missing:
            print(f"missing factory string: {text!r}")
        for text in unexpected:
            print(f"unexpected source-only string: {text!r}")
        return 1

    print(f"factory APP strings verified: {len(REQUIRED)} sequences")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
