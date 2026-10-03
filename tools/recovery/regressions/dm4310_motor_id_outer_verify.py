from pathlib import Path
import random
import struct

from unicorn import (UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE,
                     UC_MEM_READ)

exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split(
    '\nfor old, name, size in mapping[:1]:')[0])

FACTORY_ENTRY = F(0x257c4)
SOURCE_ENTRY = symbols['commissioning_run_motor_identification']

FACTORY_AUTH = F(0x26eb8)
SOURCE_AUTH = symbols['platform_require_device_authentication']
FACTORY_DELAY_US = F(0x21f60)
FACTORY_DELAY_MS = F(0x21f28)
SOURCE_DELAY_US = symbols['platform_commissioning_delay_us']
PWM = A(0x1fff9c40)
FACTORY_UART = F(0x2379c)
SOURCE_UART = symbols['platform_debug_write']
FACTORY_PRINTF = F(0x20474)
SOURCE_PRINTF = symbols['debug_console_printf']

# Current-build loop cut points.  These hooks modify only loop counters; every
# selected loop body and its real helper calls still execute.
FACTORY_LOCK_END = F(0x25856)
FACTORY_ELECTRICAL_WAIT = F(0x258b8)
FACTORY_ELECTRICAL_END = F(0x25ab6)
FACTORY_FLUX_WAIT = F(0x25b9c)
FACTORY_FLUX_WINDOW_INC = F(0x25c86)
FACTORY_FLUX_END = F(0x25df0)
FACTORY_MECHANICAL_WAIT = F(0x25f2a)
FACTORY_MECHANICAL_PHASE_INC = F(0x25f32)
FACTORY_MECHANICAL_WINDOW_INC = F(0x25f6e)
FACTORY_MECHANICAL_END = F(0x26222)
FACTORY_COAST_WAIT = F(0x2622c)
FACTORY_COAST_END = F(0x2637c)

SOURCE_LOCK_END = SOURCE_ENTRY + 0x3a
SOURCE_ELECTRICAL_WAIT = SOURCE_ENTRY + 0x88
SOURCE_ELECTRICAL_END = SOURCE_ENTRY + 0x178
SOURCE_FLUX_WAIT = SOURCE_ENTRY + (0x250 if is_dm800x else 0x252)
SOURCE_FLUX_END = SOURCE_ENTRY + (0x33e if is_dm800x else 0x346)
SOURCE_MECHANICAL_WAIT = SOURCE_ENTRY + (0x47a if is_dm800x else 0x486)
SOURCE_MECHANICAL_END = SOURCE_ENTRY + (0x58e if is_dm800x else 0x5e8)
SOURCE_COAST_WAIT = SOURCE_ENTRY + (0x5e8 if is_dm800x else 0x5f2)
SOURCE_COAST_END = SOURCE_ENTRY + (0x652 if is_dm800x else 0x65c)

STATE_BASE = FACTORY_STATE_BASE
STATE_END = A(0x1ffff500)
STAGING = A(0x1fffa5c8)
CACHE = A(0x1ffff23c)


def fbits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def put_word(image, address, value):
    offset = address - STATE_BASE
    image[offset:offset + 4] = (value & 0xffffffff).to_bytes(4, 'little')


def outer_machine(original):
    uc = machine(original)
    uc.mem_map(0x40000000, 0x100000)
    uc.mem_map(0xe0000000, 0x100000)
    if original:
        uc.mem_write(FIXED_IMAGE_BASE, factory[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE])
    else:
        for section in elf.iter_sections():
            address = section['sh_addr']
            if (A(0x1fff0000) <= address < STATE_BASE and
                    section['sh_type'] != 'SHT_NOBITS'):
                uc.mem_write(address, section.data())
    return uc


def read_c_string(uc, pointer):
    data = bytearray()
    while len(data) < 256:
        value = bytes(uc.mem_read(pointer + len(data), 1))[0]
        if value == 0:
            return bytes(data)
        data.append(value)
    raise AssertionError('unterminated string')


