from pathlib import Path
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
registers = [getattr(arm, 'UC_ARM_REG_R%d' % i) for i in range(13)]
for case in range(256):
    initial = [rng.getrandbits(32) for _ in registers]
    flags = rng.getrandbits(4) << 28
    snapshots = []
    for original in (True, False):
        u = machine(original)
        for reg, value in zip(registers, initial):
            u.reg_write(reg, value)
        u.reg_write(arm.UC_ARM_REG_APSR, flags)
        u.reg_write(arm.UC_ARM_REG_SP, 0x2000e000 + 4 * (case % 8))
        u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
        u.mem_write(A(0x1ffff490), bytes([0xa5]) * 0x60)
        trace = []
        def memory(uc, access, address, size, value, data):
            trace.append((access, address, size,
                          value if access == 17 else
                          int.from_bytes(uc.mem_read(address, size), 'little')))
        u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                   begin=A(0x1ffff490), end=A(0x1ffff4ef))
        entry = F(0x21088) if original else symbols['runtime_context_initialize']
        u.emu_start(entry | 1, 0x30000, count=1000)
        assert u.reg_read(arm.UC_ARM_REG_PC) == 0x30000
        snapshots.append(([u.reg_read(reg) for reg in registers],
                          u.reg_read(arm.UC_ARM_REG_SP),
                          u.reg_read(arm.UC_ARM_REG_APSR),
                          bytes(u.mem_read(A(0x1ffff490), 0x60)), trace))
    assert snapshots[0] == snapshots[1], (case, snapshots)
print('PASS: 256 runtime-context entries; R0-R12, SP, APSR, full 0x60 context and ordered SRAM reads/writes match with identical incoming LR')
