from pathlib import Path
import struct
from unicorn import UC_HOOK_CODE
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
edges32 = [0, 1, 0x80000000, 0x00800000, 0x3f800000,
           0x7f800000, 0xff800000, 0x7f800001, 0x7fc00000]
for case in range(2048):
    regs = [rng.getrandbits(32) for _ in range(13)]
    if case < 648:
        regs[:2] = [edges32[(case // 8) % 9], edges32[case // 72]]
    action = case % 8
    table = sum(action << (3 * i) for i in range(10))
    if case % 2: table |= 0x80000000
    saved = [rng.getrandbits(32), 0x30001]
    results = []
    for original in (True, False):
        u = machine(original)
        for i, value in enumerate(regs): u.reg_write(arm.UC_ARM_REG_R0 + i, value)
        u.mem_write(0x20002000, struct.pack('<I', table))
        u.mem_write(0x2000eff8, struct.pack('<2I', *saved))
        u.reg_write(arm.UC_ARM_REG_SP, 0x2000eff8)
        u.reg_write(arm.UC_ARM_REG_LR, 0x20002001)
        u.reg_write(arm.UC_ARM_REG_APSR, (case % 16) << 28)
        def stop(uc, address, size, data):
            if 0x20002004 <= address <= 0x20002010: uc.emu_stop()
        u.hook_add(UC_HOOK_CODE, stop)
        entry = 0x27b9c if original else symbols['dm4310_softfloat_exception_core']
        u.emu_start(entry | 1, 0x30000, count=1000)
        pc = u.reg_read(arm.UC_ARM_REG_PC)
        assert pc == 0x30000 or 0x20002004 <= pc <= 0x20002010
        results.append(([u.reg_read(arm.UC_ARM_REG_R0 + i) for i in range(13)],
                        u.reg_read(arm.UC_ARM_REG_SP), u.reg_read(arm.UC_ARM_REG_LR),
                        u.reg_read(arm.UC_ARM_REG_APSR), pc,
                        bytes(u.mem_read(0x2000eff8, 8))))
    assert results[0] == results[1], (case, action, results)
print('PASS: 2048 softfloat selector cases; eight actions, all registers/APSR/SP/LR/continuation/saved frame')
