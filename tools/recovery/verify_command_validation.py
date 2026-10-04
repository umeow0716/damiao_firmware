#!/usr/bin/env python3
"""Execute malformed command rejection in the built ARM firmware, without CAN."""
import argparse
import struct
from pathlib import Path
from elftools.elf.elffile import ELFFile
import unicorn.arm_const as arm
from verify_feedback_variants import make_machine, symbol_addresses, RETURN_ADDRESS


def check(path, identifier, length, payload, expected, enabled=False):
    with path.open('rb') as stream:
        elf = ELFFile(stream)
        machine = make_machine(elf, mclass=True)
        symbols = symbol_addresses(elf)
    frame, output, context = 0x20001000, 0x20002000, 0x20003000
    config, motor, sample, status, scratch = [0x20004000 + i * 0x200 for i in range(5)]
    machine.mem_write(frame, struct.pack('<IB', identifier, length) + payload.ljust(64, b'\0'))
    machine.mem_write(output, b'\xA5' * 64)
    machine.mem_write(context, struct.pack('<13I', 0, config, status, 0, 0, 0, 0, scratch, sample, motor, 0, 0, 3))
    machine.mem_write(motor + 0x38, struct.pack('<I', 2 if enabled else 0))
    machine.mem_write(sample + 0x3C, struct.pack('<I', 3))
    machine.mem_write(config + 0x50, struct.pack('<f', 7))
    for addr, value in ((0x1fff8b94, sample), (0x1fff8554, sample),
                        (0x1fff8b84, config), (0x1fff8544, config),
                        (0x1fff8b50, motor), (0x1fff8510, motor)):
        machine.mem_write(addr, struct.pack('<I', value))
    before = bytes(machine.mem_read(sample, 128))
    for reg, value in ((arm.UC_ARM_REG_R0, frame), (arm.UC_ARM_REG_R1, output), (arm.UC_ARM_REG_R2, context)):
        machine.reg_write(reg, value)
    machine.emu_start(symbols['can_protocol_decode_command_irq'] | 1, RETURN_ADDRESS & ~1, count=10000)
    assert machine.reg_read(arm.UC_ARM_REG_PC) == RETURN_ADDRESS & ~1
    assert machine.reg_read(arm.UC_ARM_REG_R0) == expected, (identifier, length, payload)
    if expected == 0:
        assert bytes(machine.mem_read(sample, 128)) == before
        assert bytes(machine.mem_read(output, 64)) == b'\xA5' * 64


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path, required=True)
    args = parser.parse_args()
    for family in range(4):
        for length in range(65):
            good = length == 8 or (family == 2 and length == 4)
            check(args.elf, 3 + 256 * family, length, bytes(8), 1 if good else 0)
    for family in (1, 2, 3):
        for value in (float('nan'), float('inf'), -float('inf')):
            check(args.elf, 3 + 256 * family, 8, struct.pack('<f', value), 0, True)
    check(args.elf, 0x103, 8, struct.pack('<ff', 0, float('nan')), 0, True)
    check(args.elf, 0x203, 8, struct.pack('<f', 3.4028234663852886e38), 0, True)
    for identifier in (4, 0x403, 0x803):
        check(args.elf, identifier, 8, bytes(8), 0)
    for code, expected in ((0xFC, 3), (0xFD, 4), (0xFE, 5), (0xFB, 6)):
        check(args.elf, 3, 8, b'\xff' * 7 + bytes([code]), expected)
        check(args.elf, 3, 7, b'\xff' * 7 + bytes([code]), 0)
    check(args.elf, 0x203, 8, struct.pack('<f', .2), 2, True)
    check(args.elf, 0x203, 4, struct.pack('<f', -.2), 2, True)
    print(f'{args.elf.name}: ARM command rejection and valid VEL/special commands passed')

if __name__ == '__main__':
    main()
