from pathlib import Path
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_WRITE
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
for case in range(256):
    mode=case%8
    pll=rng.getrandbits(32)
    hrc=rng.getrandbits(32)
    cpacr=rng.getrandbits(32)
    results=[]
    for original in (True,False):
        u=machine(original)
        for page in (0x40010000,0x40054000,0xe000e000): u.mem_map(page,0x1000)
        for address,value in [(0x40010684,hrc),(0x40054026,mode),(0x40054100,pll),
                              (0xe000ed88,cpacr),(A(0x1ffff4f0),0xdeadbeef),
                              (A(0x1ffff4f4),0xcafebabe)]:
            u.mem_write(address,value.to_bytes(4,'little'))
        u.reg_write(arm.UC_ARM_REG_SP,0x200004f8)
        u.reg_write(arm.UC_ARM_REG_LR,0x30001)
        u.reg_write(arm.UC_ARM_REG_PRIMASK,case%2)
        trace=[]
        def memory(uc,access,address,size,value,data):
            trace.append((access,address,size,value if access==UC_MEM_WRITE else int.from_bytes(uc.mem_read(address,size),'little')))
        for lo,hi in [(0x40010684,0x40010687),(0x40054000,0x40054fff),
                      (0xe000ed00,0xe000edff),(A(0x1ffff4f0),A(0x1ffff4f7))]:
            u.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,memory,begin=lo,end=hi)
        entry=F(0x23554) if original else symbols['SystemInit']
        u.emu_start(entry|1,0x30000,count=1000)
        assert u.reg_read(arm.UC_ARM_REG_PC)==0x30000
        results.append((trace,bytes(u.mem_read(A(0x1ffff4f0),8)),bytes(u.mem_read(0xe000ed88,4)),
                        bytes(u.mem_read(0xe000ed08,4)),u.reg_read(arm.UC_ARM_REG_PRIMASK)))
    assert results[0]==results[1],(case,results)
print('PASS: 256 real SystemInit cases; all clock sources, randomized PLL/HRC/CPACR, ordered MMIO/SRAM and preserved PRIMASK')
