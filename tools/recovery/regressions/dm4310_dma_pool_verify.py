from pathlib import Path
import struct
import sys
from unicorn import (UC_HOOK_MEM_INVALID, UC_HOOK_MEM_READ,
                     UC_HOOK_MEM_WRITE, UC_MEM_WRITE)
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
special = '--special' in sys.argv
edges = [0,0x80000000,1,0x80000001,0x7f800000,0xff800000,0x7fc12345,0x7f812345,0xffc34567,0xff834567]
for case in range(1024 if special else 256):
    motor,raw,scratch,table = ((A(0x1ffff088),A(0x1fffa666),A(0x1ffff190),A(0x1fffc84c))
        if case<128 else (0x20005000,0x20005200,0x20005400,0x20005800))
    dma_count=0x40053448 if case<128 else 0x40054000
    ready=case%8!=0; word=rng.randrange(65536);dt=rng.getrandbits(32)
    initial_motor=bytearray(0x7c);initial_scratch=bytearray(0x18)
    struct.pack_into('<f',initial_motor,0x34,1.0 if case%2 else -1.0)
    struct.pack_into('<f',initial_motor,0x5c,0.125)
    struct.pack_into('<f',initial_scratch,12,rng.uniform(0,6.28))
    struct.pack_into('<f',initial_scratch,16,rng.uniform(-1,1))
    correction=struct.pack('<256f',*(rng.uniform(-0.5,0.5) for _ in range(256)))
    if special:
        correction=struct.pack('<256I',*(edges[(case//16+i)%len(edges)] for i in range(256)))
        struct.pack_into('<I',initial_scratch,12,edges[(case//32)%len(edges)])
        struct.pack_into('<I',initial_scratch,16,edges[(case//64)%len(edges)])
    results=[]
    for original in (True,False):
        u=machine(original)
        if original:u.mem_write(FIXED_IMAGE_BASE, factory[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE])
        else:
            for s in elf.iter_sections():
                if FIXED_IMAGE_BASE<=s['sh_addr']<FACTORY_STATE_BASE and s['sh_type']!='SHT_NOBITS':u.mem_write(s['sh_addr'],s.data())
        u.mem_map(0x40053000,0x2000);u.mem_map(0xe000e000,0x1000)
        u.mem_write(dma_count,struct.pack('<I',dt));u.mem_write(0x40053408,struct.pack('<I',int(ready)))
        u.mem_write(A(0x1fff8b50),struct.pack('<IIII',motor,raw,scratch,table));u.mem_write(A(0x1fff8b74),struct.pack('<I',dma_count))
        u.mem_write(A(0x1fff8b48),struct.pack('<f',-0.25 if case&16 else 0.0))
        u.mem_write(A(0x1fff8b60),struct.pack('<5f',1.0/(32 if case&32 else 64),
            0.0003834952,6.0 if case&64 else 6.2831855,5.5,-5.5))
        if special:
            u.mem_write(A(0x1fff8b60),struct.pack('<I',edges[(case//16)%len(edges)]))
            u.mem_write(A(0x1fff8b64),struct.pack('<I',edges[(case//160)%len(edges)]))
        u.mem_write(motor,bytes(initial_motor));u.mem_write(scratch,bytes(initial_scratch));u.mem_write(raw,struct.pack('<H',word));u.mem_write(table,correction)
        mirror=symbols['position_sensor']
        for offset in (8,16,20,24):u.mem_write(mirror+offset,struct.pack('<I',0x7f812345))
        u.reg_write(arm.UC_ARM_REG_SP,0x2000f000);u.reg_write(arm.UC_ARM_REG_C1_C0_2,0xf00000);u.reg_write(arm.UC_ARM_REG_FPEXC,0x40000000)
        u.reg_write(arm.UC_ARM_REG_FPSCR,((case%4)<<22)|((((case//4)%4)<<24) if special else 0))
        trace=[];done=[False]
        def hook(uc,access,address,size,value,data):
            trace.append((access,address,size,value if access==UC_MEM_WRITE else int.from_bytes(uc.mem_read(address,size),'little')))
            if address==0xe000e280 and access==UC_MEM_WRITE:done[0]=True;uc.emu_stop()
        def invalid(uc,access,address,size,value,data):
            print('invalid',case,original,hex(uc.reg_read(arm.UC_ARM_REG_PC)),
                  hex(address),size)
            return False
        u.hook_add(UC_HOOK_MEM_INVALID, invalid)
        for lo,hi in [(A(0x1fff8b48),A(0x1fff8b7f)),(motor,motor+0x7b),(raw,raw+1),(scratch,scratch+0x17),(table,table+1023),(0x40053000,0x40054fff),(0xe000e280,0xe000e283),(mirror+8,mirror+11),(mirror+16,mirror+27)]:
            u.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,hook,begin=lo,end=hi)
        u.emu_start(A(0x1fff8771),0x30000,count=20000);assert done[0]
        results.append((trace,bytes(u.mem_read(motor,0x7c)),bytes(u.mem_read(scratch,0x18)),u.reg_read(arm.UC_ARM_REG_FPSCR)))
    if results[0]!=results[1]:
        for i,(a,b) in enumerate(zip(results[0][0],results[1][0])):
            if a!=b:print('first trace difference',i,a,b);break
        raise AssertionError((case, 'FPSCR', results[0][-1],results[1][-1]))
print('PASS',1024 if special else 256,'real DMA IRQ paths: relocated pointers/numeric pools, ordered state/MMIO/FPSCR, four poisoned unused mirrors untouched; other mirrors/stack excluded')
