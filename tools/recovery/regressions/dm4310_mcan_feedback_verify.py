from pathlib import Path
import struct
from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE

exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split(
    '\nfor old, name, size in mapping[:1]:')[0])

def f32(u, address, value):
    u.mem_write(address, struct.pack('<f', value))

for case in range(64):
    relocated = case >= 32
    controller = 0x20008000 if relocated else 0x40029000
    config = 0x20009000 if relocated else A(0x1fffa5c8)
    status = 0x2000a000 if relocated else A(0x1ffff1f0)
    dispatch = 0x2000b000 if relocated else A(0x1ffff29c)
    response = 0x2000b200 if relocated else dispatch
    scratch = 0x2000b400 if relocated else A(0x1fffcc4c)
    sample = 0x2000b600 if relocated else A(0x1ffff104)
    motor = 0x2000b800 if relocated else A(0x1ffff088)
    index = case & 0x1f
    node = 0x23
    words = (node << 18, 0x10203040 ^ case,
             0x01234567 ^ case, 0x76543210 ^ (case << 8))
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
        objects = ((controller, 0x1000), (config, 0x98), (status, 0x4c),
                   (dispatch, 0xa4), (scratch, 0x20), (sample, 0xa4),
                   (motor, 0x7c))
        for address, size in objects:
            u.mem_write(address, bytes(size))
        if response != dispatch:
            u.mem_write(response, bytes(0xa4))
        u.mem_write(A(0x1fff8b80), struct.pack('<IIIII', controller, config,
                                            status, dispatch, scratch))
        u.mem_write(A(0x1fff8b94), struct.pack('<I', sample))
        u.mem_write(A(0x1fff8b50), struct.pack('<I', motor))
        u.mem_write(A(0x1fff8f90), struct.pack('<I', response))
        u.mem_write(A(0x1fff8b48), struct.pack('<I', 0))
        u.mem_write(A(0x1fff8b68), struct.pack('<I', 0x40c90fdb))
        u.mem_write(config + 0x1c, struct.pack('<H', 0x456))
        u.mem_write(config + 0x20, struct.pack('<I', node))
        f32(u, config + 0x54, 12.5)
        f32(u, config + 0x58, 30.0)
        f32(u, config + 0x5c, 10.0)
        u.mem_write(status + 4, struct.pack('<I', (case >> 1) & 1))
        f32(u, motor + 0x18, [-6.0, 0.0, 5.5, 12.0][case & 3])
        f32(u, motor + 0x1c, [-20.0, 0.0, 12.0, 29.0][(case >> 2) & 3])
        f32(u, motor + 0x30, [-9.0, 0.0, 4.0, 9.0][(case >> 4) & 3])
        u.mem_write(motor + 0x34, struct.pack('<I',
                    0x3f800000 if case & 1 else 0x40000000))
        u.mem_write(motor + 0x38, struct.pack('<I', 0))
        f32(u, motor + 0x40, 35.75)
        u.mem_write(sample + 0x80, struct.pack('<I', (case >> 2) & 0xf))
        f32(u, sample + 0x84, 42.25)
        u.mem_write(response + 4, struct.pack('<I', 0x2c001))
        u.mem_write(controller + 0x50, struct.pack('<I', 1))
        u.mem_write(controller + 0xa4, struct.pack('<I', index << 8))
        element = 0x4002b080 + index * 72
        u.mem_write(element, struct.pack('<4I', *words))
        u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
        u.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
        u.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
        u.reg_write(arm.UC_ARM_REG_FPSCR, (case & 3) << 22)
        trace = []
        callback_args = []
        branch_target_quirk = [0]
        def memory(uc, access, address, size, value, _):
            if (access == 16 and address == motor + 0x38 and size == 4):
                branch_target_quirk[0] += 1
                return
            trace.append((access, address, size, value if access == 17 else
                          int.from_bytes(uc.mem_read(address, size), 'little')))
        def callback(uc, _address, _size, _):
            callback_args.append(tuple(uc.reg_read(reg) for reg in
                (arm.UC_ARM_REG_R0, arm.UC_ARM_REG_R1, arm.UC_ARM_REG_R2)))
            uc.emu_stop()
        ranges = [(A(0x1fff8b48), A(0x1fff8b4b)), (A(0x1fff8b50), A(0x1fff8b53)),
                  (A(0x1fff8b68), A(0x1fff8b6b)), (A(0x1fff8b80), A(0x1fff8b97)),
                  (A(0x1fff8f90), A(0x1fff8f93)),
                  (controller, controller + 0xff), (config, config + 0x97),
                  (status, status + 0x4b), (dispatch, dispatch + 0xa3),
                  (scratch, scratch + 0x1f), (sample, sample + 0xa3),
                  (motor, motor + 0x7b), (element, element + 15)]
        if response != dispatch:
            ranges.append((response, response + 0xa3))
        for lo, hi in ranges:
            u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                       begin=lo, end=hi)
        u.hook_add(UC_HOOK_CODE, callback, begin=0x2c000, end=0x2c000)
        u.emu_start(A(0x1fff88b9), 0x30000, count=300000)
        assert len(callback_args) == 1, (case, original,
                                         hex(u.reg_read(arm.UC_ARM_REG_PC)))
        if original:
            assert branch_target_quirk[0] in (0, 1), (
                case, original, branch_target_quirk)
        else:
            assert branch_target_quirk[0] == 1, (
                case, original, branch_target_quirk)
        results.append((trace, callback_args,
                        bytes(u.mem_read(status, 0x4c)),
                        bytes(u.mem_read(response, 0xa4)),
                        bytes(u.mem_read(scratch, 0x20)),
                        u.reg_read(arm.UC_ARM_REG_FPSCR)))
    if results[0] != results[1]:
        for component, (left, right) in enumerate(zip(*results)):
            if left != right:
                if component == 0:
                    for index, pair in enumerate(zip(left, right)):
                        if pair[0] != pair[1]:
                            raise AssertionError((case, component, index, pair))
                    raise AssertionError((case, component, len(left), len(right)))
                raise AssertionError((case, component, left, right))

print('PASS: 64 real IRQ003 addressed feedback-only paths through send ABI; '
      'relocated RX/response dispatch, config/status/sample/motor/scratch')
