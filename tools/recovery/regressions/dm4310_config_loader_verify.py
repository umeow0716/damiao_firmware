from io import BytesIO
from pathlib import Path
import os
import random
import struct

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
SEGMENTS = [(segment['p_paddr'], segment.data())
            for segment in ELF.iter_segments()
            if segment['p_type'] == 'PT_LOAD' and segment['p_filesz']]

FACTORY_ENTRY = F(0x228c0)
SOURCE_LOAD = SYMBOLS['platform_load_parameters']
SOURCE_PREPARE = SYMBOLS['platform_prepare_runtime_configuration']
SOURCE_INIT = SYMBOLS['app_state_init']
FLASH_WRITER = A(0x1fff9950)
FACTORY_DERIVE = F(0x25138)
SOURCE_DERIVE = SYMBOLS['dm4310_derive_control_parameters_helper']
CONFIG_FLASH = 0x3e000
STAGING = A(0x1fffa5c8)
STATE_BASE = FACTORY_STATE_BASE
STATE_END = A(0x1ffff48f)
FIXED_BASE = A(0x1fff0000)
STOP = 0x3ff00


def fbits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


BASE_RECORD = [0] * 37
BASE_RECORD[0] = fbits(15.0)
BASE_RECORD[1] = 0
BASE_RECORD[2] = fbits(100.0)
BASE_RECORD[3] = fbits(0.8)
BASE_RECORD[4] = fbits(2.0)
BASE_RECORD[5] = fbits(-2.0)
BASE_RECORD[6] = fbits(600.0)
BASE_RECORD[8] = 1
BASE_RECORD[10] = 1
BASE_RECORD[12] = 0x3796feb5
BASE_RECORD[13] = 0x56303033
BASE_RECORD[14] = 0
BASE_RECORD[15] = 0x54303035
BASE_RECORD[16] = 14
BASE_RECORD[17] = 0x3f59999a
BASE_RECORD[18] = 0x39b4e11e
BASE_RECORD[19] = 0x3b9374bc
BASE_RECORD[20] = fbits(10.0)
BASE_RECORD[21] = fbits(12.5)
BASE_RECORD[22] = fbits(30.0)
BASE_RECORD[23] = fbits(10.0)
BASE_RECORD[24] = fbits(1000.0)
BASE_RECORD[25] = 0x3b73cb3e
BASE_RECORD[26] = 0x3b03126f
BASE_RECORD[27] = fbits(54.0)
BASE_RECORD[28] = 0
BASE_RECORD[29] = fbits(32.0)
BASE_RECORD[30] = fbits(1.0)
BASE_RECORD[31] = fbits(4.0)
BASE_RECORD[32] = fbits(40.0)
BASE_RECORD[33] = fbits(2500.0)
BASE_RECORD[34] = fbits(100.0)
BASE_RECORD[35] = 4
BASE_RECORD[36] = 0x30303031


def make_machine(original, state, flash):
    uc = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    uc.mem_map(0x10000, 0x30000)
    uc.mem_map(FIXED_BASE, 0x10000)
    uc.mem_map(0x20000000, 0x30000)
    if original:
        uc.mem_write(0x20000, FACTORY[:0x10000])
        uc.mem_write(FIXED_IMAGE_BASE, FACTORY[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE])
    else:
        for address, data in SEGMENTS:
            uc.mem_write(address, data)
        for section in ELF.iter_sections():
            if (A(0x1fff0000) <= section['sh_addr'] < STATE_BASE and
                    section['sh_type'] != 'SHT_NOBITS'):
                uc.mem_write(section['sh_addr'], section.data())
    uc.mem_write(STATE_BASE, state)
    uc.mem_write(CONFIG_FLASH, flash)
    uc.reg_write(arm.UC_ARM_REG_SP, 0x2002f000)
    uc.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
    uc.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    return uc


def call(uc, entry, r0=0):
    uc.reg_write(arm.UC_ARM_REG_R0, r0)
    uc.reg_write(arm.UC_ARM_REG_LR, STOP | 1)
    uc.emu_start(entry | 1, STOP, count=500000)
    assert uc.reg_read(arm.UC_ARM_REG_PC) == STOP


def run(original, state, flash, rounding):
    uc = make_machine(original, state, flash)
    uc.reg_write(arm.UC_ARM_REG_FPSCR, rounding << 22)
    if not original:
        call(uc, SOURCE_INIT)

    trace = []

    def memory(emu, access, address, size, value, _):
        if access == UC_MEM_READ:
            value = int.from_bytes(emu.mem_read(address, size), 'little')
            trace.append(('r', address, size, value))
        else:
            trace.append(('w', address, size, value & ((1 << (size * 8)) - 1)))

    def code(emu, address, size, _):
        if original and address == 0x219a6:
            # Unicorn 2.1.4 incorrectly carries this routine's dynamic
            # ITTTT condition over the following unconditional argument
            # setup when word 14 is erased.  Execute the architecturally
            # unambiguous 0x228e6..0x228f4 fallback as one operation.
            payload = bytes(emu.mem_read(STAGING, 37 * 4))
            trace.append(('flash-writer', CONFIG_FLASH, STAGING, 37,
                          emu.reg_read(arm.UC_ARM_REG_PRIMASK), payload))
            emu.mem_write(CONFIG_FLASH, payload)
            emu.reg_write(arm.UC_ARM_REG_PC,
                          emu.reg_read(arm.UC_ARM_REG_LR))
        elif address == FLASH_WRITER:
            destination = emu.reg_read(arm.UC_ARM_REG_R0)
            source = emu.reg_read(arm.UC_ARM_REG_R1)
            words = emu.reg_read(arm.UC_ARM_REG_R2)
            payload = bytes(emu.mem_read(source, words * 4))
            trace.append(('flash-writer', destination, source, words,
                          emu.reg_read(arm.UC_ARM_REG_PRIMASK), payload))
            emu.mem_write(destination, payload)
            emu.reg_write(arm.UC_ARM_REG_PC, emu.reg_read(arm.UC_ARM_REG_LR))
        elif address == (FACTORY_DERIVE if original else SOURCE_DERIVE):
            trace.append(('derive',))
            emu.reg_write(arm.UC_ARM_REG_PC, emu.reg_read(arm.UC_ARM_REG_LR))

    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=STATE_BASE, end=STATE_END)
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=CONFIG_FLASH, end=CONFIG_FLASH + 0x93)
    uc.hook_add(UC_HOOK_CODE, code)

    if original:
        call(uc, FACTORY_ENTRY)
    else:
        call(uc, SOURCE_LOAD, SYMBOLS['g_app'])
        call(uc, SOURCE_PREPARE)

    return (trace,
            bytes(uc.mem_read(STATE_BASE, STATE_END - STATE_BASE + 1)),
            bytes(uc.mem_read(CONFIG_FLASH, 0x94)),
            uc.reg_read(arm.UC_ARM_REG_PRIMASK),
            uc.reg_read(arm.UC_ARM_REG_FPSCR))


