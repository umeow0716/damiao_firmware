from pathlib import Path
from unicorn import UC_HOOK_CODE
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split('\nfor old, name, size in mapping[:1]:')[0])
formats = ['%s', '%8s', '%-8s', '%08s', '%.0s', '%.1s', '%.3s',
           '%8.3s', '%-8.3s', '%08.3s', '%d', '%08d', '%-8d', '%x']
formats = ['%f', '%8f', '%-8f', '%08f', '%+8f', '% 8f', '%#8.0f', '%F']
for fmt in formats:
  for bits in [0x0000000000000000, 0x8000000000000000, 0x3ff4000000000000, 0xbff4000000000000]:
      results = []
      for original in (True, False):
          u = machine(original)
          u.mem_write(A(0x1ffff4bc), (0x28670).to_bytes(4, 'little'))
          u.reg_write(arm.UC_ARM_REG_C1_C0_2, 0xf00000)
          u.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
          u.mem_write(0x20001000, fmt.encode() + bytes([0]))
          u.mem_write(0x20002000, b'abcdef\0')
          u.reg_write(arm.UC_ARM_REG_R0, 0x20001000)
          u.reg_write(arm.UC_ARM_REG_R1, 0)
          u.reg_write(arm.UC_ARM_REG_R2, bits & 0xffffffff)
          u.reg_write(arm.UC_ARM_REG_R3, bits >> 32)
          u.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
          u.reg_write(arm.UC_ARM_REG_LR, 0x30001)
          emitted = bytearray()
          target = 0x24ea8 if original else symbols['platform_debug_write']
          def callback(uc, address, size, data):
              if address != target: return
              if original: emitted.append(uc.reg_read(arm.UC_ARM_REG_R0) & 255)
              else:
                  emitted.extend(uc.mem_read(uc.reg_read(arm.UC_ARM_REG_R0),
                                            uc.reg_read(arm.UC_ARM_REG_R1)))
              uc.reg_write(arm.UC_ARM_REG_PC, uc.reg_read(arm.UC_ARM_REG_LR))
          u.hook_add(UC_HOOK_CODE, callback)
          entry = 0x20474 if original else symbols['debug_console_printf']
          u.emu_start(entry | 1, 0x30000, count=100000)
          assert u.reg_read(arm.UC_ARM_REG_PC) == 0x30000, (fmt, original)
          results.append((bytes(emitted), u.reg_read(arm.UC_ARM_REG_R0)))
      assert results[0] == results[1], (fmt, results)
print('PASS: 32 whole printf finite cases; emitted bytes and return count')
