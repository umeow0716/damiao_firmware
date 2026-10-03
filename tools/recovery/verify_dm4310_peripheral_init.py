#!/usr/bin/env python3
"""Strict factory/source differential checks for DM4310 peripheral startup.

This verifier is intentionally isolated from CMake and the normal firmware
build. Run it explicitly after building the DM4310 target.
"""

import random

from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_READ
import unicorn.arm_const as arm

from dm4310_unicorn import A, load_images, make_machine
from regressions.dm4310_model_layout import F


factory, symbols, segments = load_images()


def machine(original):
    return make_machine(original, factory, segments)


def run_identity(original, initial, fpscr):
    u = machine(original)
    u.mem_map(0x40000000, 0x100000)
    u.mem_write(0x40053000, initial)
    u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    u.reg_write(arm.UC_ARM_REG_FPSCR, fpscr)
    trace = []

    def memory(uc, access, address, size, value, _):
        if access == UC_MEM_READ:
            value = int.from_bytes(uc.mem_read(address, size), 'little')
        trace.append((access, address, size, value))

    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=0x40053000, end=0x40053fff)
    entry = F(0x221f4) if original else symbols[
        'board_identity_initialize_status_and_read_variant']
    u.emu_start(entry | 1, 0x30000, count=1000)
    assert u.reg_read(arm.UC_ARM_REG_PC) == 0x30000
    return (trace, bytes(u.mem_read(0x40053000, 0x1000)),
            u.reg_read(arm.UC_ARM_REG_R0), u.reg_read(arm.UC_ARM_REG_FPSCR))


rng = random.Random(0x221f4)
for case in range(256):
    initial = bytes(rng.getrandbits(8) for _ in range(0x1000))
    fpscr = rng.getrandbits(32)
    a = run_identity(True, initial, fpscr)
    b = run_identity(False, initial, fpscr)
    assert a == b, (case, a, b)
print('PASS: 256 identity/LED initialization cases; ordered GPIO widths, '
      'protected writes, RMW state, strap result and FPSCR')


def run_power_stage(original, gpio, adc, fpscr):
    u = machine(original)
    u.mem_map(0x40000000, 0x100000)
    u.mem_write(0x40053000, gpio)
    u.mem_write(0x40040000, adc)
    failure = 0x20008000
    u.mem_write(failure, b'\xa5')
    u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    u.reg_write(arm.UC_ARM_REG_R0, 0 if original else failure)
    u.reg_write(arm.UC_ARM_REG_FPSCR, fpscr)
    trace = []
    delay = F(0x21f60) if original else symbols['board_delay_us']

    def memory(uc, access, address, size, value, _):
        if access == UC_MEM_READ:
            value = int.from_bytes(uc.mem_read(address, size), 'little')
        trace.append((access, address, size, value))

    def code(uc, address, size, _):
        if address == delay:
            trace.append(('delay', uc.reg_read(arm.UC_ARM_REG_R0)))
            uc.reg_write(arm.UC_ARM_REG_PC, uc.reg_read(arm.UC_ARM_REG_LR))

    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=0x40040000, end=0x40040fff)
    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=0x40053000, end=0x40053fff)
    u.hook_add(UC_HOOK_CODE, code)
    entry = F(0x22ca8) if original else symbols['board_power_stage_self_test']
    u.emu_start(entry | 1, 0x30000, count=100000)
    assert u.reg_read(arm.UC_ARM_REG_PC) == 0x30000
    if original:
        mask = u.reg_read(arm.UC_ARM_REG_R0) & 0xff
        ok = mask == 0
    else:
        mask = u.mem_read(failure, 1)[0]
        ok = bool(u.reg_read(arm.UC_ARM_REG_R0))
    return (trace, bytes(u.mem_read(0x40040000, 0x1000)),
            bytes(u.mem_read(0x40053000, 0x1000)), mask, ok,
            u.reg_read(arm.UC_ARM_REG_FPSCR))