def record_bytes(words):
    return b''.join((word & 0xffffffff).to_bytes(4, 'little') for word in words)


CASES = []


def add(name, changes=None, erased=None, rounds=(0,)):
    words = BASE_RECORD.copy()
    if changes:
        for index, value in changes.items():
            words[index] = value
    if erased is not None:
        words[erased] = 0xffffffff
    for rounding in rounds:
        CASES.append((f'{name}_r{rounding}', words.copy(), rounding))


add('normal', rounds=(0, 1, 2, 3))
for selector in (0, 4, 5, 11, 12, 0xffffffff):
    add(f'selector_{selector:08x}', {35: selector})
for erased in (15, 14, 8):
    add(f'erased_word_{erased}', erased=erased)
for bits in (0x7fc12345, 0xffc12345, 0x7f800000, 0xff800000,
             0x43f9ffff, 0x43fa0000, 0xbf800000, 0x00000000):
    add(f'velocity_{bits:08x}', {32: bits})
add('enhancement_nans', {33: 0x7fc00001, 34: 0xffc00002})
for mode in (0, 1, 4, 5, 0xffffffff):
    add(f'mode_{mode:08x}', {10: mode})
for override in (0, fbits(0.25), 0x7fc00001, 0x80000000):
    add(f'override_{override:08x}', {1: override})
for poles in (0, 1, 14, 0x00ffffff, 0xffffffff):
    add(f'poles_{poles:08x}', {16: poles}, rounds=(0, 1, 2, 3))
for gear in (0, fbits(-10.0), 0x7fc00001, 0x7f800000):
    add(f'gear_{gear:08x}', {20: gear})

rng = random.Random(0x228c0)
for index in range(32):
    changes = {
        1: rng.getrandbits(32),
        10: rng.getrandbits(32),
        12: rng.getrandbits(32),
        16: rng.getrandbits(32),
        17: rng.getrandbits(32),
        18: rng.getrandbits(32),
        19: rng.getrandbits(32),
        20: rng.getrandbits(32),
        24: rng.getrandbits(32),
        25: rng.getrandbits(32),
        26: rng.getrandbits(32),
        27: rng.getrandbits(32),
        28: rng.getrandbits(32),
        30: rng.getrandbits(32),
        31: rng.getrandbits(32),
        32: rng.getrandbits(32),
        33: rng.getrandbits(32),
        34: rng.getrandbits(32),
        35: rng.getrandbits(32),
    }
    add(f'random_{index:02d}', changes, rounds=(index & 3,))


for name, words, rounding in CASES:
    state = bytearray(rng.getrandbits(8)
                      for _ in range(STATE_END - STATE_BASE + 1))
    initial_staging = BASE_RECORD.copy()
    initial_staging[13] ^= 0x01010101
    initial_staging[15] ^= 0x02020202
    staging_offset = STAGING - STATE_BASE
    state[staging_offset:staging_offset + 0x94] = record_bytes(initial_staging)
    flash = record_bytes(words)
    factory_result = run(True, bytes(state), flash, rounding)
    source_result = run(False, bytes(state), flash, rounding)
    if factory_result != source_result:
        print('MISMATCH', name)
        for component, (factory_part, source_part) in enumerate(
                zip(factory_result, source_result)):
            if factory_part == source_part:
                continue
            if component == 0:
                print('trace lengths', len(factory_part), len(source_part))
                limit = min(len(factory_part), len(source_part))
                for event in range(limit):
                    if factory_part[event] != source_part[event]:
                        print('first event', event)
                        print('factory', factory_part[max(0, event-8):event+12])
                        print('source ', source_part[max(0, event-8):event+12])
                        break
                else:
                    print('factory tail', factory_part[-12:])
                    print('source tail ', source_part[-12:])
            elif component in (1, 2):
                differences = [i for i, (a, b) in enumerate(
                               zip(factory_part, source_part)) if a != b]
                print('component', component, 'differences', differences[:64])
                for offset in differences[:8]:
                    base = STATE_BASE if component == 1 else CONFIG_FLASH
                    print(hex(base + offset), hex(factory_part[offset]),
                          hex(source_part[offset]))
            else:
                print('component', component, factory_part, source_part)
        raise AssertionError(name)
    print('PASS', name)

print(f'PASS: {len(CASES)} factory 0x228c0/source configuration-loader cases; '
      'Flash identity/fallback, grouped staging, normalization, dispatch, '
      'cache, filter and post-derive fixed SRAM traces/final state match')
