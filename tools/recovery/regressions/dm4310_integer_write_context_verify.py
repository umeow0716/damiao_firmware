from pathlib import Path
import struct
from unicorn import UC_HOOK_CODE
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
formats = ['%8.3dZ', '%#08xZ', '%+8dZ']
for fmt in formats:
    for width in [0, 8, -8]:
        for precision in [0, 3, -3]:
            results = []
            for original in (True, False):
                u = machine(original)
                u.mem_write(0x20001000, fmt.encode() + bytes([0]))
                u.reg_write(arm.UC_ARM_REG_R0, 0x20001000)
                u.reg_write(arm.UC_ARM_REG_R1, width & 0xffffffff)
                u.reg_write(arm.UC_ARM_REG_R2, precision & 0xffffffff)
                u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
                u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
                trace, emitted, writes, context_pointer = [], bytearray(), [], [0]
                reader = F(0x20edc) if original else symbols['factory_stream_read']
                writer = F(0x24ea8) if original else symbols['platform_debug_write']
                def hook(uc, address, size, data):
                    if address == reader:
                        ptr = uc.reg_read(arm.UC_ARM_REG_R0)
                        context_pointer[0] = ptr
                        words = struct.unpack('<9I', bytes(uc.mem_read(ptr, 36)))
                        trace.append((words[0], words[4], words[6], words[7], words[8]))
                    if address == writer:
                        state = struct.unpack('<9I', bytes(uc.mem_read(context_pointer[0], 36)))
                        writes.append((state[0], state[6], state[7], state[8]))
                        if original: emitted.append(uc.reg_read(arm.UC_ARM_REG_R0) & 255)
                        else:
                            emitted.extend(uc.mem_read(uc.reg_read(arm.UC_ARM_REG_R0),
                                                      uc.reg_read(arm.UC_ARM_REG_R1)))
                        uc.reg_write(arm.UC_ARM_REG_PC, uc.reg_read(arm.UC_ARM_REG_LR))
                u.hook_add(UC_HOOK_CODE, hook)
                u.emu_start((F(0x20474) if original else symbols['debug_console_printf']) | 1,
                            0x30000, count=100000)
                assert u.reg_read(arm.UC_ARM_REG_PC) == 0x30000
                results.append((trace, writes, bytes(emitted), u.reg_read(arm.UC_ARM_REG_R0)))
            assert results[0] == results[1], (fmt, width, precision, results)
print('PASS: 27 integer conversion context cases; flags/next/width/raw precision/count at each read, output/result')