for case in range(256):
    gpio = bytes(rng.getrandbits(8) for _ in range(0x1000))
    adc = bytearray(rng.getrandbits(8) for _ in range(0x1000))
    for offset in (0x44, 0x444, 0x844):
        adc[offset] |= 1
    fpscr = rng.getrandbits(32)
    a = run_power_stage(True, gpio, bytes(adc), fpscr)
    b = run_power_stage(False, gpio, bytes(adc), fpscr)
    assert a == b, (case, a, b)
print('PASS: 256 six-phase power-stage self-test cases; ordered GPIO/ADC '
      'MMIO, delays, samples, failure bitmap, final state and FPSCR')


def run_adc_init(original, pages, fpscr):
    u = machine(original)
    u.mem_map(0x40000000, 0x100000)
    for address, data in pages.items():
        u.mem_write(address, data)
    u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    u.reg_write(arm.UC_ARM_REG_FPSCR, fpscr)
    trace = []

    def memory(uc, access, address, size, value, _):
        if access == UC_MEM_READ:
            value = int.from_bytes(uc.mem_read(address, size), 'little')
        trace.append((access, address, size, value))

    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=0x40000000, end=0x400fffff)
    entry = F(0x21a00) if original else symbols['board_adc_init']
    u.emu_start(entry | 1, 0x30000, count=10000)
    assert u.reg_read(arm.UC_ARM_REG_PC) == 0x30000
    return (trace, tuple(bytes(u.mem_read(address, 0x1000))
                         for address in pages),
            u.reg_read(arm.UC_ARM_REG_FPSCR))


adc_pages = (0x40040000, 0x40048000, 0x4004c000, 0x40053000,
             0x40054000)
for case in range(256):
    pages = {address: bytes(rng.getrandbits(8) for _ in range(0x1000))
             for address in adc_pages}
    fpscr = rng.getrandbits(32)
    a = run_adc_init(True, pages, fpscr)
    b = run_adc_init(False, pages, fpscr)
    assert a == b, (case, a[0], b[0])
print('PASS: 256 ADC initialization cases; complete ordered MMIO address/'
      'width/value trace, final peripheral pages and FPSCR')


def run_sampling_timer(original, pages, nvic, fpscr):
    u = machine(original)
    u.mem_map(0x40000000, 0x100000)
    u.mem_map(0xe0000000, 0x100000)
    for address, data in pages.items():
        u.mem_write(address, bytes(data))
    u.mem_write(0xe000e000, nvic)
    u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    u.reg_write(arm.UC_ARM_REG_FPSCR, fpscr)
    trace = []

    def memory(uc, access, address, size, value, _):
        if access == UC_MEM_READ:
            value = int.from_bytes(uc.mem_read(address, size), 'little')
        trace.append((access, address, size, value))

    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=0x40000000, end=0x400fffff)
    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=0xe000e000, end=0xe000efff)
    entry = F(0x22fac) if original else symbols['board_sampling_timer_init']
    u.emu_start(entry | 1, 0x30000, count=10000)
    assert u.reg_read(arm.UC_ARM_REG_PC) == 0x30000
    return (trace, tuple(bytes(u.mem_read(address, 0x1000))
                         for address in pages),
            bytes(u.mem_read(0xe000e000, 0x1000)),
            u.reg_read(arm.UC_ARM_REG_FPSCR))


timer_pages = (0x40038000, 0x40048000, 0x40051000, 0x40053000)
for case in range(256):
    pages = {address: bytes(rng.getrandbits(8) for _ in range(0x1000))
             for address in timer_pages}
    nvic = bytes(rng.getrandbits(8) for _ in range(0x1000))
    fpscr = rng.getrandbits(32)
    a = run_sampling_timer(True, pages, nvic, fpscr)
    b = run_sampling_timer(False, pages, nvic, fpscr)
    assert a == b, (case, a[0], b[0])
print('PASS: 256 sampling/PWM timer initialization cases; complete ordered '
      'GPIO/TMR4/INTC/NVIC access trace, final state and FPSCR')


def position_spi_responses(expected_image, observed, readback, entropy):
    values = []
    for value in observed:
        values.extend((entropy.getrandbits(16),
                       (value << 8) | entropy.getrandbits(8)))
    for index, value in enumerate(readback):
        if observed[index] != expected_image[index]:
            values.extend((entropy.getrandbits(16),
                           (value << 8) | entropy.getrandbits(8)))
    return values


