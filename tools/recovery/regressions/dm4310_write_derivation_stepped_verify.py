from pathlib import Path
exec(Path(__file__).with_name('dm4310_softfloat_verify.py').read_text().split(
    '\nfor old, name, size in mapping[:1]:')[0])
startup = machine(True)
startup.mem_write(0x20000, factory)
startup.reg_write(arm.UC_ARM_REG_SP, 0x2000f000)
startup.emu_start(0x20259, 0x20368, count=1000000)
assert startup.reg_read(arm.UC_ARM_REG_PC) == 0x20368
sbox = bytes(startup.mem_read(A(0x1fffc678), 0x100))
assert sbox == elf.get_section_by_name('.dm4310_aes_sbox').data()
source = Path(__file__).with_name('dm4310_gain_stepped_verify.py').read_text()
source = source.replace('if 0x19 <= address <= 0x1c for incoming in values',
                        'if address in (0x18, 0x21, 0x22) for incoming in values')
source = source.replace('range(20000)', 'range(200000)')
source = source.replace("source = source.replace(\n    'scenarios =",
    "source = source.replace('if case == 56:', 'if case == 0:')\n"
    "source = source.replace(\n    'scenarios =", 1)
source = source.replace(
    "source = source.replace(\n    '        for lo, hi in ranges:',",
    "source = source.replace('1d11662f650915a6593211985a191655', "
    "'6c0a64f59cd46c2217ae8a9e12d7ad0c')\n"
    "source = source.replace(\n    '        for lo, hi in ranges:',", 1)
source = source.replace('PASS: 32 gain IRQ paths', 'PASS: 24 derivation WRITE IRQ paths')
source = source.replace(
    '(A(0x1fff9838), A(0x1fff983f))',
    '(A(0x1fff9834), A(0x1fff983f)), (A(0x1ffff23c), A(0x1ffff29b))')
source = source.replace(
    "exec(compile(source, '<gain-stepped-irq>', 'exec'))",
    "source = source.replace('        trace = []', "
    "'        u.mem_write(A(0x1fffc678), bytes.fromhex(\"" + sbox.hex() +
    "\"))\\n        trace = []')\n"
    "exec(compile(source, '<derivation-stepped-irq>', 'exec'))")
exec(compile(source, '<derivation-stepped-setup>', 'exec'))
