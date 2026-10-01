from pathlib import Path
import struct
from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_WRITE

exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split(
    '\nfor old, name, size in mapping[:1]:')[0])

for case in range(64):
    relocated = case >= 32
    controller = 0x20008000 if relocated else 0x40029000
    config = 0x20009000 if relocated else A(0x1fffa5c8)
    status = 0x2000a000 if relocated else A(0x1ffff1f0)
    dispatch = 0x2000b000 if relocated else A(0x1ffff29c)
    bus_off = bool(case & 1)
    reinitialize = bool(case & 2)
    cccr = [0, 1, 0xffffffff, 0xa5a5a5a5][(case >> 1) & 3]
    expected = []
    for original in (True, False):
        u = machine(original)
        if original:
            u.mem_write(FIXED_IMAGE_BASE, factory[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE])
        else:
            for section in elf.iter_sections():
                if (FIXED_IMAGE_BASE <= section['sh_addr'] < FACTORY_STATE_BASE and
                        section['sh_type'] != 'SHT_NOBITS'):
                    u.mem_write(section['sh_addr'], section.data())
        if not relocated:
            u.mem_map(0x40029000, 0x1000)
        u.mem_map(0xe000e000, 0x1000)
        u.mem_write(controller, bytes(0x1000))
        u.mem_write(config, bytes(0x98))
        u.mem_write(status, bytes(0x4c))
        u.mem_write(dispatch, bytes(8))
        u.mem_write(A(0x1fff8b80), struct.pack('<III', controller, config, status))
        u.mem_write(A(0x1fff9878), struct.pack('<I', dispatch))
        u.mem_write(dispatch, struct.pack('<I', 0x2c001))
        u.mem_write(config + 0x20, struct.pack('<H', 0x123))
        u.mem_write(config + 0x8c, struct.pack('<H', 0x456))
        u.mem_write(controller + 0x18, struct.pack('<I', cccr))
        u.mem_write(controller + 0x50,
                    struct.pack('<I', (0x02000000 if bus_off else 0) |
                                (0x00800000 if reinitialize else 0)))
        u.mem_write(status + 0x30, struct.pack('<I', 7))
        u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
        u.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
        u.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
        trace = []
        callbacks = []
        done = [False]
        def callback(uc, _address, _size, _):
            callbacks.append((uc.reg_read(arm.UC_ARM_REG_R0),
                              uc.reg_read(arm.UC_ARM_REG_R1)))
            uc.reg_write(arm.UC_ARM_REG_PC, uc.reg_read(arm.UC_ARM_REG_LR))
        def hook(uc, access, address, size, value, _):
            observed = value if access == UC_MEM_WRITE else int.from_bytes(
                uc.mem_read(address, size), 'little')
            trace.append((access, address, size, observed))
            if access == UC_MEM_WRITE and address == 0xe000e280:
                done[0] = True
                uc.emu_stop()
        for lo, hi in ((A(0x1fff8b80), A(0x1fff8b8b)),
                       (A(0x1fff9878), A(0x1fff987b)),
                       (controller, controller + 0xff),
                       (config, config + 0x97), (status, status + 0x4b),
                       (dispatch, dispatch + 7),
                       (0xe000e280, 0xe000e283)):
            u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, hook,
                       begin=lo, end=hi)
        u.hook_add(UC_HOOK_CODE, callback, begin=0x2c000, end=0x2c000)
        u.emu_start(A(0x1fff88b9), 0x30000, count=100000)
        assert done[0], (case, original, hex(u.reg_read(arm.UC_ARM_REG_PC)))
        expected.append((trace, callbacks, bytes(u.mem_read(controller, 0x100)),
                         bytes(u.mem_read(status, 0x4c))))
    assert expected[0] == expected[1], (case, expected)

print('PASS: 64 real IRQ003 no-RX/reinit/bus-off tails; retained controller/'
      'config/status/callback pools, callback ABI, ordered trace and relocation')
