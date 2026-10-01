from pathlib import Path
from unicorn import UC_HOOK_CODE
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
formats = ['%s', '%8s', '%-8s', '%08s', '%.0s', '%.1s', '%.3s',
           '%8.3s', '%-8.3s', '%08.3s', '%d', '%08d', '%-8d', '%x']
for fmt in formats:
    results = []
    for original in (True, False):
        u = machine(original)
        u.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
        u.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
        u.mem_write(0x20001000, fmt.encode() + bytes([0]))
        u.mem_write(0x20002000, b'abcdef\0')
        u.reg_write(arm.UC_ARM_REG_R0, 0x20001000)
        u.reg_write(arm.UC_ARM_REG_R1, 0x20002000 if 's' in fmt else 42)
        u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
        u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
        emitted = bytearray()
        target = 0x24ea8 if original else symbols['platform_debug_write']
        def callback(uc, address, size, data):
            if address != target: return
            if original: emitted.append(uc.reg_read(arm.UC_ARM_REG_R0) & 255)
            else:
                emitted.extend(uc.mem_read(uc.reg_read(arm.UC_ARM_REG_R0),
                                          uc.reg_read(arm.UC_ARM_REG_R1)))
            uc.reg_write(arm.UC_ARM_REG_PC, uc.reg_read(arm.UC_ARM_REG_LR))
        u.hook_add(UC_HOOK_CODE, callback)
        entry = 0x20474 if original else symbols['debug_console_printf']
        u.emu_start(entry | 1, 0x30000, count=100000)
        assert u.reg_read(arm.UC_ARM_REG_PC) == 0x30000, (fmt, original)
        results.append((bytes(emitted), u.reg_read(arm.UC_ARM_REG_R0)))
    assert results[0] == results[1], (fmt, results)
print('PASS: 14 whole printf string/integer cases; emitted bytes and return count')