def run_position_init(original, pages, nvic, expected_image, responses,
                      fpscr):
    u = machine(original)
    u.mem_map(0x40000000, 0x100000)
    u.mem_map(0xe0000000, 0x100000)
    for address, data in pages.items():
        u.mem_write(address, bytes(data))
    u.mem_write(0xe000e000, nvic)
    u.mem_write(A(0x1fffa65c), expected_image)
    u.mem_write(A(0x1ffff3bc), b'\xa5' * 11)
    u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    u.reg_write(arm.UC_ARM_REG_FPSCR, fpscr)
    trace = []
    queued = list(responses)
    delay = F(0x21f28) if original else symbols['board_delay_ms']

    def memory(uc, access, address, size, value, _):
        if access == UC_MEM_READ:
            if address == 0x40020000:
                assert queued
                value = queued.pop(0)
                uc.mem_write(address, value.to_bytes(4, 'little'))
            else:
                value = int.from_bytes(uc.mem_read(address, size), 'little')
        trace.append((access, address, size, value))

    def code(uc, address, size, _):
        if address == delay:
            trace.append(('delay_ms', uc.reg_read(arm.UC_ARM_REG_R0)))
            uc.reg_write(arm.UC_ARM_REG_PC, uc.reg_read(arm.UC_ARM_REG_LR))

    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=0x40000000, end=0x400fffff)
    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=0xe000e000, end=0xe000efff)
    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=A(0x1fffa65c), end=A(0x1fffa667))
    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=A(0x1ffff3bc), end=A(0x1ffff3c6))
    u.hook_add(UC_HOOK_CODE, code)
    entry = F(0x22c10) if original else symbols['board_position_init']
    u.emu_start(entry | 1, 0x30000, count=100000)
    assert u.reg_read(arm.UC_ARM_REG_PC) == 0x30000
    assert not queued, len(queued)
    return (trace, tuple(bytes(u.mem_read(address, 0x1000))
                         for address in pages),
            bytes(u.mem_read(0xe000e000, 0x1000)),
            bytes(u.mem_read(A(0x1fffa65c), 12)),
            bytes(u.mem_read(A(0x1ffff3bc), 11)),
            u.reg_read(arm.UC_ARM_REG_FPSCR))


position_pages = (0x40010000, 0x40020000, 0x40048000, 0x40051000,
                  0x40053000)
for case in range(256):
    pages = {address: bytearray(rng.getrandbits(8) for _ in range(0x1000))
             for address in position_pages}
    pages[0x40020000][0x14] |= 0xa0
    expected = bytes(rng.getrandbits(8) for _ in range(12))
    if case == 0:
        expected = bytes((0x00, 0x00, 0x00, 0x00, 0xc0, 0xff,
                          0x1c, 0x00, 0x77, 0x9c, 0x0e, 0x00))
    observed = bytearray(rng.getrandbits(8) for _ in range(11))
    for index in range(10):
        if (case >> (index % 8)) & 1:
            observed[index] = expected[index]
    readback = bytes(rng.getrandbits(8) for _ in range(10))
    responses = position_spi_responses(expected, observed, readback, rng)
    nvic = bytes(rng.getrandbits(8) for _ in range(0x1000))
    fpscr = rng.getrandbits(32)
    a = run_position_init(True, pages, nvic, expected, responses, fpscr)
    b = run_position_init(False, pages, nvic, expected, responses, fpscr)
    assert a == b, (case, a[0], b[0])
print('PASS: 256 position-sensor initialization cases; complete ordered '
      'SPI/GPIO/DMA/PWC/AOS/INTC/NVIC/fixed-SRAM trace, sensor negotiation, '
      'final state and FPSCR')