def run(original, state, peripheral, system, electrical_count, flux_count,
        mechanical_phase, mechanical_window, fpscr, fit_override=None):
    uc = outer_machine(original)
    uc.mem_write(STATE_BASE, state)
    uc.mem_write(0x40000000, peripheral)
    uc.mem_write(0xe0000000, system)
    uc.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
    uc.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    uc.reg_write(arm.UC_ARM_REG_FPSCR, fpscr)
    uc.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    uc.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    uc.reg_write(arm.UC_ARM_REG_R0, STAGING)
    events = []
    writes = []
    visits = {
        'electrical': 0,
        'flux': 0,
        'mechanical': 0,
        'coast': 0,
    }

    def return_from_stub(emu):
        emu.reg_write(arm.UC_ARM_REG_PC, emu.reg_read(arm.UC_ARM_REG_LR))

    def memory(emu, access, address, size, value, _):
        if access == UC_MEM_READ:
            writes.append(('r', address, size, int.from_bytes(
                emu.mem_read(address, size), 'little')))
        else:
            writes.append(('w', address, size,
                           value & ((1 << (size * 8)) - 1)))

    def code(emu, address, size, _):
        if address == (FACTORY_AUTH if original else SOURCE_AUTH):
            events.append(('auth',))
            return_from_stub(emu)
            return
        if original and address == FACTORY_DELAY_US:
            events.append(('delay-us', emu.reg_read(arm.UC_ARM_REG_R0)))
            return_from_stub(emu)
            return
        if original and address == FACTORY_DELAY_MS:
            events.append(('delay-us',
                           emu.reg_read(arm.UC_ARM_REG_R0) * 1000))
            return_from_stub(emu)
            return
        if not original and address == SOURCE_DELAY_US:
            events.append(('delay-us', emu.reg_read(arm.UC_ARM_REG_R0)))
            return_from_stub(emu)
            return
        if address == PWM:
            events.append(('pwm', emu.reg_read(arm.UC_ARM_REG_S0),
                           emu.reg_read(arm.UC_ARM_REG_S1)))
            fit_return = (F(0x25ad4) if original else SOURCE_ENTRY + 0x19c)
            if fit_override is not None and (
                    emu.reg_read(arm.UC_ARM_REG_LR) & ~1) == fit_return:
                estimator = emu.reg_read(arm.UC_ARM_REG_SP) + (
                    0xc8 if original else 0x94)
                coefficient_a, coefficient_b = fit_override
                emu.mem_write(estimator + 12,
                              fbits(coefficient_a).to_bytes(4, 'little'))
                emu.mem_write(estimator + 16,
                              fbits(coefficient_b).to_bytes(4, 'little'))
            return_from_stub(emu)
            return
        if address == (FACTORY_UART if original else SOURCE_UART):
            pointer = emu.reg_read(arm.UC_ARM_REG_R0)
            length = emu.reg_read(arm.UC_ARM_REG_R1)
            events.append(('uart', bytes(emu.mem_read(pointer, length))))
            return_from_stub(emu)
            return
        if address == (FACTORY_PRINTF if original else SOURCE_PRINTF):
            events.append(('printf', read_c_string(
                emu, emu.reg_read(arm.UC_ARM_REG_R0))))
            return_from_stub(emu)
            return

        if original:
            if address == FACTORY_LOCK_END:
                emu.reg_write(arm.UC_ARM_REG_R6, 6283)
            elif address == FACTORY_ELECTRICAL_WAIT:
                visits['electrical'] += 1
                emu.reg_write(arm.UC_ARM_REG_R5, electrical_count - 1)
            elif address == FACTORY_ELECTRICAL_END:
                emu.reg_write(arm.UC_ARM_REG_R6, 59999)
            elif address == FACTORY_FLUX_WAIT:
                visits['flux'] += 1
                window = flux_count % 20
                emu.reg_write(arm.UC_ARM_REG_R5, 19 if window == 0
                              else window - 1)
            elif address == FACTORY_FLUX_END:
                emu.reg_write(arm.UC_ARM_REG_R6, 39999)
            elif address == FACTORY_MECHANICAL_WAIT:
                visits['mechanical'] += 1
                emu.reg_write(arm.UC_ARM_REG_R4, mechanical_phase - 1)
                emu.reg_write(arm.UC_ARM_REG_R5,
                              19 if mechanical_window else 0)
            elif address == FACTORY_MECHANICAL_END:
                emu.reg_write(arm.UC_ARM_REG_R6, 79999)
            elif address == FACTORY_COAST_WAIT:
                visits['coast'] += 1
            elif address == FACTORY_COAST_END:
                emu.reg_write(arm.UC_ARM_REG_R4, 19999)
        else:
            if address == SOURCE_LOCK_END:
                emu.reg_write(arm.UC_ARM_REG_R4, 1)
            elif address == SOURCE_ELECTRICAL_WAIT:
                visits['electrical'] += 1
                emu.reg_write((arm.UC_ARM_REG_R5 if is_dm800x else
                               arm.UC_ARM_REG_R4), electrical_count)
            elif address == SOURCE_ELECTRICAL_END:
                emu.reg_write((arm.UC_ARM_REG_R5 if is_dm800x else
                               arm.UC_ARM_REG_R4), 60000)
            elif address == SOURCE_FLUX_WAIT:
                visits['flux'] += 1
                emu.reg_write((arm.UC_ARM_REG_R5 if is_dm800x else
                               arm.UC_ARM_REG_R4), flux_count)
            elif address == SOURCE_FLUX_END:
                emu.reg_write((arm.UC_ARM_REG_R5 if is_dm800x else
                               arm.UC_ARM_REG_R4), 40000)
            elif address == SOURCE_MECHANICAL_WAIT:
                visits['mechanical'] += 1
                emu.reg_write(arm.UC_ARM_REG_R5, mechanical_phase)
                emu.reg_write(arm.UC_ARM_REG_R4,
                              19 if mechanical_window else 0)
            elif address == SOURCE_MECHANICAL_END:
                emu.reg_write(arm.UC_ARM_REG_R6, 1)
            elif address == SOURCE_COAST_WAIT:
                visits['coast'] += 1
            elif address == SOURCE_COAST_END:
                emu.reg_write(arm.UC_ARM_REG_R4, 1)

    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=STATE_BASE, end=STATE_END - 1)
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=0x40000000, end=0x400fffff)
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=0xe000e000, end=0xe000efff)
    uc.hook_add(UC_HOOK_CODE, code)
    uc.emu_start((FACTORY_ENTRY if original else SOURCE_ENTRY) | 1,
                 0x30000, count=5000000)
    assert uc.reg_read(arm.UC_ARM_REG_PC) == 0x30000, (
        original, hex(uc.reg_read(arm.UC_ARM_REG_PC)), visits)
    expected_visits = ({'electrical': 1, 'flux': 0, 'mechanical': 0,
                        'coast': 0} if fit_override is not None else
                       {name: 1 for name in visits})
    assert visits == expected_visits, (original, visits)
    return (
        events,
        writes,
        bytes(uc.mem_read(STATE_BASE, STATE_END - STATE_BASE)),
        bytes(uc.mem_read(0x40000000, 0x100000)),
        bytes(uc.mem_read(0xe000e000, 0x1000)),
        uc.reg_read(arm.UC_ARM_REG_FPSCR),
    )


