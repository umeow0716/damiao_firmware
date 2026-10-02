from pathlib import Path
import random

from unicorn import (UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE,
                     UC_MEM_READ)

exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split(
    '\nfor old, name, size in mapping[:1]:')[0])

FACTORY_ENTRY = F(0x264a4)
SOURCE_ENTRY = symbols['commissioning_run_direction_and_alignment']
ANGLE = A(0x1ffff198)
CORRECTION = A(0x1fffc84c)
FACTORY_AUTH = F(0x26eb8)
SOURCE_AUTH = symbols['platform_require_device_authentication']
FACTORY_PWM = {0x219b0}
SOURCE_PWM = {
    symbol['st_value'] & ~1
    for symbol in elf.get_section_by_name('.symtab').iter_symbols()
    if symbol.name == '__dm4310_svpwm_helper_veneer'
}
SOURCE_PWM.add(symbols['board_sampling_timer_write_space_vector'])
FACTORY_DELAY = F(0x21f60)
SOURCE_DELAY = symbols['platform_commissioning_delay_us']
FACTORY_UART = F(0x2379c)
SOURCE_UART = symbols['platform_debug_write']
SOURCE_POSITION_SET = symbols['position_sensor_set_inverted']
SOURCE_DIRECTION_DONE = SOURCE_ENTRY + (0x190 if model == 'dm8009' else 0x192)

factory_angle_sites = {
    F(0x26510): 0,
    F(0x2658c): 1,
    F(0x26632): 2,
}
source_angle_sites = {
    SOURCE_ENTRY + (0x64 if model == 'dm8009' else 0x66): 0,
    SOURCE_ENTRY + (0x92 if model == 'dm8009' else 0x94): 1,
    SOURCE_ENTRY + (0x118 if model == 'dm8009' else 0x11a): 2,
}


def direction_machine(original):
    uc = machine(original)
    uc.mem_map(0xe0000000, 0x100000)
    if original:
        uc.mem_write(FIXED_IMAGE_BASE, factory[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE])
    else:
        for section in elf.iter_sections():
            if (A(0x1fff0000) <= section['sh_addr'] < 0x20000000 and
                    section['sh_type'] != 'SHT_NOBITS'):
                uc.mem_write(section['sh_addr'], section.data())
    return uc


def run(original, config, runtime, staging, sine, correction, angle_values,
        fpscr):
    uc = direction_machine(original)
    uc.mem_write(A(0x1ffff000), runtime)
    uc.mem_write(A(0x1fffa5c8), staging)
    uc.mem_write(A(0x1fffa674), sine)
    uc.mem_write(CORRECTION, correction)
    source_config = 0x20008000
    uc.mem_write(source_config, config)
    uc.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
    uc.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    uc.reg_write(arm.UC_ARM_REG_FPSCR, fpscr)
    uc.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    uc.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    uc.reg_write(arm.UC_ARM_REG_R0, source_config)
    events = []
    stopped = False
    angle_sites = factory_angle_sites if original else source_angle_sites
    pwm_sites = FACTORY_PWM if original else SOURCE_PWM

    def memory(emu, access, address, size, value, _):
        if access == UC_MEM_READ:
            value = int.from_bytes(emu.mem_read(address, size), 'little')
            events.append(('read', address, size, value))
        else:
            events.append(('write', address, size, value))

    def return_from_stub(emu):
        emu.reg_write(arm.UC_ARM_REG_PC, emu.reg_read(arm.UC_ARM_REG_LR))

    def code(emu, address, size, _):
        nonlocal stopped
        if address in angle_sites:
            emu.mem_write(ANGLE,
                          angle_values[angle_sites[address]].to_bytes(4,
                                                                     'little'))
        elif address == (FACTORY_AUTH if original else SOURCE_AUTH):
            events.append(('auth',))
            return_from_stub(emu)
        elif address in pwm_sites:
            events.append(('pwm', emu.reg_read(arm.UC_ARM_REG_S0),
                           emu.reg_read(arm.UC_ARM_REG_S1)))
            return_from_stub(emu)
        elif address == (FACTORY_DELAY if original else SOURCE_DELAY):
            events.append(('delay', emu.reg_read(arm.UC_ARM_REG_R0)))
            return_from_stub(emu)
        elif not original and address == SOURCE_POSITION_SET:
            return_from_stub(emu)
        elif address == (FACTORY_UART if original else SOURCE_UART):
            pointer = emu.reg_read(arm.UC_ARM_REG_R0)
            length = emu.reg_read(arm.UC_ARM_REG_R1)
            events.append(('uart', length,
                           bytes(emu.mem_read(pointer, length))))
            return_from_stub(emu)
        elif not original and address == SOURCE_DIRECTION_DONE:
            stopped = True
            emu.emu_stop()

    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=A(0x1fff0000), end=A(0x1fffffff))
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=0xe000e100, end=0xe000e2ff)
    uc.hook_add(UC_HOOK_CODE, code)
    entry = FACTORY_ENTRY if original else SOURCE_ENTRY
    uc.emu_start(entry | 1, 0x30000, count=500000)
    if original:
        assert uc.reg_read(arm.UC_ARM_REG_PC) == 0x30000
    else:
        assert stopped
    return (
        events,
        bytes(uc.mem_read(A(0x1ffff000), len(runtime))),
        bytes(uc.mem_read(A(0x1fffa5c8), len(staging))),
        bytes(uc.mem_read(CORRECTION, len(correction))),
        uc.reg_read(arm.UC_ARM_REG_FPSCR),
    )


