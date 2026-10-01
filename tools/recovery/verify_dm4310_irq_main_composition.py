#!/usr/bin/env python3
"""Compose the real IRQ003 error tail with one deferred-event iteration."""

import random
import struct

from unicorn import (UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE,
                     UC_MEM_READ)
import unicorn.arm_const as arm

from dm4310_unicorn import A, load_fixed_sram_runtime, load_images, make_machine
from regressions.dm4310_model_layout import F


FACTORY, SYMBOLS, SEGMENTS = load_images()
IRQ003 = A(0x1FFF88B8)
FACTORY_LOOP = F(0x2540E)
SOURCE_LOOP = SYMBOLS["main"] + 0x60
FACTORY_PRINTF = F(0x20474)
SOURCE_PRINTF = SYMBOLS["debug_console_printf"]
CONTROLLER = 0x40029000
CONFIG = A(0x1FFFA5C8)
STATUS = A(0x1FFFF1F0)
DISPATCH = A(0x1FFFF29C)
MOTOR = A(0x1FFFF088)
SAMPLE = A(0x1FFFF104)


def read_c_string(machine, pointer):
    result = bytearray()
    while len(result) < 128:
        value = bytes(machine.mem_read(pointer + len(result), 1))[0]
        if value == 0:
            return bytes(result)
        result.append(value)
    raise AssertionError("unterminated diagnostic string")


