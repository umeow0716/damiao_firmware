from pathlib import Path
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_WRITE
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
for case in range(256):
    sector=0x1e000
    words=5
    payload=b''.join(rng.getrandbits(32).to_bytes(4,'little') for _ in range(words))
    frmc=rng.getrandbits(32); fwmc=rng.getrandbits(32); key=rng.getrandbits(32)
    results=[]
    for original in (True,False):
        u=machine(original)
        if original: u.mem_write(FIXED_IMAGE_BASE, factory[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE])
        else:
            for section in elf.iter_sections():
                if FIXED_IMAGE_BASE<=section['sh_addr']<FACTORY_STATE_BASE and section['sh_type']!='SHT_NOBITS':
                    u.mem_write(section['sh_addr'],section.data())
        u.mem_map(0x40010000,0x1000)
        u.mem_map(0x1e000,0x1000)
        u.mem_write(A(0x1fffc824),payload)
        u.mem_write(0x40010418,frmc.to_bytes(4,'little'))
        u.mem_write(0x4001041c,fwmc.to_bytes(4,'little'))
        u.mem_write(A(0x1fff8728),key.to_bytes(4,'little'))
        u.reg_write(arm.UC_ARM_REG_R0,sector)
        u.reg_write(arm.UC_ARM_REG_R1,A(0x1fffc824))
        u.reg_write(arm.UC_ARM_REG_R2,words)
        u.reg_write(arm.UC_ARM_REG_SP,0x2000f000)
        u.reg_write(arm.UC_ARM_REG_LR,0x30001)
        u.reg_write(arm.UC_ARM_REG_PRIMASK,case%2)
        trace=[]; polls=[0]
        def memory(uc,access,address,size,value,data):
            if access != UC_MEM_WRITE and address==0x40010420:
                polls[0]+=1
                uc.mem_write(address,(0 if polls[0]<=case%3 else 0x110).to_bytes(4,'little'))
            trace.append((access,address,size,value if access==UC_MEM_WRITE else int.from_bytes(uc.mem_read(address,size),'little')))
        u.hook_add(UC_HOOK_MEM_READ,memory,begin=A(0x1fff8724),end=A(0x1fff8743))
        u.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,memory,begin=0x40010000,end=0x40010fff)
        u.hook_add(UC_HOOK_MEM_WRITE,memory,begin=sector,end=sector+max(4,words*4)-1)
        u.hook_add(UC_HOOK_MEM_READ,memory,begin=A(0x1fffc824),end=A(0x1fffc837))
        entry=A(0x1fff8640) if original else symbols['write_boot_record_from_sram']
        u.emu_start(entry|1,0x30000,count=10000)
        assert u.reg_read(arm.UC_ARM_REG_PC)==0x30000
        results.append((trace,bytes(u.mem_read(0x40010400,0x28)),bytes(u.mem_read(sector,max(4,words*4))),u.reg_read(arm.UC_ARM_REG_PRIMASK)))
    assert results[0]==results[1],(case,results)
print('PASS: 256 boot-record writer cases; shared pool reads/MMIO/flash-write order, random keys/registers, delayed ready polls and PRIMASK')
