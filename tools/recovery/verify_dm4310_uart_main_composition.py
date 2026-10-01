#!/usr/bin/env python3
"""Compose real IRQ004 command publication with one main-loop iteration."""

import struct

from unicorn import (UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE,
                     UC_MEM_READ)
import unicorn.arm_const as arm

from dm4310_unicorn import A, load_fixed_sram_runtime, load_images, make_machine
from regressions.dm4310_model_layout import F


FACTORY, SYMBOLS, SEGMENTS = load_images()
FACTORY_IRQ = F(0x22244)
SOURCE_IRQ = SYMBOLS["IRQ004_Handler"]
FACTORY_LOOP = F(0x2540E)
SOURCE_LOOP = SYMBOLS["main"] + 0x60
FACTORY_UART = F(0x2379C)
SOURCE_UART = SYMBOLS["platform_debug_write"]
FACTORY_DIRECTION = F(0x264A4)
SOURCE_DIRECTION = SYMBOLS["commissioning_run_direction_and_alignment"]
FACTORY_ALIGNMENT = F(0x24B20)
FACTORY_MOTOR_ID = F(0x257C4)
SOURCE_MOTOR_ID = SYMBOLS["commissioning_run_motor_identification"]
FACTORY_FIRMWARE = F(0x26D00)
SOURCE_FIRMWARE = SYMBOLS["firmware_control_service"]
FACTORY_OUTPUT = F(0x246F8)
SOURCE_OUTPUT = SYMBOLS["commissioning_run_output_sensor_calibration"]
FACTORY_LOAD_CAL = F(0x22658)
SOURCE_LOAD_CAL = SYMBOLS["platform_load_motor_calibration"]
FLASH_WRITER = A(0x1FFF9950)
STATUS = A(0x1FFFF1F0)
MOTOR = A(0x1FFFF088)
SAMPLE = A(0x1FFFF104)
RX_BUFFER = A(0x1FFFF3C7)
STATE_CAPTURE_BASE = A(0x1FFFA5B8)
STATE_CAPTURE_SIZE = A(0x1FFFF490) - STATE_CAPTURE_BASE

CASES = (
    ("direction", b"Uc", 1, ("direction-and-alignment",)),
    ("output", b"Ul", 1, ("output-calibration",)),
    ("motor-id", b"UeU", 3, ("motor-identification",)),
    ("firmware-read", b"Ug\xaa", 3, ("firmware-control", 1)),
    ("firmware-write", b"UgU" + bytes(range(128)), 131,
     ("firmware-control", 2)),
    ("firmware-zero", b"UgZ" + bytes(reversed(range(128))), 131,
     ("firmware-control", 4)),
    ("firmware-derive", b"UgQ" + bytes([0x5A]) * 128, 131,
     ("firmware-control", 6)),
    ("motor-calibration", b"Ud" + bytes(60) + bytes((2, 17)), 64,
     ("load-calibration",)),
    ("output-calibration", b"UM" + bytes(60) + bytes((2, 136)), 64,
     ("load-calibration",)),
)


def machine(original, command, received_length, primask):
    emu = make_machine(original, FACTORY, SEGMENTS)
    load_fixed_sram_runtime(emu, original, FACTORY)
    emu.mem_map(0x10000, 0x10000)
    emu.mem_map(0x40000000, 0x100000)
    emu.mem_map(0xE0000000, 0x100000)
    emu.mem_write(A(0x1FFFA5B8), struct.pack("<HH", 63, 0))
    emu.mem_write(A(0x1FFFF0C0), struct.pack("<I", 4))
    emu.mem_write(A(0x1FFFF184), bytes(4))
    emu.mem_write(A(0x1FFFF0BC), struct.pack("<I", 0x3F400000))
    emu.mem_write(STATUS, bytes(0x4C))
    emu.mem_write(RX_BUFFER, command)
    emu.mem_write(0x4001CC00, struct.pack("<I", 0x100))
    emu.mem_write(0x40053068,
                  struct.pack("<I", ((200 - received_length) & 0xFFFF) << 16))
    emu.mem_write(0x1E000, bytes(range(64)))
    emu.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xF00000)
    emu.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    emu.reg_write(arm.UC_ARM_REG_FPSCR, 0)
    emu.reg_write(arm.UC_ARM_REG_PRIMASK, primask)
    return emu


