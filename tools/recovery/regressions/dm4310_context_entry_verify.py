from pathlib import Path
import struct
from unicorn import UC_HOOK_CODE
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
formats = ['%8.3sZ', '%08.3dZ', '%+08qZ']
for case in range(96):
    results = []
    for original in (True, False):
        u = machine(original)
        fmt = formats[case % 3]
        u.mem_write(0x20002000, fmt.encode() + bytes([0]))
        u.mem_write(0x20004000, b'abcdef\0')
        u.mem_write(0x20005000, struct.pack('<I', 0x20004000 if 's' in fmt else 42))
        u.mem_write(0x30020, bytes([0x70, 0x47]))
        u.reg_write(arm.UC_ARM_REG_R0, 0x20002000)
        u.reg_write(arm.UC_ARM_REG_R1, 0x12345678)
        u.reg_write(arm.UC_ARM_REG_R2, 0x20005000)
        u.reg_write(arm.UC_ARM_REG_R3, 0x30021)
        u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
        u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
        writes, emitted, context = [], bytearray(), [0]
        reader = 0x20edc if original else symbols['factory_stream_read']
        def hook(uc, address, size, data):
            if address == reader: context[0] = uc.reg_read(arm.UC_ARM_REG_R0)
            if address != 0x30020: return
            words = list(struct.unpack('<9I', bytes(uc.mem_read(context[0], 36))))
            words[3] = 0x20edc
            character = uc.reg_read(arm.UC_ARM_REG_R0) & 255
            writes.append((character, uc.reg_read(arm.UC_ARM_REG_R1), tuple(words)))
            emitted.append(character)
            if len(emitted) == 1:
                if case & 4: uc.mem_write(context[0] + 32, struct.pack('<I', 17))
                if case & 8: uc.mem_write(context[0] + 8, struct.pack('<I', 0x87654321))
            uc.reg_write(arm.UC_ARM_REG_PC, uc.reg_read(arm.UC_ARM_REG_LR))
        u.hook_add(UC_HOOK_CODE, hook)
        entry = 0x20ee6 if original else symbols['factory_format_context']
        u.emu_start(entry | 1, 0x30000, count=100000)
        assert u.reg_read(arm.UC_ARM_REG_PC) == 0x30000
        results.append((writes, bytes(emitted), u.reg_read(arm.UC_ARM_REG_R0)))
    assert results[0] == results[1], (case, results)
print('PASS: 96 context-entry cases; custom writer/opaque/count mutation; normalized callback context and result')
