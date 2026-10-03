from io import BytesIO
from pathlib import Path
import os
import random

from elftools.elf.elffile import ELFFile
from unicorn import (Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_HOOK_CODE,
                     UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_READ)
import unicorn.arm_const as arm

from dm4310_model_layout import (
    A,
    F,
    FACTORY_FIXED_SOURCE,
    FACTORY_FIXED_SIZE,
    FACTORY_STATE_BASE,
    FACTORY_STATE_SOURCE,
    FACTORY_STATE_SIZE,
    FIXED_IMAGE_BASE,
    elf_symbol_sizes,
    normalize_source_flash_value,
    normalize_source_flash_words,
)

ROOT = Path(__file__).resolve().parents[3]
MODEL = os.environ.get('DAMIAO_RECOVERY_MODEL', 'dm4310').lower()
FACTORY_PATHS = {
    'dm4310': ROOT / 'reference/APP_DM4310_V3_V5017_04.decrypted.bin',
    'dm4340': ROOT / 'reference/APP_DM4340_V3_V5117_04_decrypted.bin',
    'dm8009': ROOT / 'reference/APP_DM8009_V3_V6417_04_decrypted.bin',
}
if MODEL not in FACTORY_PATHS:
    raise ValueError(f'unsupported DAMIAO_RECOVERY_MODEL: {MODEL}')
FACTORY = FACTORY_PATHS[MODEL].read_bytes()
ELF = ELFFile(BytesIO((ROOT / f'build/{MODEL}.elf').read_bytes()))
SYMBOLS = {symbol.name: symbol['st_value'] & ~1
           for symbol in ELF.get_section_by_name('.symtab').iter_symbols()}
SYMBOL_SIZES = elf_symbol_sizes(ELF)
SEGMENTS = [(segment['p_paddr'], segment.data())
            for segment in ELF.iter_segments()
            if segment['p_type'] == 'PT_LOAD' and segment['p_filesz']]

FACTORY_ENTRY = F(0x22244)
SOURCE_ENTRY = SYMBOLS['IRQ004_Handler']
FACTORY_UART = F(0x2379c)
SOURCE_UART = SYMBOLS['platform_debug_write']
STOP = 0x30000
FIXED_BASE = A(0x1fff0000)
STATE_BASE = A(0x1fffa5b8)
STATE_END = A(0x1ffff48f)


def make_machine(original, fixed):
    uc = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    uc.mem_map(0x10000, 0x30000)
    uc.mem_map(FIXED_BASE, 0x10000)
    uc.mem_map(0x20000000, 0x10000)
    uc.mem_map(0x40000000, 0x100000)
    uc.mem_map(0xe000e000, 0x1000)
    if original:
        uc.mem_write(0x20000, FACTORY[:0x10000])
    else:
        for address, data in SEGMENTS:
            uc.mem_write(address, data)
    uc.mem_write(FIXED_BASE, fixed)
    if original:
        uc.mem_write(FIXED_IMAGE_BASE, FACTORY[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE])
    else:
        for section in ELF.iter_sections():
            if (A(0x1fff0000) <= section['sh_addr'] < FACTORY_STATE_BASE and
                    section['sh_type'] != 'SHT_NOBITS'):
                uc.mem_write(section['sh_addr'], section.data())
        uc.mem_write(SYMBOLS['g_app'], bytes(0x320))
    return uc


def put_u16(image, address, value):
    offset = address - FIXED_BASE
    image[offset:offset + 2] = (value & 0xffff).to_bytes(2, 'little')


def put_u32(image, address, value):
    offset = address - FIXED_BASE
    image[offset:offset + 4] = (value & 0xffffffff).to_bytes(4, 'little')


