from pathlib import Path
import random

from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_READ

exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split(
    '\nfor old, name, size in mapping[:1]:')[0])

FACTORY_ENTRY = F(0x24448)
SOURCE_FUNCTION = symbols['commissioning_run_output_sensor_calibration']
SOURCE_ENTRY = SOURCE_FUNCTION + (0x1a0 if is_dm800x else 0x1a8)
SOURCE_BOUND_SITE = SOURCE_FUNCTION + 0x1e4
SOURCE_STOP = SOURCE_FUNCTION + 0x234
FACTORY_PWM = {F(0x219b0)}
SOURCE_PWM = {
    symbol['st_value'] & ~1
    for symbol in elf.get_section_by_name('.symtab').iter_symbols()
    if symbol.name == '__svpwm_helper_veneer'
}
SOURCE_PWM.add(symbols['board_sampling_timer_write_space_vector'])
FACTORY_DELAY = F(0x21f60)
SOURCE_DELAY = symbols['platform_commissioning_delay_us']
FACTORY_UART = F(0x2379c)
SOURCE_UART = symbols['platform_debug_write']


def cluster_machine(original):
    uc = machine(original)
    uc.mem_map(0x40000000, 0x100000)
    if original:
        uc.mem_write(FIXED_IMAGE_BASE, factory[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE])
    else:
        for section in elf.iter_sections():
            if (A(0x1fff0000) <= section['sh_addr'] < 0x20000000 and
                    section['sh_type'] != 'SHT_NOBITS'):
                uc.mem_write(section['sh_addr'], section.data())
    return uc


def put_word(image, base, address, value):
    offset = address - base
    image[offset:offset + 4] = value.to_bytes(4, 'little')


def run(original, fixed, sine, peripheral, fpscr):
    uc = cluster_machine(original)
    uc.mem_write(A(0x1ffff000), fixed)
    uc.mem_write(A(0x1fffa674), sine)
    uc.mem_write(0x40000000, peripheral)
    uc.mem_write(0x40040044, (1).to_bytes(4, 'little'))
    uc.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
    uc.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    uc.reg_write(arm.UC_ARM_REG_FPSCR, fpscr)
    uc.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    if original:
        uc.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    else:
        # Stack/register state after the combined source worker's outer pass.
        uc.reg_write(arm.UC_ARM_REG_SP, 0x2000ef58)
        # The combined source worker keeps the canonical 0x1ffff000 base in
        # r4; its fields themselves are translated into the V6417 layout.
        uc.reg_write(arm.UC_ARM_REG_R4, 0x1ffff000)
        uc.reg_write(arm.UC_ARM_REG_S19, 0x40c90fdb)
    events = []
    stopped = False

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
        if original and address == F(0x2448e):
            # Retain the factory-derived electrical step but execute one
            # complete report iteration after the full 20,000-step lock.
            emu.reg_write(arm.UC_ARM_REG_R7, 1)
        elif not original and address == SOURCE_BOUND_SITE:
            emu.reg_write(arm.UC_ARM_REG_R5, 1)
        if address in (FACTORY_PWM if original else SOURCE_PWM):
            events.append(('pwm', emu.reg_read(arm.UC_ARM_REG_S0),
                           emu.reg_read(arm.UC_ARM_REG_S1)))
            return_from_stub(emu)
        elif address == (FACTORY_DELAY if original else SOURCE_DELAY):
            events.append(('delay_us', emu.reg_read(arm.UC_ARM_REG_R0)))
            return_from_stub(emu)
        elif address == (FACTORY_UART if original else SOURCE_UART):
            pointer = emu.reg_read(arm.UC_ARM_REG_R0)
            length = emu.reg_read(arm.UC_ARM_REG_R1)
            events.append(('uart', length,
                           bytes(emu.mem_read(pointer, length))))
            return_from_stub(emu)
        elif not original and address == SOURCE_STOP:
            stopped = True
            emu.emu_stop()

    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=A(0x1fff0000), end=A(0x1fffffff))
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=0x40000000, end=0x400fffff)
    uc.hook_add(UC_HOOK_CODE, code)
    entry = FACTORY_ENTRY if original else SOURCE_ENTRY
    uc.emu_start(entry | 1, 0x30000, count=2500000)
    if original:
        assert uc.reg_read(arm.UC_ARM_REG_PC) == 0x30000
    else:
        assert stopped, hex(uc.reg_read(arm.UC_ARM_REG_PC))
    return (
        events,
        bytes(uc.mem_read(A(0x1ffff000), len(fixed))),
        bytes(uc.mem_read(0x40040000, 0x800)),
        uc.reg_read(arm.UC_ARM_REG_FPSCR),
    )