rng = random.Random(0x264a4)
voltage_words = [0, 0x40000000, 0x7f800000, 0x7fc12345]
for case in range(8):
    runtime = bytearray(rng.getrandbits(8) for _ in range(0x300))
    staging = bytearray(rng.getrandbits(8) for _ in range(0x60))
    sine = bytes(rng.getrandbits(8) for _ in range(0x2004))
    correction = bytes(rng.getrandbits(8) for _ in range(0x400))
    config = bytes(rng.getrandbits(8) for _ in range(0x100))
    runtime[0x24c:0x250] = voltage_words[case % 4].to_bytes(4, 'little')
    staging[0x50:0x54] = rng.getrandbits(32).to_bytes(4, 'little')
    quarter = 0x3fc90fdb if case < 4 else 0xbfc90fdb
    angle_values = (0, quarter, 0)
    fpscr = (case & 3) << 22
    factory_result = run(True, config, bytes(runtime), bytes(staging), sine,
                         correction, angle_values, fpscr)
    source_result = run(False, config, bytes(runtime), bytes(staging), sine,
                        correction, angle_values, fpscr)
    if factory_result != source_result:
        print('direction case', case)
        for component, (factory_part, source_part) in enumerate(
                zip(factory_result, source_result)):
            if factory_part == source_part:
                continue
            if isinstance(factory_part, list):
                print('component', component, 'event lengths',
                      len(factory_part), len(source_part))
                for index, pair in enumerate(zip(factory_part, source_part)):
                    if pair[0] != pair[1]:
                        print('first event', index, pair)
                        print('factory context', factory_part[max(0, index - 4):index + 5])
                        print('source context', source_part[max(0, index - 4):index + 5])
                        break
            else:
                print('component', component, factory_part, source_part)
        raise AssertionError('direction mismatch')

print('PASS: 8 complete factory 0x264a4/source direction workers through '
      'reply; full 10,000-step lock, both direction arms, fixed SRAM/NVIC '
      'trace, helper ABI and FPSCR')


