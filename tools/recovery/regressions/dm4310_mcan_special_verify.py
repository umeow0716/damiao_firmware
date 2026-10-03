from pathlib import Path
import struct
from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE

exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split(
    '\nfor old, name, size in mapping[:1]:')[0])

def f32(u, address, value):
    u.mem_write(address, struct.pack('<f', value))

startup = machine(True)
startup.mem_write(0x20000, factory)
startup.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
startup.emu_start(F(0x20259), F(0x20368), count=1000000)
assert startup.reg_read(arm.UC_ARM_REG_PC) == F(0x20368)
factory_sbox = bytes(startup.mem_read(A(0x1fffc678), 0x100))
source_sbox = elf.get_section_by_name('.aes_sbox').data()
assert factory_sbox == source_sbox

write_vectors = (
    (0x00, (0x42000000, 0x41200000, 0xffffffff, 0x7fc00001)),
    (0x01, (0x3f800000, 0x00000000, 0xbf800000, 0x7fc00001)),
    (0x02, (0x42c80000, 0x42a00000, 0x43480000, 0x00000000)),
    (0x03, (0x3f000000, 0x00000000, 0x3f800000, 0x7fc00001)),
    (0x04, (0x3f800000, 0x00000000, 0xbf800000, 0x7fc00001)),
    (0x05, (0xbf800000, 0x80000000, 0x3f800000, 0x7fc00001)),
    (0x06, (0x3f800000, 0x00000000, 0xbf800000, 0x7fc00001)),
    (0x07, (0x00000000, 0x000007ff, 0x00000800, 0xffffffff)),
    (0x08, (0x00000000, 0x000007ff, 0x00000823, 0xffffffff)),
    (0x09, (0x00000000, 0x12345678, 0xffffffff, 0x7fc00001)),
    (0x0a, (0x00000001, 0x00000004, 0x00000000, 0x00000005)),
    (0x15, (0x3f800000, 0x00000000, 0xbf800000, 0x7fc00001)),
    (0x16, (0x3f800000, 0x00000000, 0xbf800000, 0x7fc00001)),
    (0x17, (0x3f800000, 0x00000000, 0xbf800000, 0x7fc00001)),
    (0x18, (0x42c80000, 0x461c4000, 0x00000000, 0x461c4001)),
    (0x19, (0x00000000, 0x3f800000, 0xbf800000, 0x7fc00001)),
    (0x1a, (0x00000000, 0x3f800000, 0xbf800000, 0x7fc00001)),
    (0x1b, (0x00000000, 0x3f800000, 0xbf800000, 0x7fc00001)),
    (0x1c, (0x00000000, 0x3f800000, 0xbf800000, 0x7fc00001)),
    (0x1d, (0x41800000, 0x00000000, 0x42800000, 0x7fc00001)),
    (0x1e, (0x3f000000, 0x3f800000, 0x00000000, 0x40000000)),
    (0x1f, (0x3f800000, 0x42000000, 0x00000000, 0x42000001)),
    (0x20, (0x3f800000, 0x43fa0000, 0x00000000, 0xbf800000)),
    (0x21, (0x42c80000, 0x461c4000, 0x00000000, 0x461c4001)),
    (0x22, (0x3f800000, 0x461c4000, 0x00000000, 0x461c4001)),
    (0x23, (0x00000000, 0x0000000b, 0x0000000c, 0xffffffff)),
)
scenarios = tuple((address, incoming) for address, values in write_vectors
                  for incoming in values)

