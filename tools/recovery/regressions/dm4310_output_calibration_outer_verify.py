from pathlib import Path
import random
import struct

from unicorn import (UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE,
                     UC_MEM_READ)

exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split(
    '\nfor old, name, size in mapping[:1]:')[0])

FACTORY_ENTRY = F(0x246f8)
SOURCE_ENTRY = symbols['commissioning_run_output_sensor_calibration']
FACTORY_CHILD = F(0x24448)
SOURCE_CHILD = SOURCE_ENTRY + (0x1a0 if is_dm800x else 0x1a8)
SOURCE_AFTER_CHILD = SOURCE_ENTRY + (0x21e if is_dm800x else 0x226)
FACTORY_SAMPLE = F(0x247a8)
SOURCE_SAMPLE = SOURCE_ENTRY + 0x60
FACTORY_COMPARE = F(0x248c2)
SOURCE_COMPARE = SOURCE_ENTRY + (0x12a if is_dm800x else 0x132)
PWM = A(0x1fff9c40)
FACTORY_UART = F(0x2379c)
SOURCE_UART = symbols['platform_debug_write']
FACTORY_DELAY_MS = F(0x21f28)
SOURCE_DELAY_MS = symbols['platform_delay_ms']
FACTORY_PRINTF = F(0x20474)
SOURCE_PRINTF = symbols['debug_console_printf']
STATE_BASE = FACTORY_STATE_BASE
STATE_END = A(0x1ffff4f8)
MOTOR_POSITION = A(0x1ffff0a0)
POSITION_READY = A(0x1ffff194)
POSITION_ANGLE = A(0x1ffff198)


def fbits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def read_c_string(uc, pointer):
    data = bytearray()
    while len(data) < 256:
        byte = bytes(uc.mem_read(pointer + len(data), 1))[0]
        if byte == 0:
            return bytes(data)
        data.append(byte)
    raise AssertionError('unterminated string')


def outer_machine(original):
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


def run(original, state, peripheral, system, samples, fpscr):
    uc = outer_machine(original)
    uc.mem_write(STATE_BASE, state)
    uc.mem_write(0x40000000, peripheral)
    uc.mem_write(0xe0000000, system)
    uc.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
    uc.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    uc.reg_write(arm.UC_ARM_REG_FPSCR, fpscr)
    uc.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    uc.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    events = []
    sample_index = 0
    source_child_tail = False

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
        nonlocal sample_index, source_child_tail
        instruction = bytes(emu.mem_read(address, min(size, 4)))
        if instruction == bytes.fromhex('bff34f8f'):
            events.append(('dsb',))
        elif instruction == bytes.fromhex('bff36f8f'):
            events.append(('isb',))

        if address == (FACTORY_SAMPLE if original else SOURCE_SAMPLE):
            raw_u, raw_v, position, angle = samples[sample_index]
            emu.mem_write(0x40040054, raw_u.to_bytes(2, 'little'))
            emu.mem_write(0x40040454, raw_v.to_bytes(2, 'little'))
            emu.mem_write(MOTOR_POSITION, position.to_bytes(4, 'little'))
            emu.mem_write(POSITION_ANGLE, angle.to_bytes(4, 'little'))
            emu.mem_write(POSITION_READY, (1).to_bytes(4, 'little'))
            sample_index += 1
        elif address == (FACTORY_COMPARE if original else SOURCE_COMPARE):
            if sample_index == len(samples):
                target_register = (arm.UC_ARM_REG_S23 if original else
                                   arm.UC_ARM_REG_S17)
                emu.mem_write(
                    MOTOR_POSITION,
                    emu.reg_read(target_register).to_bytes(4, 'little'))

        if address == (FACTORY_CHILD if original else SOURCE_CHILD):
            events.append(('offset-child-0x24448',))
            if original:
                return_from_stub(emu)
            else:
                source_child_tail = True
                emu.reg_write(arm.UC_ARM_REG_PC, SOURCE_AFTER_CHILD | 1)
        elif address == PWM:
            if not original and source_child_tail:
                source_child_tail = False
            else:
                events.append(('pwm', emu.reg_read(arm.UC_ARM_REG_S0),
                               emu.reg_read(arm.UC_ARM_REG_S1)))
            return_from_stub(emu)
        elif address == (FACTORY_UART if original else SOURCE_UART):
            pointer = emu.reg_read(arm.UC_ARM_REG_R0)
            length = emu.reg_read(arm.UC_ARM_REG_R1)
            events.append(('uart', length,
                           bytes(emu.mem_read(pointer, length))))
            return_from_stub(emu)
        elif address == (FACTORY_DELAY_MS if original else SOURCE_DELAY_MS):
            events.append(('delay-ms', emu.reg_read(arm.UC_ARM_REG_R0)))
            return_from_stub(emu)
        elif address == (FACTORY_PRINTF if original else SOURCE_PRINTF):
            stack = emu.reg_read(arm.UC_ARM_REG_SP)
            events.append(('printf',
                           read_c_string(emu, emu.reg_read(arm.UC_ARM_REG_R0)),
                           emu.reg_read(arm.UC_ARM_REG_R2),
                           emu.reg_read(arm.UC_ARM_REG_R3),
                           bytes(emu.mem_read(stack, 24))))
            return_from_stub(emu)

    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=A(0x1fff0000), end=A(0x1fffffff))
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=0x40000000, end=0x400fffff)
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=0xe000e000, end=0xe000efff)
    uc.hook_add(UC_HOOK_CODE, code)
    uc.emu_start((FACTORY_ENTRY if original else SOURCE_ENTRY) | 1,
                 0x30000, count=2000000)
    assert uc.reg_read(arm.UC_ARM_REG_PC) == 0x30000
    assert sample_index == len(samples)
    return (
        events,
        bytes(uc.mem_read(STATE_BASE, len(state))),
        bytes(uc.mem_read(0x40040000, 0x800)),
        bytes(uc.mem_read(0xe000e000, 0x1000)),
        uc.reg_read(arm.UC_ARM_REG_FPSCR),
    )