def run_finish(original, runtime, travel, direction, fpscr):
    uc = direction_machine(original)
    uc.mem_write(A(0x1ffff000), runtime)
    uc.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
    uc.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    uc.reg_write(arm.UC_ARM_REG_FPSCR, fpscr)
    uc.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    uc.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    events = []
    stopped = False

    def memory(emu, access, address, size, value, _):
        if access == UC_MEM_READ:
            value = int.from_bytes(emu.mem_read(address, size), 'little')
            events.append(('read', address, size, value))
        else:
            events.append(('write', address, size, value))

    def code(emu, address, size, _):
        nonlocal stopped
        if original and address == F(0x2665e):
            stopped = True
            emu.emu_stop()

    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=A(0x1ffff000), end=A(0x1ffff2ff))
    uc.hook_add(UC_HOOK_CODE, code)
    if original:
        uc.reg_write(arm.UC_ARM_REG_S17, travel)
        uc.reg_write(arm.UC_ARM_REG_S21, 0x40c90fdb)
        uc.reg_write(arm.UC_ARM_REG_S24, direction)
        uc.reg_write(arm.UC_ARM_REG_R6, A(0x1ffff088))
        entry = F(0x26652)
    else:
        uc.reg_write(arm.UC_ARM_REG_S0, travel)
        uc.reg_write(arm.UC_ARM_REG_S1, direction)
        entry = symbols['direction_finish_count']
    uc.emu_start(entry | 1, 0x30000, count=10000)
    assert stopped if original else uc.reg_read(arm.UC_ARM_REG_PC) == 0x30000
    return (events, bytes(uc.mem_read(A(0x1ffff000), len(runtime))),
            uc.reg_read(arm.UC_ARM_REG_R0),
            uc.reg_read(arm.UC_ARM_REG_FPSCR))


def run_publish(original, runtime, staging, poles, fpscr):
    uc = direction_machine(original)
    uc.mem_write(A(0x1ffff000), runtime)
    uc.mem_write(A(0x1fffa5c8), staging)
    uc.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
    uc.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    uc.reg_write(arm.UC_ARM_REG_FPSCR, fpscr)
    uc.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    uc.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    uc.reg_write(arm.UC_ARM_REG_R0, poles)
    events = []
    stopped = False

    def memory(emu, access, address, size, value, _):
        if access == UC_MEM_READ:
            value = int.from_bytes(emu.mem_read(address, size), 'little')
            events.append(('read', address, size, value))
        else:
            events.append(('write', address, size, value))

    def code(emu, address, size, _):
        nonlocal stopped
        if original and address == F(0x2668a):
            stopped = True
            emu.emu_stop()

    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=A(0x1ffff000), end=A(0x1ffff2ff))
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=A(0x1fffa5c8), end=A(0x1fffa627))
    uc.hook_add(UC_HOOK_CODE, code)
    if original:
        uc.reg_write(arm.UC_ARM_REG_R6, A(0x1ffff088))
        uc.reg_write(arm.UC_ARM_REG_S21, 0x40c90fdb)
        entry = F(0x2665e)
    else:
        entry = symbols['direction_publish_pole_count']
    uc.emu_start(entry | 1, 0x30000, count=10000)
    assert stopped if original else uc.reg_read(arm.UC_ARM_REG_PC) == 0x30000
    return (events, bytes(uc.mem_read(A(0x1ffff000), len(runtime))),
            bytes(uc.mem_read(A(0x1fffa5c8), len(staging))),
            uc.reg_read(arm.UC_ARM_REG_FPSCR))


float_edges = [0, 0x80000000, 0x3fc90fdb, 0x40c90fdb, 0x7f800000,
               0xff800000, 0x7fc00001, 0xffc00001]
pole_edges = [0, 1, 10, 0x01000001, 0x7fffffff, 0x80000000,
              0xfffffffe, 0xffffffff]
for case in range(256):
    runtime = bytearray(rng.getrandbits(8) for _ in range(0x300))
    staging = bytearray(rng.getrandbits(8) for _ in range(0x60))
    travel = float_edges[case] if case < len(float_edges) else rng.getrandbits(32)
    direction = 0x3f800000 if case & 1 else 0x40000000
    fpscr = (case & 3) << 22
    factory_result = run_finish(True, bytes(runtime), travel, direction, fpscr)
    source_result = run_finish(False, bytes(runtime), travel, direction, fpscr)
    assert factory_result == source_result, ('finish', case, factory_result,
                                              source_result)

    poles = pole_edges[case] if case < len(pole_edges) else rng.getrandbits(32)
    staging[0x50:0x54] = rng.getrandbits(32).to_bytes(4, 'little')
    factory_result = run_publish(True, bytes(runtime), bytes(staging), poles,
                                 fpscr)
    source_result = run_publish(False, bytes(runtime), bytes(staging), poles,
                                fpscr)
    assert factory_result == source_result, ('publish', case, factory_result,
                                              source_result)

print('PASS: 256 direction-finish plus 256 pole-publication edge/random '
      'cases; ordered fixed SRAM, result values and FPSCR match')
