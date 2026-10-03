import os
import struct
from pathlib import Path

from unicorn import (UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE,
                     UC_MEM_WRITE)

exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split(
    '\nfor old, name, size in mapping[:1]:')[0])

FACTORY_ENTRY = A(0x1fff8137)
SOURCE_ENTRY = A(0x1fff8137)
STOP = 0xe000e280
COUNT_INSTRUCTIONS = os.environ.get('DAMIAO_COUNT_INSTRUCTIONS') == '1'
instruction_counts = {True: [], False: []}

def put_u32(u, address, value):
    u.mem_write(address, struct.pack('<I', value))

def put_f32(u, address, value):
    u.mem_write(address, struct.pack('<f', value))

for case in range(96):
    relocated = case >= 48
    status = 0x20007000 if relocated else A(0x1ffff1f0)
    config = 0x20007200 if relocated else A(0x1fffa5c8)
    sample = 0x20007400 if relocated else A(0x1ffff104)
    motor = 0x20007600 if relocated else A(0x1ffff088)
    speed = 0x20007800 if relocated else A(0x1fffa568)
    temp = 0x20007a00 if relocated else A(0x1fffa560)
    raw = 0x20007c00 if relocated else A(0x1fffc778)
    mode = case % 6
    tick = (case // 6) & 1
    enabled = (case // 12) & 1
    results = []
    for original in (True, False):
        u = machine(original)
        if original:
            u.mem_write(FIXED_IMAGE_BASE, factory[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE])
        else:
            for section in elf.iter_sections():
                if (FIXED_IMAGE_BASE <= section['sh_addr'] < FACTORY_STATE_BASE and
                        section['sh_type'] != 'SHT_NOBITS'):
                    u.mem_write(section['sh_addr'], section.data())
        for page in (0x40038000, 0x40040000, 0xe000e000):
            u.mem_map(page, 0x1000)

        # State reached through the retained IRQ pool pointers.
        for address, size in ((status, 0x4c), (config, 0x98),
                              (sample, 0xa4), (motor, 0x7c),
                              (speed, 0x50), (temp, 8), (raw, 0x20)):
            u.mem_write(address, bytes(size))
        put_u32(u, A(0x1fff83f8), status)
        put_u32(u, A(0x1fff83fc), config)
        put_u32(u, A(0x1fff8400), sample)
        put_u32(u, A(0x1fff8404), motor)
        put_u32(u, A(0x1fff8410), raw)
        put_u32(u, A(0x1fff842c), temp)
        put_u32(u, A(0x1fff8454), speed)

        put_u32(u, sample + 0x38, 19 if tick else 0)
        put_u32(u, sample + 0x3c, mode)
        put_u32(u, motor + 0x38, 2 if enabled else 0)
        put_f32(u, sample + 0x40, 2048.0)
        put_f32(u, sample + 0x44, 2040.0)
        put_f32(u, sample + 0x48, 2056.0)
        put_f32(u, sample + 0x0c, 0.25)
        put_f32(u, sample + 0x10, -0.125)
        put_f32(u, sample + 0x08, 0.2)
        put_f32(u, sample + 0x2c, 0.1)
        put_f32(u, sample + 0x30, 0.8)
        put_f32(u, motor + 0x14, 0.1)
        put_f32(u, motor + 0x18, 0.2)
        put_f32(u, motor + 0x1c, -0.3)
        put_f32(u, motor + 0x48, 0.5)
        put_f32(u, motor + 0x4c, 2.0)
        put_f32(u, motor + 0x68, 0.8)
        put_f32(u, motor + 0x6c, 0.2)
        put_f32(u, motor + 0x74, 0.99)
        put_f32(u, motor + 0x78, 0.01)
        put_u32(u, motor + 0x54, 14)
        put_f32(u, motor + 0x58, 14.0 / 6.283185307179586)
        put_f32(u, motor + 0x5c, 1.0)
        for i, value in enumerate((0.0, 0.0, 0.0, 1.0, 0.02, -0.02, 30.0)):
            put_f32(u, config + i * 4, value)
        put_f32(u, speed + 0, 0.4)
        put_f32(u, speed + 24, 0.15)
        put_f32(u, speed + 40, 0.5)

        # ADC DR values and fixed helper states used by this IRQ.
        for address, value in ((0x40040000, 2050), (0x40040450, 2040),
                               (0x40040850, 2030), (0x40040452, 1234),
                               (0x40040852, 2100), (0x40040454, 2200),
                               (0x40040854, 2300)):
            u.mem_write(address, struct.pack('<H', value))
        u.mem_write(A(0x1ffff1a8), bytes(0x40))
        u.mem_write(A(0x1ffff190), bytes(0x18))
        u.mem_write(A(0x1fffc78c), bytes(0x98))
        u.mem_write(A(0x1fffc860), bytes(0x3c))
        put_f32(u, A(0x1ffff1a8), 0.1)
        put_f32(u, A(0x1ffff1ac), 0.2)
        put_f32(u, A(0x1ffff1b0), 0.3)

        u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
        u.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
        u.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
        u.reg_write(arm.UC_ARM_REG_FPSCR, (case % 4) << 22)
        trace = []
        done = [False]
        instruction_count = [0]
        if COUNT_INSTRUCTIONS:
            def instruction_hook(uc, address, size, unused):
                instruction_count[0] += 1
            u.hook_add(UC_HOOK_CODE, instruction_hook)
        def hook(uc, access, address, size, value, _):
            observed = value if access == UC_MEM_WRITE else int.from_bytes(
                uc.mem_read(address, size), 'little')
            if not original and size == 4:
                observed = normalize_source_flash_value(
                    observed, symbols, symbol_sizes)
            trace.append((access, address, size, observed))
            if access == UC_MEM_WRITE and address == STOP:
                done[0] = True
                uc.emu_stop()
        ranges = [(A(0x1fff83f8), A(0x1fff845f)),
                  (status, status + 0x4b), (config, config + 0x97),
                  (sample, sample + 0xa3), (motor, motor + 0x7b),
                  (speed, speed + 0x4f), (temp, temp + 7),
                  (raw, raw + 0x1f), (STOP, STOP + 3)]
        if relocated:
            ranges += [(A(0x1ffff088), A(0x1ffff1a7)),
                       (A(0x1fffa560), A(0x1fffa560) + 7),
                       (A(0x1fffa568), A(0x1fffa568) + 0x4f),
                       (A(0x1fffc778), A(0x1fffc778) + 0x1f)]
        for lo, hi in ranges:
            u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, hook,
                       begin=lo, end=hi)
        u.emu_start(FACTORY_ENTRY if original else SOURCE_ENTRY,
                    0x30000, count=400000)
        assert done[0], (case, original, hex(u.reg_read(arm.UC_ARM_REG_PC)))
        if COUNT_INSTRUCTIONS:
            instruction_counts[original].append(instruction_count[0])
        results.append((trace, bytes(u.mem_read(status, 0x4c)),
                        bytes(u.mem_read(sample, 0xa4)),
                        bytes(u.mem_read(motor, 0x7c)),
                        bytes(u.mem_read(speed, 0x50)),
                        bytes(u.mem_read(temp, 8)),
                        u.reg_read(arm.UC_ARM_REG_FPSCR)))
    if results[0] != results[1]:
        print('mismatch case', case, 'relocated', relocated, 'mode', mode,
              'tick', tick, 'enabled', enabled)
        for component, (factory_part, source_part) in enumerate(
                zip(results[0], results[1])):
            if factory_part == source_part:
                continue
            if component == 0:
                print('trace lengths', len(factory_part), len(source_part))
                for index, pair in enumerate(zip(factory_part, source_part)):
                    if pair[0] != pair[1]:
                        print('first trace', index, pair[0], pair[1])
                        print('factory context',
                              factory_part[max(0, index - 8):index + 12])
                        print('source context',
                              source_part[max(0, index - 8):index + 12])
                        break
            elif isinstance(factory_part, bytes):
                differences = [index for index, pair in enumerate(
                               zip(factory_part, source_part))
                               if pair[0] != pair[1]]
                print('component', component, 'byte differences',
                      differences[:64])
            else:
                print('component', component, factory_part, source_part)
        raise AssertionError('factory/source mismatch')

if COUNT_INSTRUCTIONS:
    print('IRQ002 dynamic instructions:')
    for original, label in ((True, 'factory'), (False, 'source')):
        counts = instruction_counts[original]
        print(f'  {label}: total={sum(counts)} min={min(counts)} '
              f'max={max(counts)}')
    print('  source totals by mode:',
          [sum(instruction_counts[False][mode::6]) for mode in range(6)])

print('PASS: 96 full IRQ002 executions; retained status/config/sample/motor/'
      'speed/temperature/raw pointers, fixed-address poison ranges, state and FPSCR')
