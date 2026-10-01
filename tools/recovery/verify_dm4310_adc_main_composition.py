#!/usr/bin/env python3
"""Compose real IRQ002 outer-loop publication with one main iteration."""

import struct

from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_READ
import unicorn.arm_const as arm

from dm4310_unicorn import A, load_fixed_sram_runtime, load_images, make_machine
from regressions.dm4310_model_layout import F


FACTORY, SYMBOLS, SEGMENTS = load_images()
IRQ002 = A(0x1FFF8136)
FACTORY_LOOP = F(0x2540E)
SOURCE_LOOP = SYMBOLS["main"] + 0x60
STATUS = A(0x1FFFF1F0)
CONFIG = A(0x1FFFA5C8)
SAMPLE = A(0x1FFFF104)
MOTOR = A(0x1FFFF088)
SPEED = A(0x1FFFA568)
TEMPERATURE = A(0x1FFFA560)
RAW = A(0x1FFFC778)


def put_u32(machine, address, value):
    machine.mem_write(address, struct.pack("<I", value & 0xFFFFFFFF))


def put_f32(machine, address, value):
    machine.mem_write(address, struct.pack("<f", value))


def run(original, outer_loop_due, fault, indicator_ticks, fpscr, primask):
    machine = make_machine(original, FACTORY, SEGMENTS)
    load_fixed_sram_runtime(machine, original, FACTORY)
    machine.mem_map(0x40000000, 0x100000)
    machine.mem_map(0xE0000000, 0x100000)
    for address, size in ((STATUS, 0x4C), (CONFIG, 0x98),
                          (SAMPLE, 0xA4), (MOTOR, 0x7C),
                          (SPEED, 0x50), (TEMPERATURE, 8), (RAW, 0x20)):
        machine.mem_write(address, bytes(size))
    for address, value in (
        (A(0x1FFF83F8), STATUS), (A(0x1FFF83FC), CONFIG),
        (A(0x1FFF8400), SAMPLE), (A(0x1FFF8404), MOTOR),
        (A(0x1FFF8410), RAW), (A(0x1FFF842C), TEMPERATURE),
        (A(0x1FFF8454), SPEED),
    ):
        put_u32(machine, address, value)
    put_u32(machine, SAMPLE + 0x38, 19 if outer_loop_due else 0)
    put_u32(machine, SAMPLE + 0x3C, 0)
    put_u32(machine, SAMPLE + 0x80, fault)
    put_u32(machine, MOTOR + 0x38, 0)
    put_u32(machine, STATUS + 0x48, indicator_ticks)
    for address, value in (
        (SAMPLE + 0x40, 2048.0), (SAMPLE + 0x44, 2040.0),
        (SAMPLE + 0x48, 2056.0), (SAMPLE + 0x0C, 0.25),
        (SAMPLE + 0x10, -0.125), (SAMPLE + 0x08, 0.2),
        (SAMPLE + 0x2C, 0.1), (SAMPLE + 0x30, 0.8),
        (MOTOR + 0x14, 0.1), (MOTOR + 0x18, 0.2),
        (MOTOR + 0x1C, -0.3), (MOTOR + 0x48, 0.5),
        (MOTOR + 0x4C, 2.0), (MOTOR + 0x68, 0.8),
        (MOTOR + 0x6C, 0.2), (MOTOR + 0x74, 0.99),
        (MOTOR + 0x78, 0.01), (MOTOR + 0x58, 14.0 / 6.283185307179586),
        (MOTOR + 0x5C, 1.0), (SPEED + 0x00, 0.4),
        (SPEED + 0x18, 0.15), (SPEED + 0x28, 0.5),
    ):
        put_f32(machine, address, value)
    put_u32(machine, MOTOR + 0x54, 14)
    for index, value in enumerate((0.0, 0.0, 0.0, 1.0,
                                   0.02, -0.02, 30.0)):
        put_f32(machine, CONFIG + index * 4, value)
    for address, value in ((0x40040000, 2050), (0x40040450, 2040),
                           (0x40040850, 2030), (0x40040452, 1234),
                           (0x40040852, 2100), (0x40040454, 2200),
                           (0x40040854, 2300)):
        machine.mem_write(address, struct.pack("<H", value))
    machine.mem_write(A(0x1FFFF1A8), bytes(0x40))
    machine.mem_write(A(0x1FFFF190), bytes(0x18))
    machine.mem_write(A(0x1FFFC78C), bytes(0x98))
    machine.mem_write(A(0x1FFFC860), bytes(0x3C))
    put_f32(machine, A(0x1FFFF1A8), 0.1)
    put_f32(machine, A(0x1FFFF1AC), 0.2)
    put_f32(machine, A(0x1FFFF1B0), 0.3)
    machine.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xF00000)
    machine.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    machine.reg_write(arm.UC_ARM_REG_FPSCR, fpscr)
    machine.reg_write(arm.UC_ARM_REG_PRIMASK, primask)
    events = []
    phase = ["irq"]
    loop_visits = [0]

    def memory(emu, access, address, size, value, unused):
        observed = (int.from_bytes(emu.mem_read(address, size), "little")
                    if access == UC_MEM_READ else
                    value & ((1 << (size * 8)) - 1))
        events.append((phase[0], access, address, size, observed))

    def code(emu, address, size, unused):
        if phase[0] == "main" and address == (
                FACTORY_LOOP if original else SOURCE_LOOP):
            loop_visits[0] += 1
            if loop_visits[0] == 2:
                emu.emu_stop()

    for start, end in (
        (A(0x1FFF83F8), A(0x1FFF845F)),
        (STATUS, STATUS + 0x4B), (CONFIG, CONFIG + 0x97),
        (SAMPLE, SAMPLE + 0xA3), (MOTOR, MOTOR + 0x7B),
        (SPEED, SPEED + 0x4F), (TEMPERATURE, TEMPERATURE + 7),
        (RAW, RAW + 0x1F), (0x40038000, 0x40040FFF),
        (0xE000E100, 0xE000E2FF),
    ):
        machine.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                         begin=start, end=end)
    machine.hook_add(UC_HOOK_CODE, code)
    machine.reg_write(arm.UC_ARM_REG_SP, 0x2000F000)
    machine.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    machine.emu_start(IRQ002 | 1, 0x30000, count=500000)
    assert machine.reg_read(arm.UC_ARM_REG_PC) == 0x30000
    published_status = bytes(machine.mem_read(STATUS, 0x4C))

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
        machine.mem_write(0x2000EFF4, struct.pack("<I", 0x200016BC))
    machine.emu_start((FACTORY_LOOP if original else SOURCE_LOOP) | 1,
                      0x30000, count=200000)
    assert loop_visits[0] == 2
    return (
        events,
        published_status,
        bytes(machine.mem_read(STATUS, 0x4C)),
        bytes(machine.mem_read(SAMPLE, 0xA4)),
        bytes(machine.mem_read(MOTOR, 0x7C)),
        bytes(machine.mem_read(SPEED, 0x50)),
        bytes(machine.mem_read(0x40038000, 0x9000)),
        bytes(machine.mem_read(0xE000E100, 0x200)),
        machine.reg_read(arm.UC_ARM_REG_PRIMASK),
        machine.reg_read(arm.UC_ARM_REG_FPSCR),
    )


def main():
    for case in range(16):
        outer_loop_due = bool(case & 1)
        fault = 2 if case & 2 else 0
        indicator_ticks = 250 if case & 4 else 17
        fpscr = ((case >> 1) & 3) << 22
        primask = (case >> 3) & 1
        factory_result = run(True, outer_loop_due, fault, indicator_ticks,
                             fpscr, primask)
        source_result = run(False, outer_loop_due, fault, indicator_ticks,
                            fpscr, primask)
        assert factory_result == source_result, (case, factory_result,
                                                  source_result)
        published_tick = int.from_bytes(factory_result[1][0x2C:0x30],
                                        "little")
        assert published_tick == (1 if outer_loop_due else 0)
        assert factory_result[2][0x2C:0x30] == bytes(4)
    print("PASS: 16 composed IRQ002-to-main cases; outer-loop status "
          "publication, fault-indicator consumption, fixed SRAM/MMIO order, "
          "PRIMASK and FPSCR")


if __name__ == "__main__":
    main()
