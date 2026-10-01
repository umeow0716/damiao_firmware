from pathlib import Path
import struct
from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_READ

exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split(
    '\nfor old, name, size in mapping[:1]:')[0])

for case in range(32):
    u = machine(False)
    config = 0x20009000
    dispatch = 0x2000b200
    references = 0x20008000
    node = 0x8123 + case
    selector = 0x9234 + case
    u.mem_write(config + 0x20, struct.pack('<H', node))
    u.mem_write(config + 0x8c, struct.pack('<H', selector))
    u.mem_write(dispatch, struct.pack('<I', 0x2c001))
    # Struct prefix: controller/config/status/IR/dispatch/response_dispatch.
    u.mem_write(references, struct.pack('<6I', 0, config, 0, 0, 0, dispatch))
    u.reg_write(arm.UC_ARM_REG_R0, 1)
    u.reg_write(arm.UC_ARM_REG_R1, references)
    u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    reads = []
    def read(uc, access, address, size, value, _):
        reads.append((address, size))
    def callback(uc, address, size, _):
        assert uc.reg_read(arm.UC_ARM_REG_R0) == selector
        assert uc.reg_read(arm.UC_ARM_REG_R1) == node
        uc.emu_stop()
    for lo, hi in ((config, config + 0x93), (dispatch, dispatch + 7),
                   (A(0x1ffff29c), A(0x1ffff2a3))):
        u.hook_add(UC_HOOK_MEM_READ, read, begin=lo, end=hi)
    u.hook_add(UC_HOOK_CODE, callback, begin=0x2c000, end=0x2c000)
    u.emu_start(symbols['board_mcan_reinitialize_live'] | 1,
                0x30000, count=1000)
    assert u.reg_read(arm.UC_ARM_REG_PC) == 0x2c000
    assert reads == [(config + 0x20, 2), (dispatch, 4),
                     (config + 0x8c, 2)], reads
print('PASS: 32 WRITE reinitialization owner/order/full-halfword ABI cases')
