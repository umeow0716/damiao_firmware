from pathlib import Path
from unicorn import UC_HOOK_MEM_INVALID, UC_HOOK_MEM_WRITE

exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split(
    '\nfor old, name, size in mapping[:1]:')[0])

ranges = ((A(0x1ffff104), 0xa4), (A(0x1fffc78c), 0x4c),
          (A(0x1fffc7d8), 0x4c), (A(0x1fffa568), 0x28),
          (A(0x1fffa590), 0x28))

def reset_machine(original):
    u = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    u.mem_map(0x20000, 0x20000)
    u.mem_map(A(0x1fff0000), 0x10000)
    u.mem_map(0x20000000, 0x10000)
    if original:
        u.mem_write(0x20000, factory[:0x20000])
    else:
        for address, data in segments:
            u.mem_write(address, data)
        for section in elf.iter_sections():
            if (A(0x1fff0000) <= section['sh_addr'] < 0x20000000 and
                    section['sh_type'] != 'SHT_NOBITS'):
                u.mem_write(section['sh_addr'], section.data())
    return u

for case in range(256):
    initial = {address: bytes(rng.getrandbits(8) for _ in range(size))
               for address, size in ranges}
    results = []
    for original in (True, False):
        u = reset_machine(original)
        for address, data in initial.items():
            u.mem_write(address, data)
        u.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
        u.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
        u.reg_write(arm.UC_ARM_REG_FPSCR, (case & 15) << 22)
        u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
        u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
        trace = []
        invalid = []
        def invalid_access(uc, access, address, size, value, _):
            invalid.append((access, address, size, value,
                            uc.reg_read(arm.UC_ARM_REG_PC)))
            return False
        def write(uc, access, address, size, value, _):
            trace.append((address, size, value))
        for address, size in ranges:
            u.hook_add(UC_HOOK_MEM_WRITE, write,
                       begin=address, end=address + size - 1)
        u.hook_add(UC_HOOK_MEM_INVALID, invalid_access)
        entry = F(0x2a448) if original else symbols['dm4310_reset_control_state_step']
        try:
            u.emu_start(entry | 1, 0x30000, count=10000)
        except Exception as error:
            raise AssertionError((case, original, hex(entry), invalid)) from error
        pc = u.reg_read(arm.UC_ARM_REG_PC)
        if pc != 0x30000:
            raise AssertionError((case, original, hex(entry), hex(pc),
                                  hex(u.reg_read(arm.UC_ARM_REG_LR))))
        results.append((trace,
            tuple(bytes(u.mem_read(address, size)) for address, size in ranges),
            u.reg_read(arm.UC_ARM_REG_FPSCR)))
    assert results[0] == results[1], (case, results)

print('PASS: 256 direct flash 0x2a448 reset cases; all fixed targets, ordered '
      'stores, final state and FPSCR')