def run(original, interrupt_flags, cccr, primask):
    machine = make_machine(original, FACTORY, SEGMENTS)
    load_fixed_sram_runtime(machine, original, FACTORY)
    machine.mem_map(0x40000000, 0x100000)
    machine.mem_map(0xE0000000, 0x100000)
    machine.mem_write(CONTROLLER, bytes(0x1000))
    machine.mem_write(CONFIG, bytes(0x98))
    machine.mem_write(STATUS, bytes(0x4C))
    machine.mem_write(DISPATCH, bytes(8))
    machine.mem_write(A(0x1FFF8B80),
                      struct.pack("<III", CONTROLLER, CONFIG, STATUS))
    machine.mem_write(A(0x1FFF9878), struct.pack("<I", DISPATCH))
    machine.mem_write(DISPATCH, struct.pack("<I", 0x2C001))
    machine.mem_write(CONFIG + 0x20, struct.pack("<H", 0x123))
    machine.mem_write(CONFIG + 0x8C, struct.pack("<H", 0x456))
    machine.mem_write(CONTROLLER + 0x18, struct.pack("<I", cccr))
    machine.mem_write(CONTROLLER + 0x50,
                      struct.pack("<I", interrupt_flags))
    machine.mem_write(STATUS + 0x30, bytes(4))
    machine.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xF00000)
    machine.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    machine.reg_write(arm.UC_ARM_REG_FPSCR, 0)
    machine.reg_write(arm.UC_ARM_REG_SP, 0x2000F000)
    machine.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    machine.reg_write(arm.UC_ARM_REG_PRIMASK, primask)
    events = []
    phase = ["irq"]
    loop_visits = [0]

    def memory(emu, access, address, size, value, unused):
        observed = (int.from_bytes(emu.mem_read(address, size), "little")
                    if access == UC_MEM_READ else
                    value & ((1 << (size * 8)) - 1))
        events.append((phase[0], access, address, size, observed))

    def return_from_stub(emu):
        emu.reg_write(arm.UC_ARM_REG_PC, emu.reg_read(arm.UC_ARM_REG_LR))

    def code(emu, address, size, unused):
        if address == 0x2C000:
            events.append((phase[0], "callback",
                           emu.reg_read(arm.UC_ARM_REG_R0),
                           emu.reg_read(arm.UC_ARM_REG_R1)))
            return_from_stub(emu)
            return
        if phase[0] != "main":
            return
        if address == (FACTORY_LOOP if original else SOURCE_LOOP):
            loop_visits[0] += 1
            if loop_visits[0] == 2:
                emu.emu_stop()
                return
        if address == (FACTORY_PRINTF if original else SOURCE_PRINTF):
            events.append(("main", "text", read_c_string(
                emu, emu.reg_read(arm.UC_ARM_REG_R0))))
            return_from_stub(emu)

    for start, end in (
        (A(0x1FFF8B80), A(0x1FFF8B8B)),
        (A(0x1FFF9878), A(0x1FFF987B)),
        (CONTROLLER, CONTROLLER + 0xFF),
        (CONFIG, CONFIG + 0x97),
        (STATUS, STATUS + 0x4B),
        (DISPATCH, DISPATCH + 7),
        (0xE000E100, 0xE000E2FF),
    ):
        machine.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                         begin=start, end=end)
    machine.hook_add(UC_HOOK_CODE, code)

    machine.emu_start(IRQ003 | 1, 0x30000, count=200000)
    assert machine.reg_read(arm.UC_ARM_REG_PC) == 0x30000

    phase[0] = "main"
    machine.reg_write(arm.UC_ARM_REG_SP, 0x2000EFF0)
    machine.reg_write(arm.UC_ARM_REG_PRIMASK, primask)
    if original:
        machine.reg_write(arm.UC_ARM_REG_R4, 0)
        machine.reg_write(arm.UC_ARM_REG_R5, MOTOR)
        machine.reg_write(arm.UC_ARM_REG_R6, STATUS)
        machine.reg_write(arm.UC_ARM_REG_R7, SAMPLE)
        machine.reg_write(arm.UC_ARM_REG_R8, 1)
        machine.reg_write(arm.UC_ARM_REG_R9, 0xE000E000)
        machine.reg_write(arm.UC_ARM_REG_R10, 4)
        machine.reg_write(arm.UC_ARM_REG_R11, 0x4005382A)
        machine.reg_write(arm.UC_ARM_REG_S16, 0)
    else:
        machine.reg_write(arm.UC_ARM_REG_R4, STATUS)
        machine.reg_write(arm.UC_ARM_REG_R5, 0x20001600)
        machine.mem_write(0x2000EFF4,
                          struct.pack("<I", 0x200016BC))
    machine.emu_start((FACTORY_LOOP if original else SOURCE_LOOP) | 1,
                      0x30000, count=100000)
    assert loop_visits[0] == 2
    return (
        events,
        bytes(machine.mem_read(STATUS, 0x4C)),
        bytes(machine.mem_read(CONTROLLER, 0x100)),
        bytes(machine.mem_read(0xE000E100, 0x200)),
        machine.reg_read(arm.UC_ARM_REG_PRIMASK),
        machine.reg_read(arm.UC_ARM_REG_FPSCR),
    )


def main():
    rng = random.Random(0x1FFF88B8)
    for case in range(64):
        reinitialize = bool(case & 1)
        bus_off = bool(case & 2)
        interrupt_flags = ((0x00800000 if reinitialize else 0) |
                           (0x02000000 if bus_off else 0))
        cccr = rng.getrandbits(32)
        primask = (case >> 2) & 1
        factory_result = run(True, interrupt_flags, cccr, primask)
        source_result = run(False, interrupt_flags, cccr, primask)
        assert factory_result == source_result, (case, interrupt_flags,
                                                  factory_result,
                                                  source_result)
        expected_error = 2 if bus_off else (1 if reinitialize else 0)
        texts = [event[2] for event in factory_result[0]
                 if event[:2] == ("main", "text")]
        expected_texts = ([] if expected_error == 0 else
                          [f"CAN Error {expected_error}\n\r".encode()])
        assert texts == expected_texts, (case, texts, expected_texts)
        assert factory_result[1][0x30:0x34] == bytes(4)
    print("PASS: 64 composed IRQ003-to-main cases; reinitialize/bus-off "
          "publication, diagnostic, event clear, fixed SRAM/MMIO order, "
          "PRIMASK and FPSCR")


if __name__ == "__main__":
    main()