def run(original, command, received_length, primask):
    emu = machine(original, command, received_length, primask)
    events = []
    semantic_calls = []
    phase = ["irq"]
    loop_visits = [0]

    def memory(machine, access, address, size, value, unused):
        observed = (int.from_bytes(machine.mem_read(address, size), "little")
                    if access == UC_MEM_READ else
                    value & ((1 << (size * 8)) - 1))
        events.append((phase[0], access, address, size, observed))

    def return_from_stub(machine):
        machine.reg_write(arm.UC_ARM_REG_PC,
                          machine.reg_read(arm.UC_ARM_REG_LR))

    def code(machine, address, size, unused):
        if address == (FACTORY_UART if original else SOURCE_UART):
            pointer = machine.reg_read(arm.UC_ARM_REG_R0)
            length = machine.reg_read(arm.UC_ARM_REG_R1)
            events.append((phase[0], "uart", length,
                           bytes(machine.mem_read(pointer, length))))
            return_from_stub(machine)
            return
        if phase[0] != "main":
            return
        if address == (FACTORY_LOOP if original else SOURCE_LOOP):
            loop_visits[0] += 1
            if loop_visits[0] == 2:
                machine.emu_stop()
                return
        if address == (FACTORY_DIRECTION if original else SOURCE_DIRECTION):
            semantic_calls.append(("direction-and-alignment",))
            return_from_stub(machine)
        elif original and address == FACTORY_ALIGNMENT:
            return_from_stub(machine)
        elif address == (FACTORY_MOTOR_ID if original else SOURCE_MOTOR_ID):
            semantic_calls.append(("motor-identification",))
            return_from_stub(machine)
        elif address == (FACTORY_FIRMWARE if original else SOURCE_FIRMWARE):
            semantic_calls.append(("firmware-control",
                                   machine.reg_read(arm.UC_ARM_REG_R0) & 0xFF))
            return_from_stub(machine)
        elif address == (FACTORY_OUTPUT if original else SOURCE_OUTPUT):
            semantic_calls.append(("output-calibration",))
            return_from_stub(machine)
        elif address == (FACTORY_LOAD_CAL if original else SOURCE_LOAD_CAL):
            semantic_calls.append(("load-calibration",))
            return_from_stub(machine)
        elif address == FLASH_WRITER:
            events.append(("main", "flash-write",
                           machine.reg_read(arm.UC_ARM_REG_R0),
                           machine.reg_read(arm.UC_ARM_REG_R1),
                           machine.reg_read(arm.UC_ARM_REG_R2)))
            return_from_stub(machine)

    emu.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                 begin=0x1FFF0000, end=0x1FFFFFFF)
    emu.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                 begin=0x40000000, end=0x400FFFFF)
    emu.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                 begin=0xE000E000, end=0xE000EFFF)
    emu.hook_add(UC_HOOK_CODE, code)

    emu.reg_write(arm.UC_ARM_REG_SP, 0x2000F000)
    emu.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    emu.emu_start((FACTORY_IRQ if original else SOURCE_IRQ) | 1,
                  0x30000, count=400000)
    assert emu.reg_read(arm.UC_ARM_REG_PC) == 0x30000
    published_status = bytes(emu.mem_read(STATUS, 0x34))

    phase[0] = "main"
    emu.reg_write(arm.UC_ARM_REG_SP, 0x2000EFF0)
    emu.reg_write(arm.UC_ARM_REG_PRIMASK, primask)
    if original:
        emu.reg_write(arm.UC_ARM_REG_R4, 0)
        emu.reg_write(arm.UC_ARM_REG_R5, MOTOR)
        emu.reg_write(arm.UC_ARM_REG_R6, STATUS)
        emu.reg_write(arm.UC_ARM_REG_R7, SAMPLE)
        emu.reg_write(arm.UC_ARM_REG_R8, 1)
        emu.reg_write(arm.UC_ARM_REG_R9, 0xE000E000)
        emu.reg_write(arm.UC_ARM_REG_R10, 4)
        emu.reg_write(arm.UC_ARM_REG_R11, 0x4005382A)
        emu.reg_write(arm.UC_ARM_REG_S16, 0)
    else:
        emu.reg_write(arm.UC_ARM_REG_R4, STATUS)
        emu.reg_write(arm.UC_ARM_REG_R5, 0x20001600)
        emu.mem_write(0x2000EFF4, struct.pack("<I", 0x200016BC))
    emu.emu_start((FACTORY_LOOP if original else SOURCE_LOOP) | 1,
                  0x30000, count=500000)
    assert loop_visits[0] == 2
    return (
        events,
        semantic_calls,
        published_status,
        bytes(emu.mem_read(STATE_CAPTURE_BASE, STATE_CAPTURE_SIZE)),
        bytes(emu.mem_read(0x4001C000, 0x1000)),
        bytes(emu.mem_read(0x40053000, 0x100)),
        bytes(emu.mem_read(0xE000E000, 0x1000)),
        emu.reg_read(arm.UC_ARM_REG_PRIMASK),
        emu.reg_read(arm.UC_ARM_REG_FPSCR),
    )


def main():
    for index, (name, command, length, expected_call) in enumerate(CASES):
        primask = index & 1
        factory_result = run(True, command, length, primask)
        source_result = run(False, command, length, primask)
        assert factory_result == source_result, (name, factory_result,
                                                  source_result)
        assert factory_result[1] == [expected_call], (
            name, factory_result[1], factory_result[2])
        assert factory_result[3][STATUS - STATE_CAPTURE_BASE:
                                 STATUS - STATE_CAPTURE_BASE + 0x34] == bytes(0x34)
    print(f"PASS: {len(CASES)} composed IRQ004-to-main command paths; event "
          "publication, child sequencing, request ABI, fixed SRAM/MMIO, "
          "event clear, PRIMASK and FPSCR")


if __name__ == "__main__":
    main()
