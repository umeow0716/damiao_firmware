from pathlib import Path
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
for original in (True, False):
    u = machine(original)
    u.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
    u.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    u.reg_write(arm.UC_ARM_REG_S0, 0x7f800000)
    u.reg_write(arm.UC_ARM_REG_R1, 0x3f800000)
    u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    entry = 0x23e94 if original else symbols['dm4310_runtime_round_to_int']
    u.emu_start(entry | 1, 0x30000, count=10000)
    assert u.reg_read(arm.UC_ARM_REG_PC) == 0x30000
    print('factory' if original else 'source', 'R0', hex(u.reg_read(arm.UC_ARM_REG_R0)),
          'FPSCR', hex(u.reg_read(arm.UC_ARM_REG_FPSCR)))
