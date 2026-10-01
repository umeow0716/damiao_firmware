from pathlib import Path
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_WRITE
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
entries=[(A(0x1fff9acc),'dm4310_mcan_send_classic_helper'),
         (A(0x1fff9b0e),'dm4310_mcan_send_variable_fd_helper'),
         (A(0x1fff9ba0),'dm4310_mcan_send_fd_helper')]
for old,name in entries:
    for case in range(256):
        length=case
        wraps = old != A(0x1fff9acc) and length >= 253
        identifier=rng.getrandbits(16); dlc=rng.getrandbits(32)
        index=case%32; source=0x20008000+case%4
        controller=0x40029000+(0x100 if case%2 else 0)
        headers=0x4002b000+(0x1000 if case%3 else 0)
        payload_base=0x4002b330+(0x2000 if case%5 else 0)
        payload=bytes(rng.getrandbits(8) for _ in range(256))
        results=[]
        for original in (True,False):
            u=machine(original)
            if original: u.mem_write(FIXED_IMAGE_BASE, factory[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE])
            else:
                for section in elf.iter_sections():
                    if FIXED_IMAGE_BASE<=section['sh_addr']<FACTORY_STATE_BASE and section['sh_type']!='SHT_NOBITS':
                        u.mem_write(section['sh_addr'],section.data())
            u.mem_map(0x40029000,0x6000)
            for address,value in [(A(0x1fff9c34),controller),(A(0x1fff9c38),headers),
                                  (A(0x1fff9c3c),payload_base),(controller+0xc4,index<<16)]:
                u.mem_write(address,value.to_bytes(4,'little'))
            u.mem_write(source,payload)
            for reg,value in [(arm.UC_ARM_REG_R0,source),(arm.UC_ARM_REG_R1,identifier),
                              (arm.UC_ARM_REG_R2,length),(arm.UC_ARM_REG_R3,dlc),
                              (arm.UC_ARM_REG_SP,0x2000f000),(arm.UC_ARM_REG_LR,0x30001)]:
                u.reg_write(reg,value)
            trace=[]; payload_writes=[0]
            def memory(uc,access,address,size,value,data):
                trace.append((access,address,size,value if access==UC_MEM_WRITE else int.from_bytes(uc.mem_read(address,size),'little')))
                if wraps and access == UC_MEM_WRITE and address == payload_base + index*72 + payload_writes[0]*4:
                    payload_writes[0] += 1
                    if payload_writes[0] == 128: uc.emu_stop()
            for lo,hi in [(A(0x1fff9c34),A(0x1fff9c3f)),(0x40029000,0x4002efff),
                          (source,source+255)]:
                u.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,memory,begin=lo,end=hi)
            u.emu_start((old if original else symbols[name])|1,0x30000,count=10000)
            if wraps:
                assert payload_writes[0] == 128 and u.reg_read(arm.UC_ARM_REG_PC) != 0x30000
            else:
                assert u.reg_read(arm.UC_ARM_REG_PC)==0x30000,(name,case)
            results.append((trace,None if wraps else u.reg_read(arm.UC_ARM_REG_R0),bytes(u.mem_read(0x40029000,0x6000))))
        assert results[0]==results[1],(name,case,results[0][:2],results[1][:2])
    print('PASS:',name,'256 lengths; ordered pools/MMIO/payload reads, shifted pool pointers, unaligned data and FIFO slots; FD 253..255 checked through 128 writes without return')
