from pathlib import Path
import random
import struct

from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_READ

exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split(
    '\nfor old, name, size in mapping[:1]:')[0])

FACTORY_ENTRY = F(0x24ec0)
SOURCE_ENTRY = symbols['platform_initialize_runtime']
SOURCE_STOP = SOURCE_ENTRY + 0xc6
FACTORY_DELAY_MS = F(0x21f28)
FACTORY_DELAY_US = F(0x21f60)
SOURCE_DELAY_MS = symbols['board_delay_ms']
SOURCE_DELAY_US = symbols['board_delay_us']
FACTORY_PRINTF = F(0x20474)
SOURCE_PRINTF = symbols['debug_console_printf']


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


def c_string(uc, address):
    result = bytearray()
    while True:
        byte = bytes(uc.mem_read(address, 1))[0]
        if byte == 0:
            return bytes(result)
        result.append(byte)
        address += 1


def run(original, fixed, table, peripheral, raw_u, raw_v, fpscr):
    uc = cluster_machine(original)
    uc.mem_write(A(0x1ffff000), fixed)
    uc.mem_write(A(0x1fffd078), table)
    uc.mem_write(0x40000000, peripheral)
    uc.mem_write(0x40040044, (1).to_bytes(4, 'little'))
    uc.mem_write(0x40040444, (1).to_bytes(4, 'little'))
    uc.mem_write(0x40040054, raw_u.to_bytes(2, 'little'))
    uc.mem_write(0x40040454, raw_v.to_bytes(2, 'little'))

    if not original:
        uc.mem_write(symbols['adc_initialized'], b'\x01')
        uc.mem_write(symbols['output_sensor_table_valid'], b'\x01')
        uc.mem_write(symbols['g_app'], bytes(0x500))
        uc.mem_write(symbols['startup'], bytes(0x2c))
        uc.mem_write(symbols['position_sensor'], bytes(0x48))

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

    def return_from_stub(emu):
        emu.reg_write(arm.UC_ARM_REG_PC, emu.reg_read(arm.UC_ARM_REG_LR))

    def code(emu, address, size, _):
        nonlocal stopped
        if address in ((FACTORY_DELAY_MS if original else SOURCE_DELAY_MS),
                       (FACTORY_DELAY_US if original else SOURCE_DELAY_US)):
            unit = 'ms' if address == (FACTORY_DELAY_MS if original else
                                       SOURCE_DELAY_MS) else 'us'
            events.append(('delay_' + unit, emu.reg_read(arm.UC_ARM_REG_R0)))
            return_from_stub(emu)
        elif address == (FACTORY_PRINTF if original else SOURCE_PRINTF):
            events.append(('printf', c_string(emu, emu.reg_read(arm.UC_ARM_REG_R0)),
                           emu.reg_read(arm.UC_ARM_REG_R2),
                           emu.reg_read(arm.UC_ARM_REG_R3)))
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
    uc.emu_start(entry | 1, 0x30000, count=1000000)
    if original:
        assert uc.reg_read(arm.UC_ARM_REG_PC) == 0x30000
    else:
        assert stopped, hex(uc.reg_read(arm.UC_ARM_REG_PC))

    mmio_ranges = (
        (0x40010870, 0x10),
        (0x40020000, 0x100),
        (0x40040000, 0x900),
        (0x40048000, 0x20),
    )
    return (
        events,
        bytes(uc.mem_read(A(0x1fffd078), 0x2000)),
        bytes(uc.mem_read(A(0x1ffff000), len(fixed))),
        tuple(bytes(uc.mem_read(address, size))
              for address, size in mmio_ranges),
        uc.reg_read(arm.UC_ARM_REG_FPSCR),
    )


rng = random.Random(0x24ec0)
raw_pairs = (
    (2000, 2100), (100, 2100), (2000, 4000), (100, 4000),
    (186, 187), (3909, 3910), (0, 4095), (2048, 2048),
)
for case, (raw_u, raw_v) in enumerate(raw_pairs):
    fixed = bytearray(rng.getrandbits(8) for _ in range(0x300))
    # Output sensor calibration/state fields used by the startup decode.
    for address, value in (
            (A(0x1ffff1bc), 0x45000000),   # center U = 2048
            (A(0x1ffff1c4), 0x45000000),   # center V = 2048
            (A(0x1ffff1d0), 0x3f800000),   # V gain = 1
            (A(0x1ffff1d4), 0x00000000),   # phase sine
            (A(0x1ffff1d8), 0x3f800000),   # phase cosine
            (A(0x1ffff1e0), 0x00000000),   # zero offset
            (A(0x1ffff09c), 0x00000000),   # motor electrical offset
            (A(0x1ffff0ac), 0x00000000),   # motor output offset
            (A(0x1ffff0d4), 0x3f800000),   # motor gear ratio
            (A(0x1ffff0e4), 0x3f800000),   # motor output scale
            (A(0x1ffff198), 0x3e800000)):  # SPI encoder wrapped angle
        put_word(fixed, A(0x1ffff000), address, value)
    table = bytearray(0x2000)
    for index in range(4096):
        table[index * 2:index * 2 + 2] = index.to_bytes(2, 'little')
    peripheral = bytearray(rng.getrandbits(8) for _ in range(0x100000))
    fpscr = (case & 3) << 22
    factory_result = run(True, bytes(fixed), bytes(table), bytes(peripheral),
                         raw_u, raw_v, fpscr)
    source_result = run(False, bytes(fixed), bytes(table), bytes(peripheral),
                        raw_u, raw_v, fpscr)
    if factory_result != source_result:
        print('mismatch case', case, raw_u, raw_v)
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
                else:
                    print('factory remainder', factory_component[len(source_component):])
            elif isinstance(factory_component, bytes):
                differences = [index for index, (a, b) in enumerate(
                               zip(factory_component, source_component))
                               if a != b]
                print('component', component, 'byte diffs', differences[:32])
            else:
                print('component', component, factory_component, source_component)
        raise AssertionError('factory/source mismatch')

print('PASS: 8 complete factory 0x24ec0/source current-sensor startup cases; '
      '8,000 ADC samples, U/V valid and fault arms, fixed sensor/motor state, '
      'lookup/alignment, diagnostics, final ADC trigger MMIO and FPSCR')
