from pathlib import Path

source = Path(__file__).with_name('dm4310_mcan_special_verify.py').read_text()
source = source.replace(
    'scenarios = tuple((address, incoming) for address, values in write_vectors\n'
    '                  for incoming in values)',
    'scenarios = tuple((address, incoming) for address, values in write_vectors\n'
    '                  if 0x19 <= address <= 0x1c for incoming in values)')
source = source.replace(
    '        for lo, hi in ranges:',
    '        ranges.extend(((A(0x1fff9838), A(0x1fff983f)),\n'
    '                       (A(0x1fffa510), A(0x1fffa510) + 0x27),\n'
    '                       (A(0x1fffa538), A(0x1fffa538) + 0x27),\n'
    '                       (A(0x1fffa568), A(0x1fffa568) + 0x27),\n'
    '                       (A(0x1fffa590), A(0x1fffa590) + 0x27),\n'
    "                       (symbols['g_app'], symbols['g_app'] + 0x93)))\n"
    '        for lo, hi in ranges:')
source = source.replace(
    '        for lo, hi in ranges:',
    '        if relocated:\n'
    '            u.mem_write(A(0x1fff9838), struct.pack("<II", '
    '0x2000c200, 0x2000c400))\n'
    '            ranges.extend(((0x2000c200, 0x2000c227), '
    '(0x2000c400, 0x2000c427)))\n'
    '        for lo, hi in ranges:')
source = source.replace(
    '            u.hook_add(UC_HOOK_CODE, write_pc,\n'
    '                       begin=A(0x1fff921e), end=A(0x1fff968c))\n', '')
source = source.replace(
    '            u.emu_start(A(0x1fff88b9), 0x30000, count=3000000)',
    '            pc = A(0x1fff88b9)\n'
    '            for instruction in range(20000):\n'
    '                u.emu_start(pc, 0x30000, count=1)\n'
    '                if callback_args:\n'
    '                    break\n'
    '                pc = u.reg_read(arm.UC_ARM_REG_PC) | 1')
source = source.replace(
    "print(f'PASS: {len(scenarios) * 2} real IRQ003 parameter WRITE paths; accepted/rejected '\n"
    "      'values, relocated owners, fixed response/filter side effects and FPSCR')",
    "print('PASS: 32 gain IRQ paths under single-instruction execution; "
    "literal/coefficients/decoded-config traced without instruction normalization')")
exec(compile(source, '<gain-stepped-irq>', 'exec'))
