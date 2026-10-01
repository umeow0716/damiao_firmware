#!/usr/bin/env python3
"""Factory/source differential checks for fixed position IRQ000 and IRQ001."""

import random
import struct

from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_WRITE
import unicorn.arm_const as arm

from dm4310_unicorn import A, load_fixed_sram_runtime, load_images, make_machine


FACTORY, SYMBOLS, SEGMENTS = load_images()
IRQ000 = A(0x1FFF8744)
IRQ001 = A(0x1FFF8770)


def machine(original):
    uc = make_machine(original, FACTORY, SEGMENTS)
    load_fixed_sram_runtime(uc, original, FACTORY)
    uc.mem_map(0xE000E000, 0x1000)
    uc.reg_write(arm.UC_ARM_REG_SP, 0x2000F000)
    uc.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    return uc


def timer_irq_result(original, ccsr, timer, transmit):
    uc = machine(original)
    uc.mem_write(A(0x1FFF8B40), timer.to_bytes(4, "little"))
    uc.mem_write(A(0x1FFF8B44), transmit.to_bytes(4, "little"))
    uc.mem_write(timer, bytes(0x80))
    uc.mem_write(timer + 0x58, (ccsr & 0xFFFF).to_bytes(2, "little"))
    uc.mem_write(transmit, b"\x5a\xa5\x5a\xa5")
    events = []

    def memory(emu, access, address, size, value, unused):
        observed = value if access == UC_MEM_WRITE else int.from_bytes(
            emu.mem_read(address, size), "little")
        events.append((access, address, size, observed))

    for start, end in (
        (A(0x1FFF8B40), A(0x1FFF8B47)),
        (timer, timer + 0x7F),
        (transmit, transmit + 3),
        (0xE000E280, 0xE000E283),
    ):
        uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                    begin=start, end=end)
    uc.emu_start(IRQ000 | 1, 0x30000, count=1000)
    assert uc.reg_read(arm.UC_ARM_REG_PC) == 0x30000
    return (
        events,
        bytes(uc.mem_read(timer, 0x80)),
        bytes(uc.mem_read(transmit, 4)),
        bytes(uc.mem_read(0xE000E280, 4)),
    )


def verify_timer_irq():
    rng = random.Random(0x1FFF8744)
    for case in range(256):
        timer = 0x20008000 + ((case & 3) * 0x100)
        transmit = 0x20009000 + ((case & 3) * 4)
        ccsr = rng.getrandbits(16)
        factory_result = timer_irq_result(True, ccsr, timer, transmit)
        source_result = timer_irq_result(False, ccsr, timer, transmit)
        assert factory_result == source_result, (case, ccsr,
                                                  factory_result,
                                                  source_result)
    print("PASS: 256 IRQ000 cases; retained pool pointers, conditional SPI "
          "write, timer clear and NVIC order")