def run_uart_init(original, pages, nvic, static_sram, fpscr):
    u = machine(original)
    u.mem_map(0x40000000, 0x100000)
    u.mem_map(0xe0000000, 0x100000)
    for address, data in pages.items():
        u.mem_write(address, data)
    u.mem_write(0xe000e000, nvic)
    u.mem_write(0x20000000, static_sram)
    u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    u.reg_write(arm.UC_ARM_REG_R0, 921600 if original else 0)
    u.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
    u.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    u.reg_write(arm.UC_ARM_REG_FPSCR, fpscr)
    trace = []

    def memory(uc, access, address, size, value, _):
        if access == UC_MEM_READ:
            value = int.from_bytes(uc.mem_read(address, size), 'little')
        trace.append((access, address, size, value))

    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=0x40000000, end=0x400fffff)
    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=0xe000e000, end=0xe000efff)
    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=0x1fff0000, end=0x1fffffff)
    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=0x20000000, end=0x2000dfff)
    entry = F(0x2359c) if original else symbols['board_uart_init']
    try:
        u.emu_start(entry | 1, 0x30000, count=100000)
    except Exception:
        print('UART emulation failure', original,
              hex(u.reg_read(arm.UC_ARM_REG_PC)))
        raise
    assert u.reg_read(arm.UC_ARM_REG_PC) == 0x30000
    return (trace, tuple(bytes(u.mem_read(address, 0x1000))
                         for address in pages),
            bytes(u.mem_read(0xe000e000, 0x1000)),
            bytes(u.mem_read(0x20000000, 0xe000)),
            u.reg_read(arm.UC_ARM_REG_FPSCR))


uart_pages = (0x40010000, 0x4001c000, 0x40024000, 0x40048000,
              0x40051000, 0x40053000)
for case in range(256):
    pages = {address: bytes(rng.getrandbits(8) for _ in range(0x1000))
             for address in uart_pages}
    nvic = bytes(rng.getrandbits(8) for _ in range(0x1000))
    static_sram = bytes(rng.getrandbits(8) for _ in range(0xe000))
    fpscr = rng.getrandbits(32) & 0xf7c0009f
    a = run_uart_init(True, pages, nvic, static_sram, fpscr)
    b = run_uart_init(False, pages, nvic, static_sram, fpscr)
    assert a == b, (case, a[0], b[0])
print('PASS: 256 UART1/DMA1 initialization cases; complete ordered USART/'
      'timer/GPIO/DMA/PWC/AOS/INTC/NVIC/SRAM trace, baud arithmetic, final '
      'state and FPSCR')


def run_mcan_init(original, fd_enabled, selector, node_id, pages, nvic,
                  static_sram, fpscr):
    u = machine(original)
    u.mem_map(0x40000000, 0x100000)
    u.mem_map(0xe0000000, 0x100000)
    for address, data in pages.items():
        u.mem_write(address, bytes(data))
    u.mem_write(0xe000e000, nvic)
    u.mem_write(0x20000000, static_sram)
    u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    u.reg_write(arm.UC_ARM_REG_R0, selector)
    u.reg_write(arm.UC_ARM_REG_R1, node_id)
    u.reg_write(arm.UC_ARM_REG_FPSCR, fpscr)
    trace = []

    def memory(uc, access, address, size, value, _):
        if access == UC_MEM_READ:
            value = int.from_bytes(uc.mem_read(address, size), 'little')
        trace.append((access, address, size, value))

    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=0x40000000, end=0x400fffff)
    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=0xe000e000, end=0xe000efff)
    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=0x1fff0000, end=0x1fffffff)
    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=0x20000000, end=0x2000dfff)
    if original:
        entry = F(0x21fc4 if fd_enabled else 0x21cc8)
    else:
        entry = symbols[
            'board_mcan_init_fd' if fd_enabled else 'board_mcan_init_classic'
        ]
    u.emu_start(entry | 1, 0x30000, count=200000)
    assert u.reg_read(arm.UC_ARM_REG_PC) == 0x30000, (
        original, fd_enabled, hex(u.reg_read(arm.UC_ARM_REG_PC)))
    return (trace, tuple(bytes(u.mem_read(address, 0x1000))
                         for address in pages),
            bytes(u.mem_read(0xe000e000, 0x1000)),
            bytes(u.mem_read(0x20000000, 0xe000)),
            u.reg_read(arm.UC_ARM_REG_FPSCR))


mcan_pages = (0x40029000, 0x4002b000, 0x40048000, 0x40051000,
              0x40053000, 0x40054000)
