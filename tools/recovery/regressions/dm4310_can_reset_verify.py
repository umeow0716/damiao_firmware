from pathlib import Path
import struct
from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split(
    '\nfor old, name, size in mapping[:1]:')[0])

for case in range(32):
    observations = []
    for original in (True, False):
        u = machine(original)
        if original:
            u.mem_write(FIXED_IMAGE_BASE, factory[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE])
        u.mem_map(0xe000e000, 0x1000)
        u.mem_write(0xe000ed0c, struct.pack('<I', case * 0x1020304))
        u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
        u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
        trace = []
        def memory(uc, access, address, size, value, _):
            trace.append(('write' if access == 17 else 'read', size,
                value if access == 17 else
                int.from_bytes(uc.mem_read(address, size), 'little')))
        def code(uc, address, size, _):
            if bytes(uc.mem_read(address, 4)) == bytes.fromhex('bff34f8f'):
                trace.append(('dsb',))
            if original and address == A(0x1fff97a8):
                uc.emu_stop()
            elif not original and bytes(uc.mem_read(address, 2)) == b'\x00\xbf':
                uc.emu_stop()
        u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                   begin=0xe000ed0c, end=0xe000ed0f)
        u.hook_add(UC_HOOK_CODE, code)
        u.emu_start((A(0x1fff9792) if original else
                     symbols['reset_after_barrier']) | 1,
                    0x30000, count=100)
        # Source helper is entered after its caller's first DSB.
        if not original:
            trace.insert(0, ('dsb',))
        observations.append(trace)
    assert observations[0] == observations[1], (case, observations)
print('PASS: 32 CAN reset AIRCR/barrier sequences against factory')
