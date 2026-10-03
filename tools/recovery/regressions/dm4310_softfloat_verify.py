from pathlib import Path
from io import BytesIO
import os
import random
from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_HOOK_MEM_INVALID, UC_MODE_THUMB
import unicorn.arm_const as arm

from dm4310_model_layout import (
    A,
    BUS_OVERVOLTAGE_BITS,
    F,
    FACTORY_PATHS,
    FACTORY_FIXED_SOURCE,
    FACTORY_FIXED_SIZE,
    FACTORY_STATE_BASE,
    FACTORY_STATE_SOURCE,
    FACTORY_STATE_SIZE,
    FACTORY_STACK_TOP,
    FIXED_IMAGE_BASE,
    elf_symbol_sizes,
    normalize_source_flash_value,
    normalize_source_flash_words,
)

root = Path(__file__).resolve().parents[3]
model = os.environ.get('DAMIAO_RECOVERY_MODEL', 'dm4310').lower()
if model not in FACTORY_PATHS:
    raise ValueError(f'unsupported DAMIAO_RECOVERY_MODEL: {model}')
is_dm800x = model in ('dm8006', 'dm8009')
factory = FACTORY_PATHS[model].read_bytes()
elf = ELFFile(BytesIO((root / f'build/{model}.elf').read_bytes()))
symbols = {s.name: s['st_value'] & ~1 for s in elf.get_section_by_name('.symtab').iter_symbols()}
symbol_sizes = elf_symbol_sizes(elf)
segments = [(s['p_paddr'], s.data()) for s in elf.iter_segments() if s['p_type'] == 'PT_LOAD' and s['p_filesz']]
rng = random.Random(957)
mapping = [(F(0x27ad8), '__wrap___aeabi_f2d', 0x58),
           (F(0x270d8), '__wrap___aeabi_dadd', 0x150),
           (F(0x27754), 'dm4310_softdouble_reverse_subtract_core', 0x16),
           (F(0x27904), '__wrap___aeabi_dsub', 0x1d4),
           (F(0x27558), '__wrap___aeabi_dmul', 0x154),
           (F(0x27228), '__wrap___aeabi_ddiv', 0x228)]
def machine(original):
    u = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    u.mem_map(0x20000, 0x20000)
    u.mem_map(A(0x1fff0000), 0x10000)
    u.mem_map(0x20000000, 0x10000)
    if original:
        u.mem_write(0x20000, factory[:FACTORY_FIXED_SOURCE])
    else:
        for address, data in segments: u.mem_write(address, data)
    if os.environ.get('DAMIAO_TRACE_INVALID'):
        def invalid(uc, access, address, size, value, _):
            print('INVALID', 'factory' if original else 'source',
                  hex(uc.reg_read(arm.UC_ARM_REG_PC)), access,
                  hex(address), size, hex(value & 0xffffffff), flush=True)
            return False
        u.hook_add(UC_HOOK_MEM_INVALID, invalid)
    return u
def normalize(value):
    for old, name, size in mapping:
        new = symbols[name]
        if new <= (value & ~1) < new + size:
            offset = value - new
            return old + offset
    return value
edges = [0, 1, 0x8000000000000000, 0x0010000000000000,
         0x3ff0000000000000, 0x7ff0000000000000,
         0xfff0000000000000, 0x7ff0000000000001, 0x7ff8000000000000]
for old, name, size in mapping[:1]:
    for case in range(1024):
        regs = [rng.getrandbits(32) for _ in range(13)]
        if case < len(edges) ** 2:
            a, b = edges[case % len(edges)], edges[case // len(edges)]
            regs[0] = [0, 1, 0x80000000, 0x00800000, 0x3f800000, 0x7f800000, 0xff800000, 0x7f800001, 0x7fc00000][case % len(edges)]
        results = []
        for original in (True, False):
            u = machine(original)
            for i, v in enumerate(regs): u.reg_write(arm.UC_ARM_REG_R0 + i, v)
            u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
            u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
            u.reg_write(arm.UC_ARM_REG_APSR, (case % 16) << 28)
            u.emu_start((old if original else symbols[name]) | 1, 0x30000, count=10000)
            assert u.reg_read(arm.UC_ARM_REG_PC) == 0x30000
            values = [u.reg_read(arm.UC_ARM_REG_R0 + i) for i in range(13)]
            if not original: values[12] = normalize(values[12])
            lr = u.reg_read(arm.UC_ARM_REG_LR)
            if not original: lr = normalize(lr)
            results.append((values, u.reg_read(arm.UC_ARM_REG_SP), lr,
                            u.reg_read(arm.UC_ARM_REG_APSR),
                            bytes(u.mem_read(0x2000efe8, 24))))
        assert results[0] == results[1], (name, case, regs, results)
    print('PASS:', name, '1024 raw cases; registers/APSR/SP/24 stack bytes/relocated live code pointers', flush=True)
