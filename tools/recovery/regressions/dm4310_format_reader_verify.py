from pathlib import Path
import struct
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_READ
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
for case in range(256):
    results = []
    for original in (True, False):
        u = machine(original)
        u.mem_write(0x20001010, struct.pack('<I', 0x20002000))
        u.mem_write(0x20002000, bytes([case]))
        u.reg_write(arm.UC_ARM_REG_R0, 0x20001000)
        u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
        u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
        trace = []
        def memory(uc, access, address, size, value, data):
            if access == UC_MEM_READ:
                value = int.from_bytes(uc.mem_read(address, size), 'little')
            trace.append((access, address, size, value))
        u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                   begin=0x20001000, end=0x20002001)
        entry = F(0x20edc) if original else symbols['factory_stream_read']
        u.emu_start(entry | 1, 0x30000, count=1000)
        results.append((u.reg_read(arm.UC_ARM_REG_R0), trace,
                        bytes(u.mem_read(0x20001010, 4))))
    assert results[0] == results[1], (case, results)
print('PASS: all 256 format byte values; result and pointer-read/advance-store/byte-read trace match')
