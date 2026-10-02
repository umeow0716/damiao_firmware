from pathlib import Path
import random

from unicorn import (UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE,
                     UC_MEM_READ)

exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split(
    '\nfor old, name, size in mapping[:1]:')[0])

FACTORY_ENTRY = F(0x24b20)
SOURCE_FUNCTION = symbols['commissioning_run_direction_and_alignment']
SOURCE_ENTRY = SOURCE_FUNCTION + (0x190 if model == 'dm8009' else 0x192)
ANGLE = A(0x1ffff198)
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
factory_angle_sites = {F(0x24b9a): 0, F(0x24c38): 1, F(0x24d84): 2}
source_angle_sites = {
    SOURCE_FUNCTION + (0x1f0 if model == 'dm8009' else 0x1f4): 0,
    SOURCE_FUNCTION + (0x25c if model == 'dm8009' else 0x260): 1,
    SOURCE_FUNCTION + (0x34c if model == 'dm8009' else 0x354): 2,
}


def alignment_machine(original):
    uc = machine(original)
    uc.mem_map(0x40000000, 0x100000)
    uc.mem_map(0xe0000000, 0x100000)
    if original:
        uc.mem_write(FIXED_IMAGE_BASE, factory[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE])
    else:
        for section in elf.iter_sections():
            if (A(0x1fff0000) <= section['sh_addr'] < 0x20000000 and
                    section['sh_type'] != 'SHT_NOBITS'):
                uc.mem_write(section['sh_addr'], section.data())
    return uc


def run(original, runtime, sine, peripherals, angle_values, fpscr):
    uc = alignment_machine(original)
    uc.mem_write(A(0x1ffff000), runtime)
    uc.mem_write(A(0x1fffa674), sine)
    uc.mem_write(0x40040000, peripherals)
    uc.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
    uc.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    uc.reg_write(arm.UC_ARM_REG_FPSCR, fpscr)
    uc.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    if original:
        uc.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    else:
        # State after the combined source worker's direction prefix.
        uc.reg_write(arm.UC_ARM_REG_SP, 0x2000efa8)
        uc.reg_write(arm.UC_ARM_REG_R5, A(0x1ffff23c))
        uc.reg_write(arm.UC_ARM_REG_R6, A(0x1ffff23c))
        uc.reg_write(arm.UC_ARM_REG_S18, 0x40c90fdb)
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
        elif original and address == F(0x24b68):
            # Keep the factory-derived step for one pole, but execute one
            # report point in each direction instead of all 256.
            emu.reg_write(arm.UC_ARM_REG_R6, 1)
        elif not original and address == SOURCE_FUNCTION + (
                0x1ca if model == 'dm8009' else 0x1ce):
            emu.reg_write(arm.UC_ARM_REG_R9, 1)
        if address == (FACTORY_AUTH if original else SOURCE_AUTH):
            events.append(('auth',))
            return_from_stub(emu)
        elif address in pwm_sites:
            events.append(('pwm', emu.reg_read(arm.UC_ARM_REG_S0),
                           emu.reg_read(arm.UC_ARM_REG_S1)))
            return_from_stub(emu)
        elif address == (FACTORY_DELAY if original else SOURCE_DELAY):
            events.append(('delay', emu.reg_read(arm.UC_ARM_REG_R0)))
            return_from_stub(emu)
        elif address == (FACTORY_UART if original else SOURCE_UART):
            pointer = emu.reg_read(arm.UC_ARM_REG_R0)
            length = emu.reg_read(arm.UC_ARM_REG_R1)
            events.append(('uart', length,
                           bytes(emu.mem_read(pointer, length))))
            return_from_stub(emu)
        elif address == (F(0x24e6a) if original else
                         SOURCE_FUNCTION + 0x10):
            stopped = True
            emu.emu_stop()

    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=A(0x1fff0000), end=A(0x1fffffff))
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=0x40040000, end=0x400407ff)
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=0xe000e100, end=0xe000e2ff)
    uc.hook_add(UC_HOOK_CODE, code)
    entry = FACTORY_ENTRY if original else SOURCE_ENTRY
    uc.emu_start(entry | 1, 0x30000, count=2000000)
    assert stopped, (original, hex(uc.reg_read(arm.UC_ARM_REG_PC)),
                     events[-32:])
    return (
        events,
        bytes(uc.mem_read(A(0x1ffff000), len(runtime))),
        bytes(uc.mem_read(0x40040000, len(peripherals))),
        bytes(uc.mem_read(0xe000e100, 0x200)),
        uc.reg_read(arm.UC_ARM_REG_FPSCR),
    )


rng = random.Random(0x24b20)
for case in range(8):
    runtime = bytearray(rng.getrandbits(8) for _ in range(0x300))
    runtime[0xdc:0xe0] = (1).to_bytes(4, 'little')
    runtime[0x24c:0x250] = [0, 0x40000000, 0x7f800000,
                            0x7fc12345][case % 4].to_bytes(4, 'little')
    runtime[0x190:0x192] = rng.getrandbits(16).to_bytes(2, 'little')
    sine = bytes(rng.getrandbits(8) for _ in range(0x2004))
    peripherals = bytes(rng.getrandbits(8) for _ in range(0x800))
    angle_values = (0, 0x3dcccccd if case < 4 else 0xbdcccccd,
                    0xbd4ccccd if case < 4 else 0x3d4ccccd)
    fpscr = (case & 3) << 22
    factory_result = run(True, bytes(runtime), sine, peripherals,
                         angle_values, fpscr)
    source_result = run(False, bytes(runtime), sine, peripherals,
                        angle_values, fpscr)
    if factory_result != source_result:
        factory_events, source_events = factory_result[0], source_result[0]
        print('mismatch case', case, 'event lengths', len(factory_events),
              len(source_events))
        for index, pair in enumerate(zip(factory_events, source_events)):
            if pair[0] != pair[1]:
                print('first event', index, pair)
                print('factory context', factory_events[max(0, index - 6):
                                                        index + 7])
                print('source context', source_events[max(0, index - 6):
                                                      index + 7])
                break
        for component, (left, right) in enumerate(
                zip(factory_result[1:], source_result[1:]), 1):
            if left != right:
                if isinstance(left, (bytes, bytearray)):
                    differences = [offset for offset, values in enumerate(
                                   zip(left, right))
                                   if values[0] != values[1]]
                    print('component', component, 'first byte differences',
                          differences[:16])
                else:
                    print('component', component, left, right)
        raise AssertionError(case)

print('PASS: 8 factory 0x24b20/source alignment executions; full 20,000-step '
      'lock plus one forward/reverse report, all fixed SRAM/peripheral '
      'traces, frames, helper ABI and FPSCR')
