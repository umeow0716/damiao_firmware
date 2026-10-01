from pathlib import Path

source = Path(__file__).with_name('dm4310_upgrade_reply_verify.py').read_text()
# Obtain the reply harness without executing it yet.
source = source.replace("exec(compile(source, '<upgrade-reply-irq>', 'exec'))", '')
namespace = {
    '__file__': str(Path(__file__).with_name('dm4310_upgrade_reply_verify.py')),
}
exec(compile(source, '<upgrade-setup>', 'exec'), namespace)
source = namespace['source']
source = source.replace(
    '            uc.emu_stop()\n        def rng_ready',
    '            uc.reg_write(arm.UC_ARM_REG_PC, '
    'uc.reg_read(arm.UC_ARM_REG_LR))\n'
    '        def sequence(uc, address, size, _):\n'
    '            if address == A(0x1fffa4f0):\n'
    "                trace.append(('bank-b',))\n"
    '                uc.reg_write(arm.UC_ARM_REG_PC, '
    'uc.reg_read(arm.UC_ARM_REG_LR))\n'
    "            elif address == (A(0x1fffa4e6) if original else symbols['board_delay_ms']):\n"
    "                trace.append(('delay-ms', uc.reg_read(arm.UC_ARM_REG_R0)))\n"
    '                uc.reg_write(arm.UC_ARM_REG_PC, '
    'uc.reg_read(arm.UC_ARM_REG_LR))\n'
    "            elif bytes(uc.mem_read(address, 4)) == bytes.fromhex('bff34f8f'):\n"
    "                trace.append(('dsb',))\n"
    '            elif (original and address == A(0x1fff97a8)) or '
    "(not original and address == symbols['reset_after_barrier'] + 0x12):\n"
    '                uc.emu_stop()\n'
    '        def rng_ready')
source = source.replace(
    '        u.hook_add(UC_HOOK_CODE, callback, begin=0x2c000, end=0x2c000)',
    '        u.hook_add(UC_HOOK_CODE, sequence)\n'
    '        u.hook_add(UC_HOOK_CODE, callback, begin=0x2c000, end=0x2c000)')
source = source.replace(
    "print('PASS: 32 real upgrade IRQ reply paths; retained node, relocated owners, ordered eight-byte payload and send ABI')",
    "print('PASS: 32 upgrade IRQ sequences through send return, bank-B entry, "
    "10-ms delay entry and AIRCR reset; bank/delay bodies intercepted')")
exec(compile(source, '<upgrade-sequence-irq>', 'exec'))
