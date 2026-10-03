from pathlib import Path
import struct
import sys
from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_WRITE
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
full_chain = '--full-chain' in sys.argv
memory_chain = '--memory-chain' in sys.argv
for case in range(2048 if full_chain else 128):
    input_case = case // 4 if full_chain else case
    angle = struct.pack('<f', ((case % 33) - 16) / 8.0)
    state = bytearray(0x48)
    for offset, value in [(0x10,1.0),(0x18,1.0),(0x2c,1.0),(0x30,1.0),
                           (0x38,0.25),(0x40,(-1.0 if case & 1 else 1.0)*6.0)]:
        struct.pack_into('<f',state,offset,value)
    struct.pack_into('<i',state,0x44,(case%5)-2)
    if full_chain:
        values = [0.0, -0.0, 0.125, -0.125, 1.0, -1.0, 16.0, -16.0,
                  1e-38, -1e-38, 1e-30, -1e-30, 1e30, -1e30, 1e38, -1e38]
        struct.pack_into('<f',state,0x10,values[input_case % 16])
        struct.pack_into('<f',state,0x18,values[(input_case // 16) % 16])
        struct.pack_into('<f',state,0x30,0.0)
    results=[]
    for original in (True,False):
        u=machine(original)
        if original: u.mem_write(FIXED_IMAGE_BASE, factory[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE])
        else:
            for section in elf.iter_sections():
                if FIXED_IMAGE_BASE <= section['sh_addr'] < FACTORY_STATE_BASE and section['sh_type'] != 'SHT_NOBITS':
                    u.mem_write(section['sh_addr'],section.data())
        u.mem_write(A(0x1fffd078),b''.join(struct.pack('<H',(i*7)%4096) for i in range(4096)))
        u.mem_write(0x20008000,bytes(state))
        u.reg_write(arm.UC_ARM_REG_C1_C0_2,0xf00000)
        u.reg_write(arm.UC_ARM_REG_FPEXC,0x40000000)
        u.reg_write(arm.UC_ARM_REG_FPSCR,(case%4)<<22)
        u.reg_write(arm.UC_ARM_REG_R0,0x20008000)
        u.reg_write(arm.UC_ARM_REG_SP,0x2000f000)
        u.reg_write(arm.UC_ARM_REG_LR,0x30001)
        target=A(0x1fffa4fa) if original else symbols['output_atan2f']
        def skip_atan(uc,address,size,data):
            if address==target:
                uc.reg_write(arm.UC_ARM_REG_S0,int.from_bytes(angle,'little'))
                uc.reg_write(arm.UC_ARM_REG_PC,uc.reg_read(arm.UC_ARM_REG_LR))
        trace=[]
        def read(uc,access,address,size,value,data):
            contents = value.to_bytes(size,'little') if access == UC_MEM_WRITE else bytes(uc.mem_read(address,size))
            trace.append((access,address,size,contents))
        if not full_chain: u.hook_add(UC_HOOK_CODE,skip_atan)
        u.hook_add(UC_HOOK_MEM_READ,read,begin=A(0x1fff9934),end=A(0x1fff994f))
        if memory_chain:
            u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE,read,begin=0x20008000,end=0x20008047)
            u.hook_add(UC_HOOK_MEM_READ,read,begin=A(0x1fffd078),end=A(0x1ffff077))
            u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE,read,begin=A(0x1ffff490),end=A(0x1ffff4ef))
        entry=A(0x1fff987c) if original else symbols['output_sensor_helper']
        u.emu_start(entry|1,0x30000,count=10000)
        assert u.reg_read(arm.UC_ARM_REG_PC)==0x30000
        results.append((bytes(u.mem_read(0x20008000,0x48)),trace,u.reg_read(arm.UC_ARM_REG_FPSCR)))
    assert results[0]==results[1],(case,results)
print('PASS:',2048 if full_chain else 128,'lookup/unwrap cases; full state, FPSCR and ordered SRAM reads/writes match;', 'real atan chain' if full_chain else 'atan intercepted')