rng = random.Random(0x24448)
float_cases = (
    (0x3f800000, 1, 0x3f000000, 1000, 3000),
    (0x40000000, 7, 0xbf000000, 2000, 2100),
    (0x40b00000, 14, 0x00000000, 4095, 0),
    (0x41100000, 21, 0x7f800000, 123, 3987),
    (0x3f000000, 2, 0xff800000, 3500, 700),
    (0x40400000, 10, 0x3f800000, 2048, 2048),
    (0x3fc00000, 5, 0x80000000, 3000, 1000),
    (0x40800000, 12, 0x40000000, 17, 4079),
)
for case, (gear, poles, voltage, raw_u, raw_v) in enumerate(float_cases):
    fixed = bytearray(rng.getrandbits(8) for _ in range(0x300))
    for address, value in (
            (A(0x1ffff0d4), gear),
            (A(0x1ffff0dc), poles),
            (A(0x1ffff24c), voltage),
            (A(0x1ffff1a8), 0x447a0000),
            (A(0x1ffff1ac), 0x45000000),
            (A(0x1ffff1b0), 0x3f000000),
            (A(0x1ffff1b4), 0x3f000000),
            (A(0x1ffff1bc), 0x45000000),
            (A(0x1ffff1c0), 0x447a0000),
            (A(0x1ffff1c4), 0x45000000),
            (A(0x1ffff1d0), 0x3f800000),
            (A(0x1ffff1d4), 0x3f000000),
            (A(0x1ffff1d8), 0x3f5db3d7)):
        put_word(fixed, A(0x1ffff000), address, value)
    sine = bytes(rng.getrandbits(8) for _ in range(0x2004))
    peripheral = bytearray(rng.getrandbits(8) for _ in range(0x100000))
    peripheral[0x40054:0x40056] = raw_u.to_bytes(2, 'little')
    peripheral[0x40454:0x40456] = raw_v.to_bytes(2, 'little')
    fpscr = (case & 3) << 22
    factory_result = run(True, bytes(fixed), sine, bytes(peripheral), fpscr)
    source_result = run(False, bytes(fixed), sine, bytes(peripheral), fpscr)
    if factory_result != source_result:
        print('mismatch case', case)
        for component, (factory_component, source_component) in enumerate(
                zip(factory_result, source_result)):
            if factory_component == source_component:
                continue
            if component == 0:
                print('event lengths', len(factory_component),
                      len(source_component))
                for index, (factory_event, source_event) in enumerate(
                        zip(factory_component, source_component)):
                    if factory_event != source_event:
                        print('first event', index, factory_event, source_event)
                        print('factory context', factory_component[max(0, index - 8):index + 8])
                        print('source context', source_component[max(0, index - 8):index + 8])
                        break
            elif isinstance(factory_component, bytes):
                differences = [index for index, (a, b) in enumerate(
                               zip(factory_component, source_component))
                               if a != b]
                print('component', component, 'byte diffs', differences[:32])
            else:
                print('component', component, factory_component, source_component)
        raise AssertionError('factory/source mismatch')

print('PASS: 8 factory 0x24448/source offset-sweep executions; full 20,000-step '
      'lock plus one complete filtered sample/report, fixed SRAM/MMIO traces, '
      'CRC frames, helper ABI and FPSCR')