def dma_irq_result(original, case, ready, objects, initial):
    status, motor, raw, scratch, table, count, enable, clear = objects
    uc = machine(original)
    uc.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xF00000)
    uc.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    uc.reg_write(arm.UC_ARM_REG_FPSCR, (case & 3) << 22)
    uc.mem_write(A(0x1FFF8B48), struct.pack("<f", -0.0 if case & 4 else 0.0))
    pointers = (status, motor, raw, scratch, table)
    uc.mem_write(A(0x1FFF8B4C), struct.pack("<5I", *pointers))
    uc.mem_write(A(0x1FFF8B60), struct.pack("<3f", 1.0 / 64.0,
                                         float.fromhex("0x1.921fb6p-12"),
                                         float.fromhex("0x1.921fb6p+2")))
    uc.mem_write(A(0x1FFF8B6C), struct.pack("<2I", 0x40B00000, 0xC0B00000))
    uc.mem_write(A(0x1FFF8B74), struct.pack("<3I", count, enable, clear))
    for address, data in initial.items():
        uc.mem_write(address, data)
    uc.mem_write(status, struct.pack("<I", 1 if ready else 0))
    events = []
    source_only_writes = []

    def memory(emu, access, address, size, value, unused):
        observed = value if access == UC_MEM_WRITE else int.from_bytes(
            emu.mem_read(address, size), "little")
        events.append((access, address, size, observed))

    def source_only_memory(emu, access, address, size, value, unused):
        source_only_writes.append((address, size, value))

    ranges = (
        (A(0x1FFF8B48), A(0x1FFF8B7F)),
        (status, status + 3),
        (motor, motor + 0x7B),
        (raw, raw + 1),
        (scratch, scratch + 0x17),
        (table, table + 0x3FF),
        (count, count + 3),
        (enable, enable + 3),
        (clear, clear + 3),
        (0xE000E280, 0xE000E283),
    )
    for start, end in ranges:
        uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                    begin=start, end=end)
    for address, size in ((SYMBOLS["g_app"], 0x320),
                          (SYMBOLS["position_sensor"], 0x48)):
        uc.hook_add(UC_HOOK_MEM_WRITE, source_only_memory,
                    begin=address, end=address + size - 1)
    uc.emu_start(IRQ001 | 1, 0x30000, count=100000)
    assert uc.reg_read(arm.UC_ARM_REG_PC) == 0x30000
    return (
        events,
        bytes(uc.mem_read(motor, 0x7C)),
        bytes(uc.mem_read(scratch, 0x18)),
        bytes(uc.mem_read(count, 4)),
        bytes(uc.mem_read(enable, 4)),
        bytes(uc.mem_read(clear, 4)),
        bytes(uc.mem_read(0xE000E280, 4)),
        uc.reg_read(arm.UC_ARM_REG_FPSCR),
        tuple(source_only_writes),
    )


def verify_dma_irq():
    rng = random.Random(0x1FFF8770)
    for case in range(256):
        relocated = bool(case & 0x80)
        base = 0x20008000 if relocated else 0x1FFF0000
        status = base + 0x000
        motor = base + 0x100
        raw = base + 0x200
        scratch = base + 0x300
        table = base + 0x400
        count = base + 0x800
        enable = base + 0x804
        clear = base + 0x808
        objects = (status, motor, raw, scratch, table, count, enable, clear)
        motor_image = bytearray(rng.getrandbits(8) for _ in range(0x7C))
        scratch_image = bytearray(rng.getrandbits(8) for _ in range(0x18))
        struct.pack_into("<I", motor_image, 0x08,
                         (case * 0x10203) & 0xFFFFFFFF)
        struct.pack_into("<I", motor_image, 0x34,
                         0x3F800000 if case & 1 else 0x40000000)
        struct.pack_into("<f", motor_image, 0x24,
                         ((case % 17) - 8) * 0.125)
        struct.pack_into("<f", motor_image, 0x5C,
                         0.25 + (case % 7) * 0.125)
        struct.pack_into("<f", scratch_image, 0x0C,
                         ((case % 31) - 15) * 0.25)
        struct.pack_into("<f", scratch_image, 0x10,
                         ((case % 13) - 6) * 0.0625)
        table_image = bytearray()
        for index in range(256):
            correction = ((index * 17 + case * 3) % 127 - 63) / 1024.0
            table_image.extend(struct.pack("<f", correction))
        initial = {
            status: bytes(4),
            motor: bytes(motor_image),
            raw: struct.pack("<H", (case * 257 + 0x1234) & 0xFFFF),
            scratch: bytes(scratch_image),
            table: bytes(table_image),
            count: struct.pack("<I", rng.getrandbits(32)),
            enable: struct.pack("<I", rng.getrandbits(32)),
            clear: struct.pack("<I", rng.getrandbits(32)),
        }
        ready = bool(case & 2)
        factory_result = dma_irq_result(True, case, ready, objects, initial)
        source_result = dma_irq_result(False, case, ready, objects, initial)
        assert factory_result == source_result, (case, ready,
                                                  factory_result,
                                                  source_result)
    print("PASS: 256 IRQ001 cases; ready/not-ready and relocated pointer "
          "pools, complete fixed state/MMIO trace, FPSCR and no source-only "
          "global writes")


def main():
    verify_timer_irq()
    verify_dma_irq()


if __name__ == "__main__":
    main()
