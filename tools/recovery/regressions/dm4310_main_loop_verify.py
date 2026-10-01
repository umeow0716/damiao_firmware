from pathlib import Path
import random

from unicorn import (UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE,
                     UC_MEM_READ)

exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split(
    '\nfor old, name, size in mapping[:1]:')[0])

FACTORY_LOOP = F(0x2540e)
SOURCE_LOOP = symbols['main'] + 0x60
FACTORY_PRINTF = F(0x20474)
SOURCE_PRINTF = symbols['debug_console_printf']
FACTORY_DELAY_US = F(0x21f60)
SOURCE_DELAY_US = symbols['platform_commissioning_delay_us']
SOURCE_WRITE = symbols['platform_debug_write']
FLASH_WRITER = A(0x1fff9950)
FACTORY_LOAD_CAL = F(0x22658)
SOURCE_LOAD_CAL = symbols['platform_load_motor_calibration']
FACTORY_DIRECTION = F(0x264a4)
FACTORY_ALIGNMENT = F(0x24b20)
SOURCE_DIRECTION = symbols['commissioning_run_direction_and_alignment']
FACTORY_MOTOR_ID = F(0x257c4)
SOURCE_MOTOR_ID = symbols['commissioning_run_motor_identification']
FACTORY_FIRMWARE = F(0x26d00)
SOURCE_FIRMWARE = symbols['firmware_control_service']
FACTORY_OUTPUT = F(0x246f8)
SOURCE_OUTPUT = symbols['commissioning_run_output_sensor_calibration']
STATE_BASE = FACTORY_STATE_BASE
STATE_SIZE = 0x20000000 - STATE_BASE
EVENTS = A(0x1ffff1f0)
MOTOR = A(0x1ffff088)
SAMPLE = A(0x1ffff104)


def read_c_string(uc, pointer):
    result = bytearray()
    while len(result) < 256:
        value = bytes(uc.mem_read(pointer + len(result), 1))[0]
        if value == 0:
            return bytes(result)
        result.append(value)
    raise AssertionError('unterminated string')


def loop_machine(original):
    uc = machine(original)
    uc.mem_map(0x40000000, 0x100000)
    uc.mem_map(0xe0000000, 0x100000)
    if original:
        uc.mem_write(FIXED_IMAGE_BASE, factory[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE])
    else:
        for section in elf.iter_sections():
            if (A(0x1fff0000) <= section['sh_addr'] < STATE_BASE and
                    section['sh_type'] != 'SHT_NOBITS'):
                uc.mem_write(section['sh_addr'], section.data())
    return uc


def put_word(image, address, value):
    offset = address - STATE_BASE
    image[offset:offset + 4] = (value & 0xffffffff).to_bytes(4, 'little')


def run(original, state, peripheral, system, primask):
    uc = loop_machine(original)
    uc.mem_write(STATE_BASE, state)
    uc.mem_write(0x40000000, peripheral)
    uc.mem_write(0xe0000000, system)
    uc.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
    uc.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    uc.reg_write(arm.UC_ARM_REG_SP, 0x2000eff0)
    uc.reg_write(arm.UC_ARM_REG_PRIMASK, primask)
    if original:
        uc.reg_write(arm.UC_ARM_REG_R4, 0)
        uc.reg_write(arm.UC_ARM_REG_R5, MOTOR)
        uc.reg_write(arm.UC_ARM_REG_R6, EVENTS)
        uc.reg_write(arm.UC_ARM_REG_R7, SAMPLE)
        uc.reg_write(arm.UC_ARM_REG_R8, 1)
        uc.reg_write(arm.UC_ARM_REG_R9, 0xe000e000)
        uc.reg_write(arm.UC_ARM_REG_R10, 4)
        uc.reg_write(arm.UC_ARM_REG_R11, 0x4005382a)
        uc.reg_write(arm.UC_ARM_REG_S16, 0)
    else:
        uc.reg_write(arm.UC_ARM_REG_R4, EVENTS)
        uc.reg_write(arm.UC_ARM_REG_R5, 0x20001600)
        uc.mem_write(0x2000eff4, (0x200016bc).to_bytes(4, 'little'))
    events = []
    loop_visits = 0

    def memory(emu, access, address, size, value, _):
        if access == UC_MEM_READ:
            value = int.from_bytes(emu.mem_read(address, size), 'little')
            events.append(('r', address, size, value))
        else:
            events.append(('w', address, size,
                           value & ((1 << (size * 8)) - 1)))

    def return_from_stub(emu):
        emu.reg_write(arm.UC_ARM_REG_PC, emu.reg_read(arm.UC_ARM_REG_LR))

    def code(emu, address, size, _):
        nonlocal loop_visits
        loop_entries = {FACTORY_LOOP if original else SOURCE_LOOP}
        if address in loop_entries:
            loop_visits += 1
            if loop_visits == 2:
                emu.emu_stop()
                return

        if address == (FACTORY_PRINTF if original else SOURCE_PRINTF):
            events.append(('text', read_c_string(
                emu, emu.reg_read(arm.UC_ARM_REG_R0))))
            return_from_stub(emu)
        elif not original and address == SOURCE_WRITE:
            pointer = emu.reg_read(arm.UC_ARM_REG_R0)
            length = emu.reg_read(arm.UC_ARM_REG_R1)
            events.append(('text', bytes(emu.mem_read(pointer, length))))
            return_from_stub(emu)
        elif address == (FACTORY_DELAY_US if original else SOURCE_DELAY_US):
            events.append(('delay-us', emu.reg_read(arm.UC_ARM_REG_R0)))
            return_from_stub(emu)
        elif address == FLASH_WRITER:
            events.append(('flash-write',
                           emu.reg_read(arm.UC_ARM_REG_R0),
                           emu.reg_read(arm.UC_ARM_REG_R1),
                           emu.reg_read(arm.UC_ARM_REG_R2)))
            return_from_stub(emu)
        elif address == (FACTORY_LOAD_CAL if original else SOURCE_LOAD_CAL):
            events.append(('load-calibration-0x22658',))
            return_from_stub(emu)
        elif address == (FACTORY_DIRECTION if original else SOURCE_DIRECTION):
            events.append(('direction-and-alignment',))
            return_from_stub(emu)
        elif original and address == FACTORY_ALIGNMENT:
            return_from_stub(emu)
        elif address == (FACTORY_MOTOR_ID if original else SOURCE_MOTOR_ID):
            events.append(('motor-identification',))
            return_from_stub(emu)
        elif address == (FACTORY_FIRMWARE if original else SOURCE_FIRMWARE):
            events.append(('firmware-control',
                           emu.reg_read(arm.UC_ARM_REG_R0) & 0xff))
            return_from_stub(emu)
        elif address == (FACTORY_OUTPUT if original else SOURCE_OUTPUT):
            events.append(('output-calibration',))
            return_from_stub(emu)

    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=STATE_BASE, end=A(0x1fffffff))
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=0x40000000, end=0x400fffff)
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=0xe000e000, end=0xe000efff)
    uc.hook_add(UC_HOOK_CODE, code)
    uc.emu_start((FACTORY_LOOP if original else SOURCE_LOOP) | 1,
                 0x30000, count=3000000)
    assert loop_visits == 2, (original, hex(uc.reg_read(arm.UC_ARM_REG_PC)))
    return (
        events,
        bytes(uc.mem_read(STATE_BASE, STATE_SIZE)),
        bytes(uc.mem_read(0x40000000, 0x100000)),
        bytes(uc.mem_read(0xe000e000, 0x1000)),
        uc.reg_read(arm.UC_ARM_REG_PRIMASK),
    )


