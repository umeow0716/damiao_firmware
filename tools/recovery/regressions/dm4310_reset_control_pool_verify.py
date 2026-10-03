from pathlib import Path
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_WRITE
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
for case in range(256):
    value=rng.getrandbits(32)
    pointers=[0x20008000+(((case+i)%8)*0x80) for i in range(5)]
    if case%3==0: pointers[2]=pointers[1]
    results=[]
    for original in (True,False):
        u=machine(original)
        if original: u.mem_write(FIXED_IMAGE_BASE, factory[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE])
        else:
            for section in elf.iter_sections():
                if FIXED_IMAGE_BASE<=section['sh_addr']<FACTORY_STATE_BASE and section['sh_type']!='SHT_NOBITS':
                    u.mem_write(section['sh_addr'],section.data())
        u.mem_write(0x20008000,bytes([0xa5])*0x400)
        for address,bits in zip([A(0x1fffa118),A(0x1fffa11c),A(0x1fffa130),A(0x1fffa134),A(0x1fffa128),A(0x1fffa12c)],[value]+pointers):
            u.mem_write(address,bits.to_bytes(4,'little'))
        u.reg_write(arm.UC_ARM_REG_C1_C0_2,0xf00000)
        u.reg_write(arm.UC_ARM_REG_FPEXC,0x40000000)
        u.reg_write(arm.UC_ARM_REG_FPSCR,(case%16)<<22)
        u.reg_write(arm.UC_ARM_REG_SP,0x2000f000)
        u.reg_write(arm.UC_ARM_REG_LR,0x30001)
        trace=[]
        def memory(uc,access,address,size,bits,data):
            trace.append((access,address,size,bits if access==UC_MEM_WRITE else int.from_bytes(uc.mem_read(address,size),'little')))
        for lo,hi in [(A(0x1fffa118),A(0x1fffa137)),(0x20008000,0x200083ff)]:
            u.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,memory,begin=lo,end=hi)
        entry=A(0x1fff9dc8) if original else symbols['reset_control_state_helper']
        u.emu_start(entry|1,0x30000,count=10000)
        assert u.reg_read(arm.UC_ARM_REG_PC)==0x30000
        results.append((trace,bytes(u.mem_read(0x20008000,0x400)),u.reg_read(arm.UC_ARM_REG_FPSCR)))
    assert results[0]==results[1],(case,results)
print('PASS: 256 current-observer reset cases; ordered pool/target accesses, arbitrary fill bits, relocated/aliased state pointers and FPSCR')
