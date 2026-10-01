from pathlib import Path
import struct
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_WRITE

exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split(
    '\nfor old, name, size in mapping[:1]:')[0])

for case in range(96):
    relocated = case >= 48
    controller = 0x20008000 if relocated else 0x40029000
    config = 0x20009000 if relocated else A(0x1fffa5c8)
    status = 0x2000a000 if relocated else A(0x1ffff1f0)
    dispatch = 0x2000b000 if relocated else A(0x1ffff29c)
    id_scratch = 0x2000b200 if relocated else A(0x1fffcc4c)
    sample = 0x2000b400 if relocated else A(0x1ffff104)
    motor = 0x2000b600 if relocated else A(0x1ffff088)
    index = case & 0x3f
    received_id = 0x321
    words = (received_id << 18,
             0x12340000 | case,
             0x89abcdef ^ case,
             0x76543210 ^ (case << 8))
    results = []
    for original in (True, False):
        u = machine(original)
        if original:
            u.mem_write(FIXED_IMAGE_BASE, factory[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE])
        else:
            for section in elf.iter_sections():
                if (FIXED_IMAGE_BASE <= section['sh_addr'] < FACTORY_STATE_BASE and
                        section['sh_type'] != 'SHT_NOBITS'):
                    u.mem_write(section['sh_addr'], section.data())
        for page in (0x40029000, 0x4002b000, 0x4002c000, 0xe000e000):
            u.mem_map(page, 0x1000)
        for address, size in ((controller, 0x1000), (config, 0x98),
                              (status, 0x4c), (dispatch, 0xa4),
                              (id_scratch, 4), (sample, 0xa4), (motor, 0x7c)):
            u.mem_write(address, bytes(size))
        u.mem_write(A(0x1fff8b80), struct.pack('<IIIII', controller, config,
                                            status, dispatch, id_scratch))
        u.mem_write(A(0x1fff8b94), struct.pack('<I', sample))
        u.mem_write(A(0x1fff8b50), struct.pack('<I', motor))
        u.mem_write(A(0x1fff8b48), struct.pack('<f', -0.0 if case & 1 else 0.0))
        u.mem_write(A(0x1fff8b68), struct.pack('<I', 0x40c90fdb))
        u.mem_write(config + 0x20, struct.pack('<I', 0x123))
        u.mem_write(controller + 0x50, struct.pack('<I', 1))
        u.mem_write(controller + 0xa4, struct.pack('<I', index << 8))
        element = 0x4002b080 + index * 72
        u.mem_write(element, struct.pack('<4I', *words))
        u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
        u.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
        u.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
        trace = []
        done = [False]
        def hook(uc, access, address, size, value, _):
            observed = value if access == UC_MEM_WRITE else int.from_bytes(
                uc.mem_read(address, size), 'little')
            trace.append((access, address, size, observed))
            if access == UC_MEM_WRITE and address == 0xe000e280:
                done[0] = True
                uc.emu_stop()
        ranges = [(A(0x1fff8b48), A(0x1fff8b4b)),
                  (A(0x1fff8b50), A(0x1fff8b53)),
                  (A(0x1fff8b68), A(0x1fff8b6b)),
                  (A(0x1fff8b80), A(0x1fff8b97)),
                  (controller, controller + 0xff),
                  (config, config + 0x97), (status, status + 0x4b),
                  (dispatch, dispatch + 0xa3),
                  (id_scratch, id_scratch + 3),
                  (sample, sample + 0xa3), (motor, motor + 0x7b),
                  (element, element + 15), (0xe000e280, 0xe000e283)]
        if relocated:
            ranges += [(A(0x1ffff088), A(0x1ffff1a7)),
                       (A(0x1ffff1f0), A(0x1ffff23b)),
                       (A(0x1ffff29c), A(0x1ffff33f)),
                       (A(0x1fffa5c8), A(0x1fffa65f)),
                       (A(0x1fffcc4c), A(0x1fffcc4f)),
                       (0x40029000, 0x400290ff)]
        for lo, hi in ranges:
            u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, hook,
                       begin=lo, end=hi)
        u.emu_start(A(0x1fff88b9), 0x30000, count=300000)
        assert done[0], (case, original, hex(u.reg_read(arm.UC_ARM_REG_PC)))
        results.append((trace, bytes(u.mem_read(controller, 0x100)),
                        bytes(u.mem_read(dispatch, 0xa4)),
                        bytes(u.mem_read(id_scratch, 4))))
    assert results[0] == results[1], (case, results)

print('PASS: 96 real IRQ003 unknown-node RX paths; FIFO acquisition and '
      'retained controller/config/status/dispatch/id/sample/motor roots')
