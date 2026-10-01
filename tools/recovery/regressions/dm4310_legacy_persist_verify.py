from pathlib import Path

# Reuse the isolated real-IRQ setup and owner trace, with a returning send
# callback. Stop only after the persistence store, at the IRQ-tail boundary.
source = Path(__file__).with_name('dm4310_mcan_special_verify.py').read_text()
source = source.replace(
    'scenarios = tuple((address, incoming) for address, values in write_vectors\n'
    '                  for incoming in values)',
    'scenarios = tuple((node, master) for node in (1, 35, 127, 255)\n'
    '                  for master in (0, 69, 128, 255))')
source = source.replace(
    'header = (address << 24) | (0x55 << 16) | node',
    'header = 0x550000aa | (address << 8) | (incoming << 16)')
source = source.replace('            uc.emu_stop()\n        def rng_ready',
    '            uc.reg_write(arm.UC_ARM_REG_PC, '
    'uc.reg_read(arm.UC_ARM_REG_LR))\n'
    '        def tail_stop(uc, address, size, _):\n'
    '            uc.emu_stop()\n'
    '        def rng_ready')
source = source.replace(
    '        u.hook_add(UC_HOOK_CODE, callback, begin=0x2c000, end=0x2c000)',
    '        u.hook_add(UC_HOOK_CODE, tail_stop,\n'
    '                   begin=A(0x1fff97d8) if original else '
    "symbols['platform_ack_mcan_irq'],\n"
    '                   end=A(0x1fff97d8) if original else '
    "symbols['platform_ack_mcan_irq'])\n"
    '        u.hook_add(UC_HOOK_CODE, callback, begin=0x2c000, end=0x2c000)')
source = source.replace(
    "print(f'PASS: {len(scenarios) * 2} real IRQ003 parameter WRITE paths; accepted/rejected '\n"
    "      'values, relocated owners, fixed response/filter side effects and FPSCR')",
    "print('PASS: 32 real legacy ID IRQ paths through returning send and "
    "full-word retained-status persistence store')")
exec(compile(source, '<legacy-persist-irq>', 'exec'))