def run(original, fixed, peripheral, system):
    uc = make_machine(original, fixed)
    uc.mem_write(0x40000000, peripheral)
    uc.mem_write(0xe000e000, system)
    uc.mem_write(0x1e000, bytes.fromhex(
        '112233445566778899aabbccddeeff00' + '00' * 48))
    uc.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    uc.reg_write(arm.UC_ARM_REG_LR, STOP | 1)
    uc.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
    uc.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    events = []

    def memory(emu, access, address, size, value, _):
        if access == UC_MEM_READ:
            value = int.from_bytes(emu.mem_read(address, size), 'little')
        if not original and size == 4:
            value = normalize_source_flash_value(
                value, SYMBOLS, SYMBOL_SIZES)
        events.append(('read' if access == UC_MEM_READ else 'write',
                       address, size, value))

    def code(emu, address, size, _):
        if address == (FACTORY_UART if original else SOURCE_UART):
            pointer = emu.reg_read(arm.UC_ARM_REG_R0)
            length = emu.reg_read(arm.UC_ARM_REG_R1)
            events.append(('uart', length, bytes(emu.mem_read(pointer, length))))
            emu.reg_write(arm.UC_ARM_REG_PC,
                          emu.reg_read(arm.UC_ARM_REG_LR))

    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=FIXED_BASE, end=A(0x1fffffff))
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=0x40000000, end=0x400fffff)
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=0xe000e000, end=0xe000efff)
    uc.hook_add(UC_HOOK_CODE, code)
    uc.emu_start((FACTORY_ENTRY if original else SOURCE_ENTRY) | 1,
                 STOP, count=300000)
    assert uc.reg_read(arm.UC_ARM_REG_PC) == STOP
    state_image = bytes(uc.mem_read(STATE_BASE, STATE_END - STATE_BASE + 1))
    if not original:
        state_image = normalize_source_flash_words(
            state_image, SYMBOLS, SYMBOL_SIZES)
    return (
        events,
        state_image,
        bytes(uc.mem_read(0x4001c000, 0x1000)),
        bytes(uc.mem_read(0x40024000, 0x100)),
        bytes(uc.mem_read(0x40053000, 0x100)),
        bytes(uc.mem_read(0xe000e000, 0x1000)),
    )


CASES = (
    ('no_irq', b'', 0, 0, 0),
    ('zero_length_retained_motor', b'm', 0, 0, 0),
    ('negative_length_retained_setup', b's', -1, 0, 0),
    ('unknown_menu', b'?', 1, 0, 1),
    ('escape_fault1', b'\x1b', 1, 0, 1),
    ('escape_fault2', b'\x1b', 1, 0, 2),
    ('motor', b'm', 1, 0, 0),
    ('motor_fault2', b'm', 1, 0, 2),
    ('setup', b's', 1, 0, 0),
    ('Uc', b'Uc', 1, 4, 0),
    ('Ul', b'Ul', 1, 4, 0),
    ('Ue_parameters', b'Ue\xaa', 3, 4, 0),
    ('Ue_identify', b'UeU', 3, 4, 0),
    ('Uf_identity', b'Uf\xaa', 3, 4, 0),
    ('Ug_request', b'Ug\xaa', 3, 4, 0),
    ('Ug_upload', b'UgU' + bytes(range(128)), 131, 4, 0),
    ('Ug_zero', b'UgZ' + bytes(reversed(range(128))), 131, 4, 0),
    ('FCB4', b'FCB4', 1, 4, 0),
    ('FCB_colon', b'FCB:', 4, 4, 0),
    ('FCB_invalid', b'FCB/', 4, 4, 0),
    ('Ud_complete', b'Ud' + bytes(60) + bytes((2, 17)), 64, 4, 0),
    ('UM_complete', b'UM' + bytes(60) + bytes((2, 136)), 64, 4, 0),
)

rng = random.Random(0x22244)
for name, command, received_length, mode, fault in CASES:
    fixed = bytearray(rng.getrandbits(8) for _ in range(0x10000))
    peripheral = bytearray(rng.getrandbits(8) for _ in range(0x100000))
    system = bytearray(rng.getrandbits(8) for _ in range(0x1000))
    put_u16(fixed, A(0x1fffa5b8), 63)
    put_u16(fixed, A(0x1fffa5ba), 0)
    put_u32(fixed, A(0x1ffff0c0), mode)
    put_u32(fixed, A(0x1ffff184), fault)
    put_u32(fixed, A(0x1ffff0bc), 0x3f400000)
    rx_offset = A(0x1ffff3c7) - FIXED_BASE
    fixed[rx_offset:rx_offset + len(command)] = command
    status = int.from_bytes(peripheral[0x1cc00:0x1cc04], 'little')
    if name == 'no_irq':
        status &= ~0x100
    else:
        status |= 0x100
    peripheral[0x1cc00:0x1cc04] = status.to_bytes(4, 'little')
    dma_count = int.from_bytes(peripheral[0x53068:0x5306c], 'little')
    dma_count = ((200 - received_length) & 0xffff) << 16 | (dma_count & 0xffff)
    peripheral[0x53068:0x5306c] = dma_count.to_bytes(4, 'little')

    factory_result = run(True, bytes(fixed), bytes(peripheral), bytes(system))
    source_result = run(False, bytes(fixed), bytes(peripheral), bytes(system))
    if factory_result != source_result:
        print('MISMATCH', name)
        for component, (factory_part, source_part) in enumerate(
                zip(factory_result, source_result)):
            if factory_part == source_part:
                continue
            if component == 0:
                print('event lengths', len(factory_part), len(source_part))
                for index in range(min(len(factory_part), len(source_part))):
                    if factory_part[index] != source_part[index]:
                        print('first event', index)
                        print('factory', factory_part[max(0, index-8):index+12])
                        print('source ', source_part[max(0, index-8):index+12])
                        break
                else:
                    print('common prefix; factory tail', factory_part[-12:])
                    print('common prefix; source tail ', source_part[-12:])
            else:
                differences = [index for index, (a, b) in enumerate(
                               zip(factory_part, source_part)) if a != b]
                print('component', component, 'byte differences',
                      differences[:40])
        raise AssertionError(name)
    print('PASS', name)


