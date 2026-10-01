from pathlib import Path
import struct
from unicorn import UC_HOOK_CODE
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
formats = ['A%08.3dZ', 'A%8.3sZ', 'A%+08qZ', 'A%*.*qZ']
for case in range(128):
    results = []
    for original in (True, False):
        u = machine(original)
        fmt = formats[case % 4]
        initial = [0x400, 0x30021, 0x20001000, 0x30041, 0x20002000, 0,
                   0x12345678, 0xabcdef01, 0x87654321]
        u.mem_write(0x20001000, struct.pack('<9I', *initial))
        u.mem_write(0x20002000, fmt.encode() + bytes([0]))
        u.mem_write(0x20004000, b'abcdef\0')
        args = [6, (case % 7 - 3) & 0xffffffff] if '*' in fmt else [0x20004000 if 's' in fmt else 42]
        u.mem_write(0x20005000, struct.pack('<' + 'I' * len(args), *args))
        for address in (0x30020, 0x30040, 0x30060): u.mem_write(address, bytes([0x70, 0x47]))
        u.reg_write(arm.UC_ARM_REG_R0, 0x20001000)
        u.reg_write(arm.UC_ARM_REG_R1, 0x20005000)
        u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
        u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
        reads, writes, emitted = [], [], bytearray()
        def callback(uc, address, size, data):
            if address not in (0x30020, 0x30040, 0x30060): return
            words = list(struct.unpack('<9I', bytes(uc.mem_read(0x20001000, 36))))
            if address in (0x30040, 0x30060):
                reads.append((address, tuple(words)))
                uc.mem_write(0x20001010, struct.pack('<I', words[4] + 1))
                uc.reg_write(arm.UC_ARM_REG_R0, bytes(uc.mem_read(words[4], 1))[0])
            else:
                character = uc.reg_read(arm.UC_ARM_REG_R0) & 255
                writes.append((character, uc.reg_read(arm.UC_ARM_REG_R1), tuple(words)))
                emitted.append(character)
                if len(emitted) == 1:
                    if case & 4: uc.mem_write(0x2000100c, struct.pack('<I', 0x30061))
                    if case & 8: uc.mem_write(0x20001020, struct.pack('<I', 17))
                    if case & 16: uc.mem_write(0x20001008, struct.pack('<I', 0x11111111))
            uc.reg_write(arm.UC_ARM_REG_PC, uc.reg_read(arm.UC_ARM_REG_LR))
        u.hook_add(UC_HOOK_CODE, callback)
        entry = 0x205fc if original else symbols['factory_parse']
        u.emu_start(entry | 1, 0x30000, count=100000)
        assert u.reg_read(arm.UC_ARM_REG_PC) == 0x30000
        results.append((reads, writes, bytes(emitted), u.reg_read(arm.UC_ARM_REG_R0),
                        bytes(u.mem_read(0x20001000, 36))))
    assert results[0] == results[1], (case, results)
print('PASS: 128 generic parser cases; custom read/write callbacks, context/count/reader mutations and full 36-byte context')
