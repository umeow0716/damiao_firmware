from pathlib import Path
import struct
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_WRITE
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
# The source slice lives inside prepare_current_controllers.isra.0.  Anchor it
# to the local symbol so ordinary link-layout changes cannot stale the test.
SOURCE_SLICE_START = symbols['prepare_current_controllers.isra.0'] + 0x8a
SOURCE_SLICE_END = symbols['prepare_current_controllers.isra.0'] + 0xf4
for case in range(256):
    observer = A(0x1ffff380) if case < 128 else 0x20002000
    state = struct.pack('<48f', *(rng.uniform(-10,10) for _ in range(48)))
    obs = struct.pack('<15f', *(rng.uniform(-2,2) for _ in range(15)))
    speed = struct.pack('<10f', *(rng.uniform(-2,2) for _ in range(10)))
    results = []
    for original in (True, False):
        u = machine(original)
        if original:
            u.mem_write(FIXED_IMAGE_BASE, factory[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE])
        else:
            for section in elf.iter_sections():
                if FIXED_IMAGE_BASE <= section['sh_addr'] < FACTORY_STATE_BASE and section['sh_type'] != 'SHT_NOBITS':
                    u.mem_write(section['sh_addr'], section.data())
        u.mem_write(A(0x1ffff088), state)
        u.mem_write(A(0x1ffff104), state)
        u.mem_write(observer, obs)
        u.mem_write(A(0x1fffa568), speed)
        u.mem_write(A(0x1fff8630), struct.pack('<I', observer))
        u.mem_write(0x2000e014, struct.pack('<I', A(0x1fffa568)))
        u.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
        u.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
        u.reg_write(arm.UC_ARM_REG_FPSCR, (case % 4) << 22)
        values = {arm.UC_ARM_REG_SP:0x2000f000}
        if original:
            values.update({arm.UC_ARM_REG_R4:A(0x1ffff104), arm.UC_ARM_REG_R5:A(0x1ffff088), arm.UC_ARM_REG_R8:A(0x1fffa568)})
        else:
            values.update({arm.UC_ARM_REG_R3:A(0x1fff8000), arm.UC_ARM_REG_R4:A(0x1ffff088), arm.UC_ARM_REG_R5:0x2000e000, arm.UC_ARM_REG_R6:A(0x1ffff104)})
        for reg, value in values.items(): u.reg_write(reg, value)
        trace = []
        def memory(uc, access, address, size, value, data):
            trace.append((access,address,size,value if access == UC_MEM_WRITE else int.from_bytes(uc.mem_read(address,size),'little')))
        for lo, hi in [(A(0x1fff8630),A(0x1fff8633)),(A(0x1ffff088),A(0x1ffff1a7)),(A(0x1fffa568),A(0x1fffa58f)),(observer,observer+59),(symbols['g_app'],symbols['g_app']+0x2ff)]:
            u.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,memory,begin=lo,end=hi)
        start = A(0x1fff8492) if original else SOURCE_SLICE_START
        end = A(0x1fff84ee) if original else SOURCE_SLICE_END
        u.emu_start(start | 1, end, count=10000)
        assert u.reg_read(arm.UC_ARM_REG_PC) == end
        results.append((trace, bytes(u.mem_read(observer,60)),bytes(u.mem_read(A(0x1fffa568),40)),u.reg_read(arm.UC_ARM_REG_FPSCR)))
    assert results[0] == results[1], (case, results)
print('PASS: 256 filter/observer/speed-loop slices, relocated observer, ordered factory-state/pool/g_app trace and FPSCR; raw ABI excluded')