def run_reset(original, fixed, peripheral, system):
    uc = make_machine(original, fixed)
    uc.mem_write(0x40000000, peripheral)
    uc.mem_write(0xe000e000, system)
    uc.mem_write(0x1e000, bytes(range(64)))
    uc.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    uc.reg_write(arm.UC_ARM_REG_LR, STOP | 1)
    uc.reg_write(arm.UC_ARM_REG_PRIMASK, 0)
    events = []
    barriers = 0

    def memory(emu, access, address, size, value, _):
        if access == UC_MEM_READ:
            value = int.from_bytes(emu.mem_read(address, size), 'little')
        if not original and size == 4:
            value = normalize_source_flash_value(
                value, SYMBOLS, SYMBOL_SIZES)
        events.append(('read' if access == UC_MEM_READ else 'write',
                       address, size, value))

    def return_from_stub(emu):
        emu.reg_write(arm.UC_ARM_REG_PC, emu.reg_read(arm.UC_ARM_REG_LR))

    def code(emu, address, size, _):
        nonlocal barriers
        bank = (0x26e04 if original else
                SYMBOLS['dm4310_select_configuration_bank_b_helper'])
        delay = 0x21f60 if original else SYMBOLS['board_delay_us']
        if address == bank:
            events.append(('bank-b',))
            return_from_stub(emu)
        elif address == delay:
            events.append(('delay-us', emu.reg_read(arm.UC_ARM_REG_R0)))
            return_from_stub(emu)
        elif bytes(emu.mem_read(address, 4)) == bytes.fromhex('bff34f8f'):
            events.append(('dsb',))
            barriers += 1
            if barriers == 2:
                emu.emu_stop()

    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=FIXED_BASE, end=A(0x1fffffff))
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=0x40000000, end=0x400fffff)
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=0xe000e000, end=0xe000efff)
    uc.hook_add(UC_HOOK_CODE, code)
    uc.emu_start((FACTORY_ENTRY if original else SOURCE_ENTRY) | 1,
                 STOP, count=300000)
    assert barriers == 2
    state_image = bytes(uc.mem_read(STATE_BASE, STATE_END - STATE_BASE + 1))
    if not original:
        state_image = normalize_source_flash_words(
            state_image, SYMBOLS, SYMBOL_SIZES)
    return (events,
            state_image,
            bytes(uc.mem_read(0x40053000, 0x100)),
            bytes(uc.mem_read(0xe000ed0c, 4)),
            uc.reg_read(arm.UC_ARM_REG_PRIMASK))


fixed = bytearray(rng.getrandbits(8) for _ in range(0x10000))
peripheral = bytearray(rng.getrandbits(8) for _ in range(0x100000))
system = bytearray(rng.getrandbits(8) for _ in range(0x1000))
put_u16(fixed, A(0x1fffa5b8), 63)
put_u32(fixed, A(0x1ffff0c0), 0)
rx_offset = A(0x1ffff3c7) - FIXED_BASE
fixed[rx_offset] = ord('X')
status = int.from_bytes(peripheral[0x1cc00:0x1cc04], 'little') | 0x100
peripheral[0x1cc00:0x1cc04] = status.to_bytes(4, 'little')
dma_count = int.from_bytes(peripheral[0x53068:0x5306c], 'little')
peripheral[0x53068:0x5306c] = (
    (199 << 16) | (dma_count & 0xffff)).to_bytes(4, 'little')
factory_reset = run_reset(True, bytes(fixed), bytes(peripheral), bytes(system))
source_reset = run_reset(False, bytes(fixed), bytes(peripheral), bytes(system))
if factory_reset != source_reset:
    print('MISMATCH reset-X')
    for component, (factory_part, source_part) in enumerate(
            zip(factory_reset, source_reset)):
        if factory_part != source_part:
            print('component', component, factory_part, source_part)
    raise AssertionError('reset-X')
print('PASS reset-X')

print('PASS: complete factory 0x22244/source UART IRQ command matrix; direct '
      'fixed DMA frame, signed malformed lengths, parser SRAM/MMIO traces, '
      'replies, normal epilogue and non-returning X reset sequence')
