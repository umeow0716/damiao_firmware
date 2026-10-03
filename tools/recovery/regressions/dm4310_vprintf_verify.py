from pathlib import Path
import struct
from unicorn import UC_HOOK_CODE
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
for case in range(256):
    results = []
    for original in (True, False):
        u = machine(original)
        u.mem_write(0x20001000, b'A%8.3sZ\0')
        u.mem_write(0x20002000, b'abcdef\0')
        u.mem_write(0x20003000, struct.pack('<I', 0x20002000))
        flags = case & 255
        u.mem_write(0x2000400c, struct.pack('<I', flags))
        u.reg_write(arm.UC_ARM_REG_R0, 0x20001000)
        u.reg_write(arm.UC_ARM_REG_R1, 0x20004000)
        u.reg_write(arm.UC_ARM_REG_R2, 0x20003000)
        u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
        u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
        emitted = bytearray()
        target = F(0x24ea8) if original else symbols['platform_debug_write']
        def callback(uc, address, size, data):
            if address != target: return
            if original: emitted.append(uc.reg_read(arm.UC_ARM_REG_R0) & 255)
            else:
                emitted.extend(uc.mem_read(uc.reg_read(arm.UC_ARM_REG_R0),
                                          uc.reg_read(arm.UC_ARM_REG_R1)))
            if len(emitted) == 1 and case % 2:
                uc.mem_write(0x2000400c, struct.pack('<I', flags ^ 128))
            uc.reg_write(arm.UC_ARM_REG_PC, uc.reg_read(arm.UC_ARM_REG_LR))
        u.hook_add(UC_HOOK_CODE, callback)
        entry = F(0x20de0) if original else symbols['factory_vprintf']
        u.emu_start(entry | 1, 0x30000, count=100000)
        assert u.reg_read(arm.UC_ARM_REG_PC) == 0x30000
        results.append((bytes(emitted), u.reg_read(arm.UC_ARM_REG_R0),
                        bytes(u.mem_read(0x20004000, 16))))
    assert results[0] == results[1], (case, results)
print('PASS: 256 vprintf stream-flag cases including callback mutation; all output precedes error return')
