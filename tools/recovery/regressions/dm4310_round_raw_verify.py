from pathlib import Path
import struct
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
mapping.extend([(F(0x23e94), 'dm4310_runtime_round_to_int', 0xc2),
                (F(0x27c3c), 'dm4310_round_even_core', 0x60),
                (F(0x208d6), 'dm4310_runtime_errno_set_helper', 0xc)])
edges32 = [0, 1, 0x80000000, 0x00800000, 0x3f000000, 0xbf000000,
           0x3fc00000, 0xbfc00000, 0x4f000000, 0xcf000000,
           0x7f800000, 0xff800000, 0x7f800001, 0x7fc00000]
for case in range(1024):
    regs = [rng.getrandbits(32) for _ in range(13)]
    floats = [rng.getrandbits(32) for _ in range(32)]
    if case < len(edges32) * 4: floats[0] = edges32[case // 4]
    results = []
    for original in (True, False):
        u = machine(original)
        u.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
        u.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
        for i, v in enumerate(regs): u.reg_write(arm.UC_ARM_REG_R0 + i, v)
        for i, v in enumerate(floats): u.reg_write(arm.UC_ARM_REG_S0 + i, v)
        u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
        u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
        u.reg_write(arm.UC_ARM_REG_APSR, (case % 16) << 28)
        u.reg_write(arm.UC_ARM_REG_FPSCR, (case % 4) << 22)
        entry = F(0x23e94) if original else symbols['dm4310_runtime_round_to_int']
        u.emu_start(entry | 1, 0x30000, count=10000)
        assert u.reg_read(arm.UC_ARM_REG_PC) == 0x30000
        values = [u.reg_read(arm.UC_ARM_REG_R0 + i) for i in range(13)]
        frame = list(struct.unpack('<8I', bytes(u.mem_read(0x2000efe0, 32))))
        lr = u.reg_read(arm.UC_ARM_REG_LR)
        if not original:
            values[12] = normalize(values[12])
            lr = normalize(lr)
            frame = [normalize(v) for v in frame]
        results.append((values, [u.reg_read(arm.UC_ARM_REG_S0 + i) for i in range(32)],
                        u.reg_read(arm.UC_ARM_REG_SP), lr,
                        u.reg_read(arm.UC_ARM_REG_APSR), u.reg_read(arm.UC_ARM_REG_FPSCR),
                        frame, bytes(u.mem_read(A(0x1ffff490), 4))))
    assert results[0] == results[1], (case, hex(floats[0]), results)
print('PASS: 1024 whole float-to-int cases; r0-r12, s0-s31, APSR/FPSCR, SP/LR, normalized 32-byte stack and errno')
