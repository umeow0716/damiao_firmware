#!/usr/bin/env python3
"""Execute the actual ARM CAN FD sender. No hardware and no normal build hook."""
from __future__ import annotations

import argparse
import struct
from pathlib import Path

from elftools.elf.elffile import ELFFile
from unicorn import UC_HOOK_CODE
import unicorn.arm_const as arm

from verify_feedback_variants import make_machine, symbol_addresses, RETURN_ADDRESS, STACK_TOP

MCAN = 0x40029000
MESSAGE_RAM = 0x4002B000
PAYLOAD = 0x20006000  # Outside the enlarged reliable-capture BSS.


def run_case(path: Path, *, length: int = 64, cccr: int = 0x300,
             psr: int = 0, txfqs: int = 2 << 16,
             pointer: int = PAYLOAD, identifier: int = 0x6F2,
             expected: bool = True, initial_mask: int = 0) -> int:
    with path.open("rb") as stream:
        elf = ELFFile(stream)
        symbols = symbol_addresses(elf)
        machine = make_machine(elf, mclass=True)
    machine.mem_map(MCAN, 0x1000)
    machine.mem_map(MESSAGE_RAM, 0x1000)
    machine.mem_write(MESSAGE_RAM, bytes([0xA5]) * 0x1000)
    payload = bytes(range(64))
    machine.mem_write(PAYLOAD, payload)
    for offset, value in ((0x18, cccr), (0x44, psr), (0xC4, txfqs)):
        machine.mem_write(MCAN + offset, struct.pack("<I", value))
    machine.reg_write(arm.UC_ARM_REG_R0, identifier)
    machine.reg_write(arm.UC_ARM_REG_R1, pointer)
    machine.reg_write(arm.UC_ARM_REG_R2, length)
    machine.reg_write(arm.UC_ARM_REG_PRIMASK, initial_mask)
    instructions = [0]

    def count(*_args):
        instructions[0] += 1

    machine.hook_add(UC_HOOK_CODE, count)
    machine.emu_start(symbols["board_mcan_try_send_identification"] | 1,
                      RETURN_ADDRESS & ~1, count=10000)
    assert machine.reg_read(arm.UC_ARM_REG_PC) == RETURN_ADDRESS & ~1
    assert bool(machine.reg_read(arm.UC_ARM_REG_R0)) is expected
    assert machine.reg_read(arm.UC_ARM_REG_PRIMASK) == initial_mask
    ram = bytes(machine.mem_read(MESSAGE_RAM, 0x1000))
    if expected:
        index = (txfqs >> 16) & 31
        offset = 0x328 + index * 72
        header = struct.pack("<II", identifier << 18,
                             0x3F0000 if length == 64 else 0x80000)
        expected_ram = bytearray([0xA5] * 0x1000)
        expected_ram[offset:offset + 8 + length] = header + payload[:length]
        assert ram == expected_ram, "header, all payload bytes and RAM canaries"
        assert struct.unpack("<I", machine.mem_read(MCAN + 0xD0, 4))[0] == 1 << index
    else:
        assert ram == bytes([0xA5]) * 0x1000, "rejection must not touch message RAM"
        assert struct.unpack("<I", machine.mem_read(MCAN + 0xD0, 4))[0] == 0
    return instructions[0]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", type=Path, default=Path("build/dm4310_sysid.elf"))
    args = parser.parse_args()
    full = run_case(args.elf)
    classic = run_case(args.elf, length=8, cccr=0, identifier=0x6F1)
    run_case(args.elf, initial_mask=1)
    run_case(args.elf, initial_mask=1, txfqs=0x200000, expected=False)
    for options in (
        {"length": 9}, {"length": 0}, {"pointer": 0}, {"identifier": 0x800},
        {"cccr": 0}, {"cccr": 0x100}, {"cccr": 0x301},
        {"psr": 0x80}, {"txfqs": 0x200000}, {"txfqs": 3 << 16},
    ):
        run_case(args.elf, expected=False, **options)
    sampling_counts = sampling_paths(args.elf)
    print(f"ARM sender checks passed: FD64 {full} instructions, classic8 {classic}; "
          "10 rejection cases; counts are NOT hardware cycle/WCET measurements")
    print(f"ARM sampling path instruction counts: {sampling_counts}")


def sampling_paths(path: Path) -> dict[str, int]:
    with path.open("rb") as stream:
        elf = ELFFile(stream)
        symbols = symbol_addresses(elf)
        machine = make_machine(elf, mclass=True)
    machine.mem_map(MCAN, 0x1000)
    machine.mem_map(MESSAGE_RAM, 0x1000)
    machine.mem_write(MCAN + 0x18, struct.pack("<I", 0x300))
    measurement = struct.pack("<11fI7Bx", 1.25, -2.0, 3.0, 0.5, -1.0, 24.0,
                              30.0, 10.0, 0.8, 5.0, 1.0, 7, 1, 1, 0, 1, 0, 0, 0)
    machine.mem_write(PAYLOAD, measurement)
    count = [0]
    machine.hook_add(UC_HOOK_CODE, lambda *_: count.__setitem__(0, count[0] + 1))

    def invoke(name: str, r0: int = 0, r1: int = 0) -> int:
        count[0] = 0
        machine.reg_write(arm.UC_ARM_REG_SP, STACK_TOP)
        machine.reg_write(arm.UC_ARM_REG_LR, RETURN_ADDRESS)
        machine.reg_write(arm.UC_ARM_REG_R0, r0)
        machine.reg_write(arm.UC_ARM_REG_R1, r1)
        machine.emu_start(symbols[name] | 1, RETURN_ADDRESS & ~1, count=10000)
        assert machine.reg_read(arm.UC_ARM_REG_PC) == RETURN_ADDRESS & ~1
        return count[0]

    counts = {"inactive": invoke("identification_sample_tick", PAYLOAD)}
    machine.mem_write(PAYLOAD + 0x100, bytes.fromhex("01 01 01 01 78 56 34 12"))
    invoke("identification_request_irq", PAYLOAD + 0x100, 1)
    invoke("identification_service")
    counts["filter_initialization"] = invoke("identification_sample_tick", PAYLOAD)
    counts["accumulate"] = invoke("identification_sample_tick", PAYLOAD)
    for _ in range(17):
        invoke("identification_sample_tick", PAYLOAD)
    machine.mem_write(PAYLOAD + 54, b"\x01")
    counts["enqueue"] = invoke("identification_sample_tick", PAYLOAD)
    machine.mem_write(PAYLOAD + 54, b"\x00")
    for _ in range(20 * 15):
        invoke("identification_sample_tick", PAYLOAD)
    for _ in range(19):
        invoke("identification_sample_tick", PAYLOAD)
    machine.mem_write(PAYLOAD + 54, b"\x01")
    counts["queue_full"] = invoke("identification_sample_tick", PAYLOAD)
    invoke("identification_service")
    packet = bytes(machine.mem_read(MESSAGE_RAM + 0x328 + 8, 64))
    assert packet[:3] == bytes((1, 0x90, 1))
    assert struct.unpack_from("<f", packet, 28)[0] == 3.0
    return counts


if __name__ == "__main__":
    main()