mcan_selectors = (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                  0xffff)
for fd_enabled in (False, True):
    for case in range(256):
        pages = {address: bytearray(rng.getrandbits(8)
                                    for _ in range(0x1000))
                 for address in mcan_pages}
        pages[0x40029000][0x18] &= ~0x18
        nvic = bytes(rng.getrandbits(8) for _ in range(0x1000))
        static_sram = bytes(rng.getrandbits(8) for _ in range(0xe000))
        fpscr = rng.getrandbits(32)
        selector = (mcan_selectors[case]
                    if case < len(mcan_selectors) else rng.getrandbits(16))
        node_id = rng.getrandbits(16)
        a = run_mcan_init(True, fd_enabled, selector, node_id, pages, nvic,
                          static_sram, fpscr)
        b = run_mcan_init(False, fd_enabled, selector, node_id, pages, nvic,
                          static_sram, fpscr)
        assert a == b, (fd_enabled, case, selector, node_id, a[0], b[0])
print('PASS: 256 classic plus 256 FD MCAN initialization cases; complete '
      'ordered GPIO/clock/controller/message-RAM/INTC/NVIC/SRAM trace, all '
      'selector classes, final state and FPSCR')


def run_board_clock(original, pages, nvic, static_sram, fpscr):
    u = machine(original)
    u.mem_map(0x40000000, 0x100000)
    u.mem_map(0xe0000000, 0x100000)
    for address, data in pages.items():
        u.mem_write(address, bytes(data))
    u.mem_write(0xe000e000, nvic)
    u.mem_write(0x20000000, static_sram)
    u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    u.reg_write(arm.UC_ARM_REG_FPSCR, fpscr)
    trace = []

    def memory(uc, access, address, size, value, _):
        if access == UC_MEM_READ:
            value = int.from_bytes(uc.mem_read(address, size), 'little')
        trace.append((access, address, size, value))

    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=0x40000000, end=0x400fffff)
    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=0xe000e000, end=0xe000efff)
    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=0x1fff0000, end=0x1fffffff)
    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
               begin=0x20000000, end=0x2000dfff)
    entry = F(0x23208) if original else symbols['board_clock_init']
    u.emu_start(entry | 1, 0x30000, count=200000)
    assert u.reg_read(arm.UC_ARM_REG_PC) == 0x30000, (
        original, hex(u.reg_read(arm.UC_ARM_REG_PC)))
    return (trace, tuple(bytes(u.mem_read(address, 0x1000))
                         for address in pages),
            bytes(u.mem_read(0xe000e000, 0x1000)),
            bytes(u.mem_read(0x20000000, 0xe000)),
            u.reg_read(arm.UC_ARM_REG_FPSCR))


clock_pages = (0x40010000, 0x4003a000, 0x40048000, 0x4004c000,
               0x40050000, 0x40054000)
for case in range(256):
    pages = {address: bytearray(rng.getrandbits(8)
                                for _ in range(0x1000))
             for address in clock_pages}
    pages[0x40054000][0x3c] |= 0x28
    if case & 1:
        pages[0x40054000][0x26] = 5
    else:
        source = case % 5
        pages[0x40054000][0x26] = (
            pages[0x40054000][0x26] & ~7) | source
    nvic = bytes(rng.getrandbits(8) for _ in range(0x1000))
    static_sram = bytes(rng.getrandbits(8) for _ in range(0xe000))
    fpscr = rng.getrandbits(32)
    a = run_board_clock(True, pages, nvic, static_sram, fpscr)
    b = run_board_clock(False, pages, nvic, static_sram, fpscr)
    if a != b:
        difference = next((index for index, pair in enumerate(zip(a[0], b[0]))
                           if pair[0] != pair[1]),
                          min(len(a[0]), len(b[0])))
        raise AssertionError((case, difference,
                              a[0][difference:difference + 4],
                              b[0][difference:difference + 4]))
print('PASS: 256 board-clock initialization cases; both incoming clock '
      'sources, complete ordered clock/PWC/SRAMC/EFM/SysTick/TMRA/NVIC/SRAM '
      'trace, final state and FPSCR')