def describe_difference(factory_result, source_result):
    for component, (factory_part, source_part) in enumerate(
            zip(factory_result, source_result)):
        if factory_part == source_part:
            continue
        if component in (0, 1):
            print('component', component, 'lengths', len(factory_part),
                  len(source_part))
            for index, pair in enumerate(zip(factory_part, source_part)):
                if pair[0] != pair[1]:
                    print('first difference', index)
                    print('factory', factory_part[max(0, index-8):index+12])
                    print('source ', source_part[max(0, index-8):index+12])
                    break
        elif isinstance(factory_part, bytes):
            differences = [index for index, pair in enumerate(
                           zip(factory_part, source_part))
                           if pair[0] != pair[1]]
            print('component', component, 'differences', differences[:80])
        else:
            print('component', component, factory_part, source_part)


rng = random.Random(0x257c4)
cases = [
    # no cadence branch, no completed mechanical sine cycle
    (1, 1, 1, False),
    # electrical target-only update
    (10, 2, 2, False),
    # simultaneous ramp/target update and pre-window flux sample
    (100, 19, 9999, False),
    # final ramp update, observer windows, cycle threshold boundary
    (10000, 20, 10000, True),
    # RLS threshold is strict (> 20000)
    (20000, 1, 10001, False),
    # electrical target update/RLS, flux and mechanical 20-sample windows,
    # and completed-cycle projection
    (20001, 20, 10001, True),
]
for case_index, (electrical_count, flux_count, mechanical_phase,
                 mechanical_window) in enumerate(cases):
    state = bytearray(STATE_END - STATE_BASE)
    # Persistent staging values used by the identification and derivation.
    staging_values = {
        0: fbits(10.0), 11: fbits(0.001), 12: fbits(0.002),
        16: 7, 17: fbits(0.2), 18: fbits(0.001), 19: fbits(0.02),
        20: fbits(6.0), 24: fbits(100.0), 25: fbits(0.3),
        26: fbits(0.02), 27: fbits(1.0), 28: fbits(0.1),
        31: fbits(0.9), 32: fbits(200.0), 33: fbits(1.0),
    }
    for index, value in staging_values.items():
        put_word(state, STAGING + index * 4, value)
    cache_values = {
        0: fbits(0.01), 2: fbits(1.0), 3: fbits(0.00005),
        5: fbits(0.1), 6: fbits(2.0), 7: fbits(0.3),
        8: fbits(0.2), 9: fbits(1.0), 10: fbits(2.0),
        14: fbits(100.0), 17: fbits(0.2), 18: fbits(0.001),
    }
    for index, value in cache_values.items():
        put_word(state, CACHE + index * 4, value)
    put_word(state, A(0x1ffff0dc), 7)
    put_word(state, A(0x1ffff0f0), fbits(0.7991513609886169))
    put_word(state, A(0x1ffff0f4), fbits(0.20084863901138306))
    put_word(state, A(0x1ffff144), fbits(1000.0))
    put_word(state, A(0x1ffff148), fbits(1100.0))
    put_word(state, A(0x1ffff194), 0)
    put_word(state, A(0x1ffff198), fbits(0.25))
    put_word(state, A(0x1ffff1a0), fbits(0.01))

    peripheral = bytearray(0x100000)
    peripheral[0x40044] = 1
    peripheral[0x40050:0x40052] = (900).to_bytes(2, 'little')
    peripheral[0x40450:0x40452] = (1050).to_bytes(2, 'little')
    peripheral[0x40850:0x40852] = (1200).to_bytes(2, 'little')
    peripheral[0x40052:0x40054] = (2400).to_bytes(2, 'little')
    system = bytes(0x100000)
    fpscr = (case_index & 3) << 22

    factory_result = run(True, bytes(state), bytes(peripheral), system,
                         electrical_count, flux_count, mechanical_phase,
                         mechanical_window, fpscr)
    source_result = run(False, bytes(state), bytes(peripheral), system,
                        electrical_count, flux_count, mechanical_phase,
                        mechanical_window, fpscr)
    if factory_result != source_result:
        print('MISMATCH case', case_index, cases[case_index])
        describe_difference(factory_result, source_result)
        raise AssertionError(case_index)
    print('PASS', case_index)

for failure_name, fit_override in [
        ('negative-resistance', (2.0, 1.0)),
        ('negative-inductance', (2.0, -1.0))]:
    factory_result = run(True, bytes(state), bytes(peripheral), system,
                         1, 1, 1, False, 0, fit_override)
    source_result = run(False, bytes(state), bytes(peripheral), system,
                        1, 1, 1, False, 0, fit_override)
    if factory_result != source_result:
        print('MISMATCH', failure_name)
        describe_difference(factory_result, source_result)
        raise AssertionError(failure_name)
    assert ('printf', b'error!/r/n') in factory_result[0]
    assert ('delay-us', 5000) not in factory_result[0][6:]
    print('PASS', failure_name)

print('PASS: shortened whole factory 0x257c4/source motor-identification '
      'flows; all stage bodies, cadence/failure branches, fixed SRAM/MMIO, '
      'result frame, derivation, epilogue and FPSCR')
