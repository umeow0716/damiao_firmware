from pathlib import Path
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_WRITE
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
edges=[0,0x80000000,1,0x80000001,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc00001,0x7f800001]
for case in range(1024):
    alpha,beta=(edges[(case//4)%10],edges[(case//40)%10]) if case<400 else (rng.getrandbits(32),rng.getrandbits(32))
    results=[]
    for original in (True,False):
        u=machine(original)
        if original: u.mem_write(FIXED_IMAGE_BASE, factory[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE])
        else:
            for section in elf.iter_sections():
                if FIXED_IMAGE_BASE<=section['sh_addr']<FACTORY_STATE_BASE and section['sh_type']!='SHT_NOBITS':
                    u.mem_write(section['sh_addr'],section.data())
        u.mem_map(0x40038000,0x1000)
        if case % 3 == 0: u.mem_write(A(0x1fff9d50),bytes.fromhex('00409c44'))  # 1250
        if case % 5 == 0: u.mem_write(A(0x1fff9d54),(0x40038100).to_bytes(4,'little'))
        if case % 7 == 0: u.mem_write(A(0x1fff9d4c),(0x3f000000).to_bytes(4,'little'))
        u.reg_write(arm.UC_ARM_REG_C1_C0_2,0xf00000)
        u.reg_write(arm.UC_ARM_REG_FPEXC,0x40000000)
        u.reg_write(arm.UC_ARM_REG_FPSCR,((case%4)<<22)|(((case//4)%2)<<24)|(((case//8)%2)<<25))
        u.reg_write(arm.UC_ARM_REG_S0,alpha)
        u.reg_write(arm.UC_ARM_REG_S1,beta)
        u.reg_write(arm.UC_ARM_REG_SP,0x2000f000)
        u.reg_write(arm.UC_ARM_REG_LR,0x30001)
        trace=[]
        def memory(uc,access,address,size,value,data):
            trace.append((access,address,size,value if access==UC_MEM_WRITE else int.from_bytes(uc.mem_read(address,size),'little')))
            if access == UC_MEM_WRITE and address in (0x40038014,0x40038114):
                uc.mem_write(A(0x1fff9d50),(0x3f800000).to_bytes(4,'little'))
                uc.mem_write(A(0x1fff9d54),(0x40038200).to_bytes(4,'little'))
        u.hook_add(UC_HOOK_MEM_READ,memory,begin=A(0x1fff9d4c),end=A(0x1fff9d5b))
        u.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,memory,begin=0x40038000,end=0x40038fff)
        entry=A(0x1fff9c40) if original else symbols['dm4310_svpwm_helper']
        u.emu_start(entry|1,0x30000,count=10000)
        assert u.reg_read(arm.UC_ARM_REG_PC)==0x30000
        results.append((trace,u.reg_read(arm.UC_ARM_REG_FPSCR),bytes(u.mem_read(0x40038000,0x20))))
    assert results[0]==results[1],(case,hex(alpha),hex(beta),results)
print('PASS: 1024 SVPWM cases; rounding/DN/FZ modes, special/random inputs, changed pool coefficients/scale/base and post-W mutations; ordered pool reads and PWM MMIO/FPSCR')