for case in range(len(scenarios) * 2):
    relocated = case >= len(scenarios)
    controller = 0x20008000 if relocated else 0x40029000
    config = 0x20009000 if relocated else A(0x1fffa5c8)
    status = 0x2000a000 if relocated else A(0x1ffff1f0)
    dispatch = 0x2000b000 if relocated else A(0x1ffff29c)
    response = 0x2000b200 if relocated else dispatch
    scratch = 0x2000b400 if relocated else A(0x1fffcc4c)
    sample = 0x2000b600 if relocated else A(0x1ffff104)
    motor = 0x2000b800 if relocated else A(0x1ffff088)
    parameter_scratch = 0x2000ba00 if relocated else scratch
    calibration = 0x2000bc00 if relocated else A(0x1ffff078)
    zero_staging = 0x2000be00 if relocated else A(0x1fffa5c0)
    output = 0x2000c000 if relocated else A(0x1ffff1a8)
    scenario = case % len(scenarios)
    address, incoming = scenarios[scenario]
    index = case & 0x1f
    node = 0x23
    header = (address << 24) | (0x55 << 16) | node
    words = (0x7ff << 18, 0x10203040 ^ case, header, incoming)
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
        for page in (0x03000000, 0x40010000, 0x4001c000, 0x40029000,
                     0x4002b000, 0x4002c000,
                     0x4003a000, 0x40048000, 0x40051000, 0x40053000,
                     0x40054000,
                     0xe000e000):
            u.mem_map(page, 0x1000)
        u.mem_write(0x4003a081, b'\x80')
        u.mem_write(0x4001cc00, struct.pack('<I', 0x80))
        # UID is zero-filled in this harness. Slot zero contains its factory
        # AES-CTR binding token so parameter derivation returns normally.
        u.mem_write(0x03000c00,
                    bytes.fromhex('6c0a64f59cd46c2217ae8a9e12d7ad0c'))
        u.mem_write(A(0x1fffc678), factory_sbox if original else source_sbox)
        objects = ((controller, 0x1000), (config, 0x98), (status, 0x4c),
                   (dispatch, 0xa4), (scratch, 0x20), (sample, 0xa4),
                   (motor, 0x7c), (calibration, 0x10),
                   (zero_staging, 8), (output, 0x48))
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
        u.mem_write(A(0x1fff8f9c), struct.pack('<I', parameter_scratch))
        u.mem_write(A(0x1fff93d0), struct.pack('<III', calibration,
                                            zero_staging, output))
        if parameter_scratch != scratch:
            u.mem_write(parameter_scratch, bytes(0x20))
        u.mem_write(config + 0x1c, struct.pack('<H', 0x456))
        u.mem_write(config + 0x20, struct.pack('<I', node))
        for word in range(0x25):
            if word not in (7, 8):
                u.mem_write(config + word * 4,
                            struct.pack('<I', 0x41000000 + word * 0x10101 + case))
        u.mem_write(config + 0x1c, struct.pack('<I', 0x456))
        u.mem_write(config + 0x20, struct.pack('<I', node))
        f32(u, config + 0x54, 12.5)
        f32(u, config + 0x58, 30.0)
        f32(u, config + 0x5c, 10.0)
        u.mem_write(status, struct.pack('<II', 0xa5a50000 | case,
                                        1 + ((case >> 2) & 1)))
        f32(u, motor + 0x18, (-6.0, 0.0, 5.5, 12.0)[case & 3])
        f32(u, motor + 0x1c, (-20.0, 0.0, 12.0, 29.0)[(case >> 2) & 3])
        f32(u, motor + 0x30, (-9.0, 0.0, 4.0, 9.0)[(case >> 4) & 3])
        u.mem_write(motor + 0x34, struct.pack('<I',
                    0x3f800000 if case & 1 else 0x40000000))
        u.mem_write(motor + 0x38, struct.pack('<II', 0,
                                              0x56780000 | case))
        f32(u, motor + 0x40, 35.75)
        u.mem_write(motor + 0x34, struct.pack('<I',
                    0x3f800000 if case & 1 else 0x40000000))
        f32(u, motor + 0x18, -3.25 + case * 0.0625)
        f32(u, output + 0x3c, 2.75 - case * 0.03125)
        u.mem_write(calibration, struct.pack('<4I',
                    0x3f010203 ^ case, 0x40040506 ^ case,
                    0xbf070809 ^ case, 0xc00a0b0c ^ case))
        u.mem_write(zero_staging, struct.pack('<II',
                    0x3f123456 ^ case, 0x40123456 ^ case))
        u.mem_write(sample + 0x80, struct.pack('<I', (case >> 2) & 7))
        f32(u, sample + 0x84, 42.25)
        f32(u, sample + 0x64, (-0.75, -0.125, 0.25, 0.875)[(case >> 3) & 3])
        u.mem_write(response + 4, struct.pack('<I', 0x2c001))
        u.mem_write(response + 0x64, struct.pack('<II', header, incoming))
        u.mem_write(controller + 0x50, struct.pack('<I', 1))
        u.mem_write(controller + 0xa4, struct.pack('<I', index << 8))
        element = 0x4002b080 + index * 72
        u.mem_write(element, struct.pack('<4I', *words))
        u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
        u.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
        u.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
        u.reg_write(arm.UC_ARM_REG_FPSCR, (case & 3) << 22)
        trace = []
        factory_write_pcs = []
        callback_args = []
        def memory(uc, access, address, size, value, _):
            observed = value if access == 17 else int.from_bytes(
                uc.mem_read(address, size), 'little')
            if not original and size == 4:
                observed = normalize_source_flash_value(
                    observed, symbols, symbol_sizes)
            trace.append((access, address, size, observed))
        def callback(uc, _address, _size, _):
            callback_args.append(tuple(uc.reg_read(reg) for reg in
                (arm.UC_ARM_REG_R0, arm.UC_ARM_REG_R1, arm.UC_ARM_REG_R2)))
            uc.emu_stop()
        def rng_ready(uc, access, address, size, value, _):
            if address == 0x4003a080:
                uc.mem_write(0x4003a081, b'\x80')
        def factory_rng_ready(uc, address, _size, _):
            uc.reg_write(arm.UC_ARM_REG_APSR,
                         uc.reg_read(arm.UC_ARM_REG_APSR) | 0x80000000)
        def factory_token(uc, address, _size, _):
            if case == 56:
                print('TOKEN WORDS', [hex(uc.reg_read(reg)) for reg in
                    (arm.UC_ARM_REG_R3, arm.UC_ARM_REG_R2,
                     arm.UC_ARM_REG_R0, arm.UC_ARM_REG_R1,
                     arm.UC_ARM_REG_R4)],
                     bytes(uc.mem_read(0x03000c00, 16)).hex())
        def write_pc(uc, address, _size, _):
            factory_write_pcs.append((address,
                uc.reg_read(arm.UC_ARM_REG_R0),
                uc.reg_read(arm.UC_ARM_REG_R7),
                uc.reg_read(arm.UC_ARM_REG_R8),
                uc.reg_read(arm.UC_ARM_REG_R9)))
            # Unicorn skips the 16-bit STRB at A(0x1fff9654) after the preceding
            # wide LDRH. Execute that decoded instruction before 0x9656.
            if address == A(0x1fff9656):
                target = uc.reg_read(arm.UC_ARM_REG_R7) + 8
                value = uc.reg_read(arm.UC_ARM_REG_R0) & 0xff
                if not trace or trace[-1][:3] != (17, target, 1):
                    trace.append((17, target, 1, value))
                    uc.mem_write(target, bytes((value,)))
        ranges = [(A(0x1fff8b48), A(0x1fff8b4b)), (A(0x1fff8b50), A(0x1fff8b53)),
                  (A(0x1fff8b68), A(0x1fff8b6b)), (A(0x1fff8b80), A(0x1fff8b97)),
                  (A(0x1fff8f9c), A(0x1fff8f9f)),
                  (A(0x1fff93d0), A(0x1fff93db)),
                  (A(0x1fff8f90), A(0x1fff8f93)),
                  (controller, controller + 0xff), (config, config + 0x97),
                  (status, status + 0x4b), (dispatch, dispatch + 0xa3),
                  (scratch, scratch + 0x1f), (sample, sample + 0xa3),
                  (motor, motor + 0x7b), (calibration, calibration + 0xf),
                  (zero_staging, zero_staging + 7),
                  (output, output + 0x47),
                  (0x4002b000, 0x4002b007),
                  (0xe000e000, 0xe000efff), (element, element + 15)]
        if parameter_scratch != scratch:
            ranges.append((parameter_scratch, parameter_scratch + 0x1f))
        if response != dispatch:
            ranges.append((response, response + 0xa3))
        for lo, hi in ranges:
            u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                       begin=lo, end=hi)
        u.hook_add(UC_HOOK_CODE, callback, begin=0x2c000, end=0x2c000)
        u.hook_add(UC_HOOK_CODE, callback,
                   begin=A(0x1fff9acc), end=A(0x1fff9acc))
        u.hook_add(UC_HOOK_CODE, callback,
                   begin=A(0x1fff9ba0), end=A(0x1fff9ba0))
        u.hook_add(UC_HOOK_MEM_WRITE, rng_ready,
                   begin=0x4003a080, end=0x4003a080)
        if original:
            u.hook_add(UC_HOOK_CODE, write_pc,
                       begin=A(0x1fff921e), end=A(0x1fff968c))
            u.hook_add(UC_HOOK_CODE, factory_rng_ready,
                       begin=F(0x21f46), end=F(0x21f46))
            u.hook_add(UC_HOOK_CODE, factory_token,
                       begin=F(0x26f70), end=F(0x26f70))
        try:
            u.emu_start(A(0x1fff88b9), 0x30000, count=3000000)
        except Exception:
            print('FAILED CASE', case, hex(header), original,
                  hex(u.reg_read(arm.UC_ARM_REG_PC)),
                  'regs', [hex(u.reg_read(arm.UC_ARM_REG_R0 + i))
                           for i in range(13)])
            raise
        assert len(callback_args) == 1, (case, original,
                                         hex(u.reg_read(arm.UC_ARM_REG_PC)),
                                         bytes(u.mem_read(0x4003a080, 2)))
        def snapshot(address, size):
            data = bytes(u.mem_read(address, size))
            return (normalize_source_flash_words(data, symbols, symbol_sizes)
                    if not original else data)
        results.append((trace, callback_args,
                        snapshot(status, 0x4c),
                        snapshot(sample, 0xa4),
                        snapshot(motor, 0x7c),
                        snapshot(parameter_scratch, 0x20),
                        snapshot(calibration, 0x10),
                        snapshot(zero_staging, 8),
                        snapshot(output, 0x48),
                        snapshot(response, 0xa4),
                        snapshot(scratch, 0x20),
                        u.reg_read(arm.UC_ARM_REG_FPSCR)))
    if results[0] != results[1]:
        for component, (left, right) in enumerate(zip(*results)):
            if left != right:
                if component == 0:
                    for item, pair in enumerate(zip(left, right)):
                        if pair[0] != pair[1]:
                            raise AssertionError((case, component, item, pair,
                                left[max(0, item - 8):item + 9],
                                right[max(0, item - 8):item + 9],
                                [x for x in left if x[1] == response + 8],
                                [x for x in right if x[1] == response + 8]))
                    raise AssertionError((case,
                                          component, len(left), len(right)))
                raise AssertionError((case,
                                      component, left, right))

print(f'PASS: {len(scenarios) * 2} real IRQ003 parameter WRITE paths; accepted/rejected '
      'values, relocated owners, fixed response/filter side effects and FPSCR')
