from pathlib import Path
import random

from unicorn import (UC_HOOK_CODE, UC_HOOK_MEM_INVALID, UC_HOOK_MEM_READ,
                     UC_HOOK_MEM_WRITE, UC_MEM_READ)

exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split(
    '\nfor old, name, size in mapping[:1]:')[0])

FACTORY_ENTRY = F(0x22658)
SOURCE_ENTRY = symbols['platform_load_motor_calibration.part.0']
FACTORY_PRINTF = F(0x20474)
SOURCE_PRINTF = symbols['debug_console_printf']
MOTOR_FLASH = 0x3c000
ZERO_FLASH = 0x36000
OUTPUT_FLASH = 0x38000
TABLE_FLASH = 0x3a000
CORRECTION = A(0x1fffc84c)
TABLE = A(0x1fffd078)


def loader_machine(original):
    uc = machine(original)
    uc.mem_map(0x40000000, 0x100000)
    if original:
        uc.mem_write(FIXED_IMAGE_BASE, factory[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE])
    else:
        for section in elf.iter_sections():
            if (A(0x1fff0000) <= section['sh_addr'] < 0x20000000 and
                    section['sh_type'] != 'SHT_NOBITS'):
                uc.mem_write(section['sh_addr'], section.data())
    return uc


def c_string(uc, address):
    result = bytearray()
    while True:
        byte = bytes(uc.mem_read(address, 1))[0]
        if byte == 0:
            return bytes(result)
        result.append(byte)
        address += 1


def run(original, motor_record, zero_record, output_record, table_record,
        runtime, correction, sine, gpio, fpscr):
    uc = loader_machine(original)
    uc.mem_write(MOTOR_FLASH, motor_record)
    uc.mem_write(ZERO_FLASH, zero_record)
    uc.mem_write(OUTPUT_FLASH, output_record)
    uc.mem_write(TABLE_FLASH, table_record)
    uc.mem_write(A(0x1ffff000), runtime)
    uc.mem_write(CORRECTION, correction)
    uc.mem_write(A(0x1fffa674), sine)
    uc.mem_write(0x40053800, gpio)
    controller = 0x20008000
    uc.mem_write(controller, bytes([0xa5]) * 0x200)
    uc.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
    uc.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    uc.reg_write(arm.UC_ARM_REG_FPSCR, fpscr)
    uc.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    uc.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    uc.reg_write(arm.UC_ARM_REG_R0, controller)
    events = []
    invalid_accesses = []
    printf = FACTORY_PRINTF if original else SOURCE_PRINTF

    def memory(emu, access, address, size, value, _):
        if access == UC_MEM_READ:
            value = int.from_bytes(emu.mem_read(address, size), 'little')
            events.append(('read', address, size, value))
        else:
            events.append(('write', address, size, value))

    def code(emu, address, size, _):
        if address == printf:
            fmt = c_string(emu, emu.reg_read(arm.UC_ARM_REG_R0))
            if b'%' in fmt:
                args = (emu.reg_read(arm.UC_ARM_REG_R2),
                        emu.reg_read(arm.UC_ARM_REG_R3))
            else:
                args = ()
            events.append(('printf', fmt, args))
            emu.reg_write(arm.UC_ARM_REG_PC,
                          emu.reg_read(arm.UC_ARM_REG_LR))

    def invalid(emu, access, address, size, value, _):
        pc = emu.reg_read(arm.UC_ARM_REG_PC)
        invalid_accesses.append((pc, access, address, size, value))
        return False

    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=A(0x1ffff000), end=A(0x1ffff2ff))
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=A(0x1fffa5c0), end=A(0x1fffa5c7))
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=A(0x1fffa674), end=A(0x1fffc677))
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=CORRECTION, end=CORRECTION + 0x3ff)
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=0x40053800, end=0x400538ff)
    uc.hook_add(UC_HOOK_MEM_INVALID, invalid)
    uc.hook_add(UC_HOOK_CODE, code)
    entry = FACTORY_ENTRY if original else SOURCE_ENTRY
    try:
        uc.emu_start(entry | 1, 0x30000, count=1000000)
    except Exception as error:
        raise RuntimeError(
            f'emulation failed original={original} invalid={invalid_accesses}') from error
    assert uc.reg_read(arm.UC_ARM_REG_PC) == 0x30000
    return (
        events,
        bytes(uc.mem_read(A(0x1ffff000), len(runtime))),
        bytes(uc.mem_read(A(0x1fffa5c0), 8)),
        bytes(uc.mem_read(CORRECTION, 0x400)),
        bytes(uc.mem_read(TABLE, 0x2000)),
        bytes(uc.mem_read(0x40053800, len(gpio))),
        uc.reg_read(arm.UC_ARM_REG_FPSCR),
    )


rng = random.Random(0x22658)
nan_words = [0x7fc00001, 0xffc12345, 0x7f800001, 0xff800001]
for case in range(12):
    motor_words = [rng.getrandbits(32) for _ in range(259)]
    if case < 4:
        motor_words[case * 17] = nan_words[case]
    motor_words[256] = [0x3f000000, 0x7fc00001, 0x7f800000,
                        0xff800000][case % 4]
    motor_words[258] = [0x3f800000, 0x7fc00001, 0x40000000,
                        0xbf800000][case % 4]
    motor_record = b''.join(word.to_bytes(4, 'little')
                            for word in motor_words)
    zero_words = [rng.getrandbits(32), rng.getrandbits(32)]
    if case % 3 == 1:
        zero_words[0] = 0x7fc00001
    if case % 3 == 2:
        zero_words[1] = 0xffc00001
    zero_record = b''.join(word.to_bytes(4, 'little') for word in zero_words)
    output_words = [rng.getrandbits(32) for _ in range(4)]
    # The factory SRAM sin/cos helper accepts its persisted phase in the
    # calibrated -pi..pi domain; arbitrary float bit patterns can index beyond
    # the 2049-entry table and are not valid flash records.
    output_words[3] = [
        0x00000000, 0x3f000000, 0xbf000000, 0x3f800000,
        0xbf800000, 0x3fc00000, 0xbfc00000, 0x40000000,
        0xc0000000, 0x40400000, 0xc0400000, 0x3e800000,
    ][case]
    output_record = b''.join(word.to_bytes(4, 'little')
                             for word in output_words)

    values = [index & 0xfff for index in range(4096)]
    route = case % 3
    if route == 1:
        values[1 + (case * 97) % 4094] = 0xffff
    elif route == 2:
        values[1] = 100
    table_record = b''.join(value.to_bytes(2, 'little') for value in values)

    runtime = bytes(rng.getrandbits(8) for _ in range(0x300))
    correction = bytes(rng.getrandbits(8) for _ in range(0x400))
    sine = bytes(rng.getrandbits(8) for _ in range(0x2004))
    gpio = bytes(rng.getrandbits(8) for _ in range(0x100))
    fpscr = (case & 3) << 22
    factory_result = run(True, motor_record, zero_record, output_record,
                         table_record, runtime, correction, sine, gpio, fpscr)
    source_result = run(False, motor_record, zero_record, output_record,
                        table_record, runtime, correction, sine, gpio, fpscr)
    assert factory_result == source_result, (case, route, factory_result,
                                              source_result)

print('PASS: 12 complete factory 0x22658/source calibration loader cases; '
      'correction/records/table, all validation exits, ordered fixed SRAM/'
      'GPIO, diagnostics and FPSCR')
