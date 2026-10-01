from pathlib import Path
import random

from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE

exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split(
    '\nfor old, name, size in mapping[:1]:')[0])

FACTORY_ENTRY = F(0x266e8)
FACTORY_DELAY = F(0x21f28)
FACTORY_PRINTF = F(0x20474)
SOURCE_ENTRY = symbols['platform_check_startup_bus_voltage']
SOURCE_DELAY = symbols['board_delay_ms']
SOURCE_PRINTF = symbols['debug_console_printf']
BUS = A(0x1ffff16c)
GPIO_START = 0x40053800
GPIO_END = 0x400538ff


def c_string(uc, address):
    result = bytearray()
    while True:
        byte = bytes(uc.mem_read(address, 1))[0]
        if byte == 0:
            return bytes(result)
        result.append(byte)
        address += 1


def run(original, initial_bus, reloads, gpio_seed):
    uc = machine(original)
    uc.mem_map(0x40000000, 0x100000)
    uc.mem_write(GPIO_START, gpio_seed)
    uc.mem_write(BUS, initial_bus.to_bytes(4, 'little'))
    uc.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
    uc.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    uc.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    uc.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    events = []
    printf_count = 0
    reload_index = 0

    def memory(emu, access, address, size, value, _):
        if access == 17:
            value = int.from_bytes(emu.mem_read(address, size), 'little')
            events.append(('read', address, size, value))
        else:
            events.append(('write', address, size, value))

    def code(emu, address, size, _):
        nonlocal printf_count, reload_index
        delay = FACTORY_DELAY if original else SOURCE_DELAY
        printf = FACTORY_PRINTF if original else SOURCE_PRINTF
        if address == delay:
            events.append(('delay', emu.reg_read(arm.UC_ARM_REG_R0)))
            if reload_index < len(reloads):
                emu.mem_write(BUS, reloads[reload_index].to_bytes(4, 'little'))
                reload_index += 1
            emu.reg_write(arm.UC_ARM_REG_PC,
                          emu.reg_read(arm.UC_ARM_REG_LR))
        elif address == printf:
            events.append(('printf', c_string(emu,
                                               emu.reg_read(arm.UC_ARM_REG_R0)),
                           emu.reg_read(arm.UC_ARM_REG_R2),
                           emu.reg_read(arm.UC_ARM_REG_R3)))
            printf_count += 1
            if printf_count == len(reloads):
                emu.emu_stop()
            else:
                emu.reg_write(arm.UC_ARM_REG_PC,
                              emu.reg_read(arm.UC_ARM_REG_LR))

    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=BUS, end=BUS + 3)
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=GPIO_START, end=GPIO_END)
    uc.hook_add(UC_HOOK_CODE, code)
    entry = FACTORY_ENTRY if original else SOURCE_ENTRY
    try:
        uc.emu_start(entry | 1, 0x30000, count=10000)
    except Exception as error:
        raise AssertionError((original, hex(entry),
                              hex(uc.reg_read(arm.UC_ARM_REG_PC)), events)) from error
    returned = uc.reg_read(arm.UC_ARM_REG_PC) == 0x30000
    return events, bytes(uc.mem_read(GPIO_START, 0x100)), returned


rng = random.Random(0x266e8)
threshold_cases = [
    0, 1, BUS_OVERVOLTAGE_BITS - 1, BUS_OVERVOLTAGE_BITS,
    BUS_OVERVOLTAGE_BITS + 1, 0x7f7fffff,
    0x7f800000, 0x7fc00000, 0x80000000, 0xbf800000, 0xff800000,
    0xffffffff,
] + [rng.getrandbits(32) for _ in range(1024)]

for case, bits in enumerate(threshold_cases):
    reloads = ([rng.getrandbits(32)]
               if BUS_OVERVOLTAGE_BITS < bits < 0x80000000 else [])
    seed = bytes(rng.getrandbits(8) for _ in range(0x100))
    factory_result = run(True, bits, reloads, seed)
    source_result = run(False, bits, reloads, seed)
    assert factory_result == source_result, (case, hex(bits), factory_result,
                                              source_result)
    assert factory_result[2] == (not reloads), (case, hex(bits), factory_result)

for case in range(128):
    bits = rng.randrange(BUS_OVERVOLTAGE_BITS + 1, 0x80000000)
    reloads = [rng.getrandbits(32) for _ in range(3)]
    seed = bytes(rng.getrandbits(8) for _ in range(0x100))
    factory_result = run(True, bits, reloads, seed)
    source_result = run(False, bits, reloads, seed)
    assert factory_result == source_result, (case, hex(bits), factory_result,
                                              source_result)
    assert not factory_result[2]

print('PASS: 1036 threshold cases plus 128 three-iteration overvoltage cases; '
      'ordered fixed-SRAM/GPIO accesses, delay and printf ABI match')
