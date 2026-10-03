from pathlib import Path
import random

from unicorn import (UC_HOOK_CODE, UC_HOOK_MEM_INVALID, UC_HOOK_MEM_READ,
                     UC_HOOK_MEM_WRITE, UC_MEM_READ)

exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split(
    '\nfor old, name, size in mapping[:1]:')[0])

FACTORY_ENTRY = F(0x26d00)
SOURCE_ENTRY = symbols['firmware_control_service']
STAGING = A(0x1fffa5c8)
CACHE = A(0x1ffff23c)
SPEED = A(0x1fffa568)
POSITION = A(0x1fffa590)
AIRCR = 0xe000ed0c

factory_calls = {
    'alloc': F(0x203c8),
    'copy': F(0x20734),
    'flash': F(0x219a6),
    'uart': F(0x2379c),
    'delay': F(0x21f60),
    'derive': F(0x25138),
    'free': F(0x20426),
}
source_calls = {
    'alloc': symbols['runtime_alloc'],
    'copy': symbols['runtime_copy_bytes'],
    'flash': symbols['board_flash_replace_sector_prefix'],
    'uart': symbols['platform_debug_write'],
    'delay': symbols['board_delay_us'],
    # firmware_control_service uses the first same-named GNU linker veneer;
    # derive it from the current ELF because its address follows .text size.
    'derive': min(
        symbol['st_value'] & ~1
        for symbol in elf.get_section_by_name('.symtab').iter_symbols()
        if symbol.name == '__derive_control_parameters_helper_veneer'
    ),
    'free': symbols['runtime_free'],
}
factory_barriers = {F(0x26d62), F(0x26d74)}
source_barriers = {
    symbols['platform_system_reset'] + 0x0a,
    symbols['reset_after_barrier'] + 0x0e,
}
factory_loop = F(0x26d78)
source_loop = symbols['reset_after_barrier'] + 0x12
tracked_ranges = ((STAGING, 0x94), (CACHE, 0x80),
                  (SPEED, 0x28), (POSITION, 0x28))


def run(original, request, response, initial, aircr):
    uc = machine(original)
    uc.mem_map(0xe0000000, 0x100000)
    uc.mem_write(AIRCR, aircr.to_bytes(4, 'little'))
    for address, data in initial.items():
        uc.mem_write(address, data)
    uc.mem_write(response, bytes([0xa5]) * 0x82)
    uc.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
    uc.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    uc.reg_write(arm.UC_ARM_REG_FPSCR, (request & 15) << 22)
    uc.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    uc.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    uc.reg_write(arm.UC_ARM_REG_R0, request)
    events = []
    invalid = []
    stopped = False
    calls = factory_calls if original else source_calls

    def memory(emu, access, address, size, value, _):
        if access == UC_MEM_READ:
            value = int.from_bytes(emu.mem_read(address, size), 'little')
            events.append(('read', address, size, value))
        else:
            events.append(('write', address, size, value))

    def invalid_memory(emu, access, address, size, value, _):
        invalid.append((access, address, size, value,
                        emu.reg_read(arm.UC_ARM_REG_PC)))
        return False

    def return_from_stub(emu):
        emu.reg_write(arm.UC_ARM_REG_PC, emu.reg_read(arm.UC_ARM_REG_LR))

    def code(emu, address, size, _):
        nonlocal stopped
        if address == calls['alloc']:
            events.append(('alloc', emu.reg_read(arm.UC_ARM_REG_R0)))
            emu.reg_write(arm.UC_ARM_REG_R0, response)
            return_from_stub(emu)
        elif address == calls['copy']:
            destination = emu.reg_read(arm.UC_ARM_REG_R0)
            source = emu.reg_read(arm.UC_ARM_REG_R1)
            length = emu.reg_read(arm.UC_ARM_REG_R2)
            events.append(('copy', destination - response, source, length))
            emu.mem_write(destination, bytes(emu.mem_read(source, length)))
            return_from_stub(emu)
        elif address == calls['flash']:
            destination = emu.reg_read(arm.UC_ARM_REG_R0)
            source = emu.reg_read(arm.UC_ARM_REG_R1)
            length = emu.reg_read(arm.UC_ARM_REG_R2)
            if original:
                length *= 4
            events.append(('flash', destination, source, length,
                           (emu.reg_read(arm.UC_ARM_REG_CPSR) >> 7) & 1))
            return_from_stub(emu)
        elif address == calls['uart']:
            pointer = emu.reg_read(arm.UC_ARM_REG_R0)
            length = emu.reg_read(arm.UC_ARM_REG_R1)
            events.append(('uart', pointer - response, length,
                           bytes(emu.mem_read(pointer, length)),
                           (emu.reg_read(arm.UC_ARM_REG_CPSR) >> 7) & 1))
            return_from_stub(emu)
        elif address == calls['delay']:
            events.append(('delay', emu.reg_read(arm.UC_ARM_REG_R0),
                           (emu.reg_read(arm.UC_ARM_REG_CPSR) >> 7) & 1))
            return_from_stub(emu)
        elif address == calls['derive']:
            events.append(('derive',))
            return_from_stub(emu)
        elif address == calls['free']:
            events.append(('free', emu.reg_read(arm.UC_ARM_REG_R0) - response))
            stopped = True
            emu.emu_stop()
        elif address in (factory_barriers if original else source_barriers):
            events.append(('barrier',))
        elif address == (factory_loop if original else source_loop):
            events.append(('reset_loop',))
            stopped = True
            emu.emu_stop()

    for address, length in tracked_ranges:
        uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                    begin=address, end=address + length - 1)
    uc.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory,
                begin=AIRCR, end=AIRCR + 3)
    uc.hook_add(UC_HOOK_MEM_INVALID, invalid_memory)
    uc.hook_add(UC_HOOK_CODE, code)
    entry = FACTORY_ENTRY if original else SOURCE_ENTRY
    try:
        uc.emu_start(entry | 1, 0x30000, count=10000)
    except Exception as error:
        raise AssertionError((original, request,
                              hex(uc.reg_read(arm.UC_ARM_REG_PC)), invalid,
                              events)) from error
    assert stopped, (original, request, hex(uc.reg_read(arm.UC_ARM_REG_PC)),
                     events)
    return (
        events,
        bytes(uc.mem_read(response, 0x82)),
        tuple(bytes(uc.mem_read(address, length))
              for address, length in tracked_ranges),
        bytes(uc.mem_read(AIRCR, 4)),
        uc.reg_read(arm.UC_ARM_REG_FPSCR),
        (uc.reg_read(arm.UC_ARM_REG_CPSR) >> 7) & 1,
    )


rng = random.Random(0x26d00)
requests = [0, 1, 2, 3, 4, 5, 6, 7, 0x7f, 0x80, 0xfe, 0xff]
response_offsets = [0, 4, 0x3c]
count = 0
for pattern in range(32):
    initial = {
        address: bytes(rng.getrandbits(8) for _ in range(length))
        for address, length in tracked_ranges
    }
    aircr = rng.getrandbits(32)
    for request in requests:
        for offset in response_offsets:
            response = 0x20008000 + offset
            factory_result = run(True, request, response, initial, aircr)
            source_result = run(False, request, response, initial, aircr)
            assert factory_result == source_result, (
                pattern, request, offset, factory_result, source_result)
            count += 1

print(f'PASS: {count} factory 0x26d00/source worker cases; request control, '
      'payloads, fixed-SRAM trace/state, child ABI, reset sequence and FPSCR')
