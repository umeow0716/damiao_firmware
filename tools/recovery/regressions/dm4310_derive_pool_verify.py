from pathlib import Path
import struct
import sys
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_WRITE
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
nan_matrix = '--nan-matrix' in sys.argv
for case in range(1024 if nan_matrix else 768):
    state=bytearray([0xa5]*0x800)
    inputs=[(0x68,24.0 if case%2 else 10.0),(0x100,12.0),(0x104,0.0 if case%3 else 2.0),
            (0x144,0.8),(0x148,0.001),(0x14c,0.01),(0x150,10.0),(0x160,1000.0),
            (0x178,1.2),(0x208,0.25),(0x20c,0.00005)]
    for offset,value in inputs:
        bits=int.from_bytes(struct.pack('<f',value),'little') if case<256 or nan_matrix else rng.getrandbits(32)
        struct.pack_into('<I',state,offset,bits)
    struct.pack_into('<I',state,0x140,case%32)
    if nan_matrix:
        special=[0,0x7f800000,0xff800000,0x7fc12345,0x7fc54321,0x7f812345,0xff854321,0x007fffff]
        struct.pack_into('<f',state,0x68,24.0)
        struct.pack_into('<I',state,0x160,special[(case//16)%8])
        struct.pack_into('<I',state,0x148,special[(case//128)%8])
    pointers={A(0x1fffa11c):0x20008000,A(0x1fffa13c):0x20008100,A(0x1fffa140):0x20008200,
              A(0x1fffa120):0x20008300,A(0x1fffa124):0x20008400,A(0x1fffa130):0x20008500,
              A(0x1fffa134):0x20008600,A(0x1fffa158):0x20008700}
    if case%5==0: pointers[A(0x1fffa134)]=pointers[A(0x1fffa130)]
    literals={address:rng.getrandbits(32) for address in [A(0x1fffa118),A(0x1fffa144),A(0x1fffa148),A(0x1fffa14c),A(0x1fffa150),A(0x1fffa154)]} if case>=512 and not nan_matrix else {}
    results=[]
    for original in (True,False):
        u=machine(original)
        if original: u.mem_write(FIXED_IMAGE_BASE, factory[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE])
        else:
            for section in elf.iter_sections():
                if FIXED_IMAGE_BASE<=section['sh_addr']<FACTORY_STATE_BASE and section['sh_type']!='SHT_NOBITS':
                    u.mem_write(section['sh_addr'],section.data())
        u.mem_write(0x20008000,bytes(state))
        for address,value in (pointers|literals).items(): u.mem_write(address,value.to_bytes(4,'little'))
        u.reg_write(arm.UC_ARM_REG_C1_C0_2,0xf00000)
        u.reg_write(arm.UC_ARM_REG_FPEXC,0x40000000)
        u.reg_write(arm.UC_ARM_REG_FPSCR,(case%16)<<22)
        u.reg_write(arm.UC_ARM_REG_SP,0x2000f000)
        u.reg_write(arm.UC_ARM_REG_LR,0x30001)
        trace=[]
        def memory(uc,access,address,size,value,data):
            trace.append((access,address,size,value if access==UC_MEM_WRITE else int.from_bytes(uc.mem_read(address,size),'little')))
        for lo,hi in [(A(0x1fffa118),A(0x1fffa15b)),(0x20008000,0x200087ff)]:
            u.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,memory,begin=lo,end=hi)
        entry=A(0x1fff9ea6) if original else symbols['dm4310_derive_runtime_controller_states_helper']
        u.emu_start(entry|1,0x30000,count=10000)
        assert u.reg_read(arm.UC_ARM_REG_PC)==0x30000
        results.append((trace,bytes(u.mem_read(0x20008000,0x800)),u.reg_read(arm.UC_ARM_REG_FPSCR)))
    assert results[0]==results[1],(case,results[0][0],results[1][0],results[0][2],results[1][2])
print('PASS:',1024 if nan_matrix else 768,'controller-derivation cases;', 'NaN operand-pair matrix' if nan_matrix else 'normal/arbitrary-bit inputs and pool constants', 'relocated/aliased state, ordered SRAM accesses and FPSCR')
