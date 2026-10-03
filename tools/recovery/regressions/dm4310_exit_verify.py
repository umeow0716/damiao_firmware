from pathlib import Path
import struct
from unicorn import UC_HOOK_CODE
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
new = symbols['runtime_exit']
def normalize(value):
    return {new + 0x1b: F(0x20381), new + 0x21: F(0x20387)}.get(value, value)
for case in range(256):
    regs = [rng.getrandbits(32) for _ in range(13)]
    results = []
    for original in (True, False):
        u = machine(original)
        for i, v in enumerate(regs): u.reg_write(arm.UC_ARM_REG_R0 + i, v)
        u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
        u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
        u.reg_write(arm.UC_ARM_REG_APSR, (case % 16) << 28)
        def stop(uc, address, size, data):
            if bytes(uc.mem_read(address, 2)) == bytes([0xab, 0xbe]):
                uc.emu_stop()
        u.hook_add(UC_HOOK_CODE, stop)
        u.emu_start((F(0x210d2) if original else new) | 1, 0x30000, count=1000)
        pc = u.reg_read(arm.UC_ARM_REG_PC)
        assert bytes(u.mem_read(pc, 2)) == bytes([0xab, 0xbe])
        values = [u.reg_read(arm.UC_ARM_REG_R0 + i) for i in range(13)]
        lr = u.reg_read(arm.UC_ARM_REG_LR)
        frame = struct.unpack('<4I', bytes(u.mem_read(0x2000eff0, 16)))
        if not original:
            lr = normalize(lr)
            frame = tuple(normalize(v) for v in frame)
        results.append((values, u.reg_read(arm.UC_ARM_REG_SP), lr,
                        u.reg_read(arm.UC_ARM_REG_APSR), frame))
    assert results[0] == results[1], (case, results)
print('PASS: 256 exit cases; r0-r12/APSR/SP/relocated LR and 16-byte finalizer/argument stack at semihost trap')
