from pathlib import Path
import csv
import sys
from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_WRITE
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
images = []
machines = []
mmio_traces = []
real_system = '--real-system' in sys.argv
clock_source = int(next((arg.split('=', 1)[1] for arg in sys.argv if arg.startswith('--clock-source=')), '0'))
interrupt_mask = 1 if '--masked' in sys.argv else 0
for original in (True, False):
    u = machine(original)
    if original: u.mem_write(0x20000, factory)
    u.mem_write(A(0x1fff0000), bytes([0xa5]) * 0x10000)
    u.mem_write(0x20000000, bytes([0xa5]) * 0x10000)
    u.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
    u.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
    u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
    u.reg_write(arm.UC_ARM_REG_PRIMASK, interrupt_mask)
    u.mem_map(0x40050000, 0x1000)
    trace = []
    def mmio(uc, access, address, size, value, data):
        trace.append((access, address, size, value if access == UC_MEM_WRITE else int.from_bytes(uc.mem_read(address,size),'little')))
    if real_system:
        for lo,hi in [(0x40010000,0x40010fff),(0x40050000,0x40054fff),(0xe000ed00,0xe000edff)]:
            u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE,mmio,begin=lo,end=hi)
    if real_system:
        for page in (0x40010000, 0x40054000, 0xe000e000):
            u.mem_map(page, 0x1000)
        u.mem_write(0x40010684, (1 if '--hrc16' in sys.argv else 0).to_bytes(4, 'little'))
        u.mem_write(0x40054026, clock_source.to_bytes(1, 'little'))
        u.mem_write(0x40054100, (0x39306300 | (0x80 if '--internal-pll' in sys.argv else 0)).to_bytes(4, 'little'))
    if original:
        u.emu_start(0x20389 if real_system else 0x20259, 0x20368, count=2000000)
        assert u.reg_read(arm.UC_ARM_REG_PC) == 0x20368
    else:
        system_init = symbols['SystemInit']
        def skip_system(uc, address, size, data):
            if address == system_init:
                uc.reg_write(arm.UC_ARM_REG_PC, uc.reg_read(arm.UC_ARM_REG_LR))
        if not real_system: u.hook_add(UC_HOOK_CODE, skip_system)
        u.emu_start(symbols['Reset_Handler'] | 1,
                    symbols['dm4310_runtime_main_entry'], count=2000000)
        assert u.reg_read(arm.UC_ARM_REG_PC) == symbols['dm4310_runtime_main_entry']
        sections = [s for s in elf.iter_sections()
                    if FIXED_IMAGE_BASE <= s['sh_addr'] < FACTORY_STATE_BASE
                    and s['sh_type'] != 'SHT_NOBITS' and s['sh_size']]
        rows = list(csv.DictReader((root / 'recovered/dm4310/tables/sram_function_inventory.tsv').open(),
                                   delimiter='\t'))
        for row in rows:
            entry = A(int(row['entry'], 16))
            owners = [s for s in sections if s['sh_addr'] <= entry < s['sh_addr'] + s['sh_size']]
            assert len(owners) == 1, (hex(entry), [s.name for s in owners])
            section = owners[0]
            actual = bytes(u.mem_read(section['sh_addr'], section['sh_size']))
            expected = section.data()
            if actual != expected:
                differences = [(hex(section['sh_addr'] + index), left, right)
                               for index, (left, right) in
                               enumerate(zip(actual, expected)) if left != right]
                raise AssertionError((section.name, differences[:32]))
        print('PASS: all 33 inventoried fixed SRAM entries loaded by actual source Reset; section bytes match ELF')
    images.append(bytes(u.mem_read(FACTORY_STATE_BASE, FACTORY_STATE_SIZE)))
    machines.append(u)
    mmio_traces.append(trace)
assert images[0] == images[1], [(hex(FACTORY_STATE_BASE + i), a, b)
                               for i, (a, b) in enumerate(zip(*images)) if a != b][:16]
print(f'PASS: complete {FACTORY_STATE_SIZE:#x}-byte initialized scatter range '
      'matches factory after actual startup paths')
for address, size in [(A(0x1fff8630), 16), (A(0x1fff8724), 32),
                      (A(0x1fff9830), 76), (A(0x1fff9934), 28),
                      (A(0x1fff9ac0), 12), (A(0x1fff9c34), 12),
                      (A(0x1fff9d4c), 16), (A(0x1fffa118), 72),
                      (A(0x1fffa300), 12), (A(0x1fffa4bc), 32)]:
    offset = address - FIXED_IMAGE_BASE + FACTORY_FIXED_SOURCE
    assert bytes(machines[1].mem_read(address, size)) == factory[offset:offset + size], hex(address)
print('PASS: all 308 shared SRAM literal bytes match after actual source Reset')
for address in (A(0x1fff982e), A(0x1fff9932), A(0x1fff9c32),
                A(0x1fffa2fe), A(0x1fffa4ba), A(0x1fffa50e)):
    assert bytes(machines[1].mem_read(address, 2)) == bytes(2), hex(address)
print('PASS: all twelve inter-body/tail alignment bytes match factory')
runtime = []
entry_stacks = []
heaps = []
for original, u in zip((True, False), machines):
    entry = 0x20368 if original else symbols['dm4310_runtime_main_entry']
    stop = F(0x252f4) if original else symbols['main']
    u.emu_start(entry | 1, stop, count=100000)
    assert u.reg_read(arm.UC_ARM_REG_PC) == stop
    runtime.append(bytes(u.mem_read(A(0x1ffff490), 0x60)))
    entry_stacks.append(u.reg_read(arm.UC_ARM_REG_SP))
    heaps.append(bytes(u.mem_read(A(0x1ffff4f8), 0x400)))
    assert u.reg_read(arm.UC_ARM_REG_PRIMASK) == interrupt_mask
assert runtime[0] == runtime[1]
print('PASS: full 0x60 runtime-context bytes match before main, including actual saved return PC 0x2036d')
assert heaps[0] == heaps[1]
print('PASS: complete fixed 0x400-byte heap matches before main')
assert entry_stacks == [FACTORY_STACK_TOP, FACTORY_STACK_TOP]
print('Main-entry SP (factory/source):', [hex(value) for value in entry_stacks])
if real_system:
    assert mmio_traces[0] == mmio_traces[1], mmio_traces
    print('PASS: ordered Reset-to-main MMIO and preserved PRIMASK', interrupt_mask)
    assert bytes(machines[0].mem_read(A(0x1ffff4f0), 8)) == bytes(machines[1].mem_read(A(0x1ffff4f0), 8))
    print('PASS: real Reset/SystemInit/scatter/runtime chain; fixed clock SRAM words match')
print('Scope: real SystemInit in emulated MMIO' if real_system else 'Scope: SystemInit intercepted', '; not physical hardware boot proof')
