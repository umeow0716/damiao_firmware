from pathlib import Path
import struct
from unicorn import (UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE,
                     UC_MEM_READ, UC_MEM_WRITE)
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
for case in range(512):
    status, config, sample, motor = ((A(0x1ffff1f0),A(0x1fffa5c8),A(0x1ffff104),A(0x1ffff088))
        if case < 256 else (0x20002000,0x20002200,0x20002400,0x20002600))
    initial = {status:bytearray(0x4c),config:bytearray(0x98),sample:bytearray(0xa4),motor:bytearray(0x7c)}
    def word(base, offset, value): struct.pack_into('<I',initial[base],offset,value)
    def floating(base, offset, value): struct.pack_into('<f',initial[base],offset,value)
    word(status,0,case%4*100);word(status,4,(case>>2)&1)
    word(config,0x24,0 if case%8==0 else 150)
    for offset, threshold in [(0x44,8000),(0x40,8000),(0x3c,5000),(0x34,5000),(0x38,20000)]:
        word(status,offset,rng.choice([0,threshold-1,threshold,threshold+1,0xffffffff]))
    floating(config,8,80);floating(config,12,10);floating(config,0,12);floating(config,0x74,48)
    floating(motor,0x40,90 if case&1 else 40)
    floating(sample,0x84,130 if case&2 else 30)
    floating(sample,0x64,15 if case&4 else -5)
    floating(sample,0x68,[5,24,60,float('nan')][(case>>3)%4])
    word(sample,0x80,(case>>5)%16);word(motor,0x38,(case>>1)%4)
    result=[]
    for original in (True,False):
        u=machine(original)
        if original:u.mem_write(FIXED_IMAGE_BASE, factory[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE])
        else:
            for s in elf.iter_sections():
                if FIXED_IMAGE_BASE<=s['sh_addr']<FACTORY_STATE_BASE and s['sh_type']!='SHT_NOBITS':u.mem_write(s['sh_addr'],s.data())
        for base,data in initial.items():u.mem_write(base,bytes(data))
        u.mem_write(A(0x1fff83f8),struct.pack('<IIIII',status,config,sample,motor,0x42f00000))
        u.reg_write(arm.UC_ARM_REG_SP,0x2000f000);u.reg_write(arm.UC_ARM_REG_LR,0x30001)
        u.reg_write(arm.UC_ARM_REG_C1_C0_2,0xf00000);u.reg_write(arm.UC_ARM_REG_FPEXC,0x40000000)
        u.reg_write(arm.UC_ARM_REG_FPSCR,(case%4)<<22)
        trace=[]
        def hook(uc,access,address,size,value,data):
            # STRD is exposed as two word writes by Unicorn on both paths.
            trace.append((access,address,size,value if access==UC_MEM_WRITE else int.from_bytes(uc.mem_read(address,size),'little')))
        def factory_unicorn_overvoltage_read(uc, address, size, data):
            # Unicorn skips either V6417 LDR immediately following this
            # VCMPE/BLE sequence.  Both instructions are present in the
            # factory disassembly; restore the required read and destination
            # register when execution reaches the following instruction.
            target = status + (0x38 if address == A(0x1fff80fc) else 4)
            value = int.from_bytes(uc.mem_read(target, 4), 'little')
            uc.reg_write(arm.UC_ARM_REG_R2, value)
            event = (UC_MEM_READ, target, 4, value)
            if not trace or trace[-1] != event:
                trace.append(event)
        for lo,hi in [(A(0x1fff83f8),A(0x1fff840b))]+[(a,a+len(b)-1) for a,b in initial.items()]:
            u.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,hook,begin=lo,end=hi)
        if original and model == 'dm8009':
            u.hook_add(UC_HOOK_CODE, factory_unicorn_overvoltage_read,
                       begin=A(0x1fff80fc), end=A(0x1fff80fc))
            u.hook_add(UC_HOOK_CODE, factory_unicorn_overvoltage_read,
                       begin=A(0x1fff8116), end=A(0x1fff8116))
        u.emu_start(A(0x1fff8001),0x30000,count=10000)
        assert u.reg_read(arm.UC_ARM_REG_PC)==0x30000
        result.append((trace,[bytes(u.mem_read(a,len(b))) for a,b in initial.items()],u.reg_read(arm.UC_ARM_REG_FPSCR)))
    assert result[0]==result[1],(case,result)
print('PASS: 512 full fault-monitor calls, original/relocated state, ordered pools/state and FPSCR; g_app mirrors/raw ABI excluded')