rng = random.Random(0x246f8)
for case in range(8):
    state = bytearray(rng.getrandbits(8)
                      for _ in range(STATE_END - STATE_BASE))
    start = (-1.25 + case * 0.125)
    put_word(state, MOTOR_POSITION, fbits(start))
    put_word(state, POSITION_READY, 1)
    put_word(state, POSITION_ANGLE, fbits(0.125 * case))
    put_word(state, A(0x1ffff0dc), [1, 2, 5, 14][case & 3])
    put_word(state, A(0x1ffff0e0), fbits(0.5 + 0.25 * (case & 3)))
    put_word(state, A(0x1ffff09c), fbits(-0.2 + 0.05 * case))
    put_word(state, A(0x1ffff1a8), fbits(800.0 + 20.0 * case))
    put_word(state, A(0x1ffff1ac), fbits(1200.0 + 30.0 * case))
    put_word(state, A(0x1ffff1b0), fbits(0.75))
    put_word(state, A(0x1ffff1b4), fbits(0.25))
    put_word(state, A(0x1ffff1bc), fbits(1500.0))
    put_word(state, A(0x1ffff1c4), fbits(1700.0))
    put_word(state, A(0x1ffff1d0), fbits(1.0))

    peripheral = bytearray(rng.getrandbits(8) for _ in range(0x100000))
    status = int.from_bytes(peripheral[0x40044:0x40048], 'little') | 1
    peripheral[0x40044:0x40048] = status.to_bytes(4, 'little')
    initial_u = 900 + case * 13
    initial_v = 1800 - case * 11
    peripheral[0x40054:0x40056] = initial_u.to_bytes(2, 'little')
    peripheral[0x40454:0x40456] = initial_v.to_bytes(2, 'little')
    system = bytes(rng.getrandbits(8) for _ in range(0x100000))
    samples = []
    for index in range(20):
        raw_u = (300 + index * 137 + case * 17) & 0xfff
        raw_v = (3700 - index * 149 - case * 19) & 0xfff
        position = fbits(start + index * 0.2)
        angle = fbits(-2.0 + index * 0.2)
        samples.append((raw_u, raw_v, position, angle))
    fpscr = (case & 3) << 22

    factory_result = run(True, bytes(state), bytes(peripheral), system,
                         samples, fpscr)
    source_result = run(False, bytes(state), bytes(peripheral), system,
                        samples, fpscr)
    if factory_result != source_result:
        print('MISMATCH', case)
        for component, (factory_part, source_part) in enumerate(
                zip(factory_result, source_result)):
            if factory_part == source_part:
                continue
            if component == 0:
                print('event lengths', len(factory_part), len(source_part))
                for index, (factory_event, source_event) in enumerate(
                        zip(factory_part, source_part)):
                    if factory_event != source_event:
                        print('first event', index)
                        print('factory', factory_part[max(0, index-10):index+16])
                        print('source ', source_part[max(0, index-10):index+16])
                        break
            elif isinstance(factory_part, bytes):
                differences = [index for index, (a, b) in enumerate(
                               zip(factory_part, source_part)) if a != b]
                print('component', component, 'differences', differences[:64])
            else:
                print('component', component, factory_part, source_part)
        raise AssertionError(case)
    print('PASS', case)

print('PASS: 8 complete factory 0x246f8/source output-calibration outer '
      'flows; 20 extrema samples and h/J/Z frames, composed 0x24448 child, '
      'diagnostic ABI, ADC/NVIC epilogue, ordered fixed SRAM/MMIO and FPSCR')
