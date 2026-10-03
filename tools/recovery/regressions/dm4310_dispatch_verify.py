from pathlib import Path
import struct
from unicorn import UC_HOOK_CODE
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
for old, name in [(F(0x20328), 'factory_dispatch')]:
    for case in range(256):
        flags = case % 64
        width = [0, 1, 4, 12, 0xffffffff][case % 5]
        precision = case % 8
        written = rng.getrandbits(32)
        limit = [0, 1, 3, 0xffffffff][case % 4]
        results = []
        for original in (True, False):
            u = machine(original)
            words = [flags, 0x30021, 0x12345678, 0, 0, 0, width, precision, written]
            u.mem_write(0x20001000, struct.pack('<9I', *words))
            u.mem_write(0x20002000, b'abcdef\0' + bytes(25))
            u.mem_write(0x30020, bytes([0x70, 0x47]))
            u.reg_write(arm.UC_ARM_REG_R0, 0x20001000)
            conversion = ['d', 'x', 's', 'q', '%', 'u', 'F', 'c'][case % 8]
            u.mem_write(0x20003000, struct.pack('<I', 0x20002000 if conversion == 's' else 42))
            u.reg_write(arm.UC_ARM_REG_R1, ord(conversion))
            u.reg_write(arm.UC_ARM_REG_R2, 0x20003000)
            u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
            u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
            trace = []
            def callback(uc, address, size, data):
                if address != 0x30020: return
                trace.append((uc.reg_read(arm.UC_ARM_REG_R0),
                              uc.reg_read(arm.UC_ARM_REG_R1),
                              struct.unpack('<I', bytes(uc.mem_read(0x20001020, 4)))[0]))
                if len(trace) == 1:
                    uc.mem_write(0x20001008, struct.pack('<I', 0x87654321))
                    uc.mem_write(0x20002001, b'Z')
                    uc.mem_write(0x20001020, struct.pack('<I', 17))
                uc.reg_write(arm.UC_ARM_REG_PC, uc.reg_read(arm.UC_ARM_REG_LR))
            u.hook_add(UC_HOOK_CODE, callback)
            entry = old if original else symbols[name]
            u.emu_start(entry | 1, 0x30000, count=10000)
            assert u.reg_read(arm.UC_ARM_REG_PC) == 0x30000
            results.append((u.reg_read(arm.UC_ARM_REG_R0), trace, bytes(u.mem_read(0x20001000, 36)),
                            bytes(u.mem_read(0x20002000, 32))))
        assert results[0] == results[1], (name, case, results)
    print('PASS:', name, '256 stream cases; callbacks/live context/written and text mutations', flush=True)