cases = [
    {},
    {'mode_pending': 1, 'mode': 0},
    {'mode_pending': 1, 'mode': 2},
    {'tick': 1, 'fault': 1, 'ticks': 249},
    {'tick': 1, 'fault': 2, 'ticks': 250},
    {'can_error': 1},
    {'can_error': 2},
    {'save': 1},
    {'reserved': 0x12345678},
    {'direction': 1},
    {'calibration': 1},
    {'calibration': 2},
    {'calibration': 3},
    {'motor_id': 1},
    {'firmware': 0x1234},
    {'output': 1},
    {'mode_pending': 1, 'mode': 0, 'tick': 1, 'fault': 2, 'ticks': 250,
     'can_error': 1, 'save': 1, 'reserved': 1, 'direction': 1,
     'calibration': 2, 'motor_id': 1, 'firmware': 0x1234, 'output': 1},
]

rng = random.Random(0x252f4)
for case_index, values in enumerate(cases):
    state = bytearray(rng.getrandbits(8) for _ in range(STATE_SIZE))
    for address in range(EVENTS, EVENTS + 0x4c, 4):
        put_word(state, address, 0)
    put_word(state, MOTOR + 0x38, values.get('mode', 0))
    put_word(state, MOTOR + 0x3c, values.get('mode_pending', 0))
    put_word(state, SAMPLE + 0x80, values.get('fault', 0))
    put_word(state, EVENTS + 0x08, values.get('reserved', 0))
    put_word(state, EVENTS + 0x0c, values.get('save', 0))
    put_word(state, EVENTS + 0x10, values.get('direction', 0))
    put_word(state, EVENTS + 0x14, values.get('output', 0))
    put_word(state, EVENTS + 0x1c, values.get('calibration', 0))
    put_word(state, EVENTS + 0x24, values.get('motor_id', 0))
    put_word(state, EVENTS + 0x28, values.get('firmware', 0))
    put_word(state, EVENTS + 0x2c, values.get('tick', 0))
    put_word(state, EVENTS + 0x30, values.get('can_error', 0))
    put_word(state, EVENTS + 0x48, values.get('ticks', 0))
    peripheral = bytes(rng.getrandbits(8) for _ in range(0x100000))
    system = bytes(rng.getrandbits(8) for _ in range(0x100000))
    primask = case_index & 1
    factory_result = run(True, bytes(state), peripheral, system, primask)
    source_result = run(False, bytes(state), peripheral, system, primask)
    if factory_result != source_result:
        print('MISMATCH', case_index, values)
        for component, (factory_part, source_part) in enumerate(
                zip(factory_result, source_result)):
            if factory_part == source_part:
                continue
            if component == 0:
                print('event lengths', len(factory_part), len(source_part))
                for index, pair in enumerate(zip(factory_part, source_part)):
                    if pair[0] != pair[1]:
                        print('first event', index)
                        print('factory', factory_part[max(0,index-8):index+12])
                        print('source ', source_part[max(0,index-8):index+12])
                        break
            elif isinstance(factory_part, bytes):
                differences = [i for i, pair in enumerate(
                               zip(factory_part, source_part))
                               if pair[0] != pair[1]]
                print('component', component, 'differences', differences[:64])
            else:
                print('component', component, factory_part, source_part)
        raise AssertionError(case_index)
    print('PASS', case_index)

print('PASS: 17 factory 0x252f4/source deferred-event loop matrices; all '
      'single events, simultaneous priority, fixed SRAM/MMIO order, child '
      'ABI, final state and PRIMASK')
