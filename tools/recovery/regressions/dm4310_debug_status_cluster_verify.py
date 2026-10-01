from pathlib import Path
import random

from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_MEM_READ

exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split(
    '\nfor old, name, size in mapping[:1]:')[0])

FACTORY_ENTRY = F(0x26758)
SOURCE_ENTRY = symbols['debug_console_print_status']
FACTORY_PRINTF = F(0x20474)
SOURCE_PRINTF = symbols['debug_console_printf']
MCAN = 0x40029000
fixed_ranges = ((A(0x1ffff000), 0x200), (A(0x1fffa500), 0x200))

def c_string(uc, address):
    result = bytearray()
    while True:
        byte = bytes(uc.mem_read(address, 1))[0]
        if byte == 0:
            return bytes(result)
        result.append(byte)
        address += 1


def format_argument(uc, fmt):
    percent = fmt.find(b'%')
    if percent < 0:
        return ()
    conversion = fmt[-1:]
    if conversion in (b'f', b'F', b'e', b'E', b'g', b'G'):
        return (uc.reg_read(arm.UC_ARM_REG_R2),
                uc.reg_read(arm.UC_ARM_REG_R3))
    return (uc.reg_read(arm.UC_ARM_REG_R1),)


def run(original, fixed_images, plan, fpscr, s17):
    uc = machine(original)
    uc.mem_map(0x40000000, 0x100000)
    for address, data in fixed_images.items():
        uc.mem_write(address, data)
    uc.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
    uc.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    uc.reg_write(arm.UC_ARM_REG_FPSCR, fpscr)
    uc.reg_write(arm.UC_ARM_REG_S16, 0)
    uc.reg_write(arm.UC_ARM_REG_S17, s17)
    uc.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    uc.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    events = []
    mmio_read_counts = {offset: 0 for offset in plan}
    printf = FACTORY_PRINTF if original else SOURCE_PRINTF

    def memory(emu, access, address, size, value, _):
        assert access == UC_MEM_READ
        offset = address - MCAN
        if offset in plan:
            index = mmio_read_counts[offset]
            emu.mem_write(address, plan[offset][index].to_bytes(size,
                                                                 'little'))
            mmio_read_counts[offset] = index + 1
        value = int.from_bytes(emu.mem_read(address, size), 'little')
        events.append(('read', address, size, value))

    def code(emu, address, size, _):
        if address == printf:
            fmt = c_string(emu, emu.reg_read(arm.UC_ARM_REG_R0))
            events.append(('printf', fmt, format_argument(emu, fmt)))
            emu.reg_write(arm.UC_ARM_REG_PC,
                          emu.reg_read(arm.UC_ARM_REG_LR))

    for address, length in fixed_ranges:
        uc.hook_add(UC_HOOK_MEM_READ, memory,
                    begin=address, end=address + length - 1)
    uc.hook_add(UC_HOOK_MEM_READ, memory,
                begin=MCAN, end=MCAN + 0x1f)
    uc.hook_add(UC_HOOK_CODE, code)
    entry = FACTORY_ENTRY if original else SOURCE_ENTRY
    uc.emu_start(entry | 1, 0x30000, count=10000)
    assert uc.reg_read(arm.UC_ARM_REG_PC) == 0x30000
    return (events, uc.reg_read(arm.UC_ARM_REG_FPSCR),
            uc.reg_read(arm.UC_ARM_REG_S16),
            uc.reg_read(arm.UC_ARM_REG_S17))


rng = random.Random(0x26758)
count = 0
for case in range(384):
    fixed_images = {
        address: bytes(rng.getrandbits(8) for _ in range(length))
        for address, length in fixed_ranges
    }
    variant = case % 6
    mode = (case // 6) % 6
    mutable = bytearray(fixed_images[A(0x1fffa500)])
    mutable[0xbc] = variant
    fixed_images[A(0x1fffa500)] = bytes(mutable)
    mutable = bytearray(fixed_images[A(0x1ffff000)])
    mutable[0x140:0x144] = mode.to_bytes(4, 'little')
    fixed_images[A(0x1ffff000)] = bytes(mutable)

    irrelevant = rng.getrandbits(32) & ~0x300
    scenario = case % 4
    if scenario == 0:
        cccr = [irrelevant, irrelevant]
        nbtp = [rng.getrandbits(32) for _ in range(3)]
        dbtp = [0, 0, 0]
    elif scenario == 1:
        cccr = [irrelevant | 0x100, irrelevant | 0x300]
        nbtp = [0, 0, 0]
        dbtp = [rng.getrandbits(32) for _ in range(3)]
    elif scenario == 2:
        cccr = [irrelevant | 0x300, irrelevant | 0x200]
        nbtp = [0, 0, 0]
        dbtp = [0, 0, 0]
    else:
        cccr = [irrelevant | 0x200, irrelevant | 0x300]
        nbtp = [0, 0, 0]
        dbtp = [rng.getrandbits(32) for _ in range(3)]
    plan = {0x18: cccr, 0x1c: nbtp, 0x0c: dbtp}
    fpscr = (case & 15) << 22
    s17 = rng.getrandbits(32)
    factory_result = run(True, fixed_images, plan, fpscr, s17)
    source_result = run(False, fixed_images, plan, fpscr, s17)
    assert factory_result == source_result, (case, plan, factory_result,
                                              source_result)
    count += 1

print(f'PASS: {count} factory 0x26758/source diagnostic cases; dynamic '
      'CCCR/timing reads, all fixed operands, output ABI, modes and FPSCR')
