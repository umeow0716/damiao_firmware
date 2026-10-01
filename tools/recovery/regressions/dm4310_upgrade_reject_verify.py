from pathlib import Path

source = Path(__file__).with_name('dm4310_mcan_special_verify.py').read_text()
source = source.replace(
    'scenarios = tuple((address, incoming) for address, values in write_vectors\n'
    '                  for incoming in values)',
    'scenarios = (\n'
    '    (0xaa020155, 0x00000000, 0),\n'
    '    (0xaa020155, 0x00000022, 0),\n'
    '    (0xaa020155, 0x00000024, 0),\n'
    '    (0xaa020155, 0x000007ff, 0),\n'
    '    (0xaa020155, 0x00000023, 1),\n'
    '    (0xaa020155, 0x00000023, 2),\n'
    '    (0xaa020155, 0x00000023, 0xffffffff),\n'
    '    (0xaa030155, 0x00000023, 0),\n'
    '    (0xab020155, 0x00000023, 0),\n'
    '    (0xaa020154, 0x00000023, 0),\n'
    '    (0xaa010155, 0x00000023, 0),\n'
    '    (0x55020155, 0x00000023, 0),\n'
    '    (0xaa020255, 0x00000023, 0),\n'
    '    (0xaa020055, 0x00000023, 0),\n'
    '    (0x00000000, 0x00000023, 0),\n'
    '    (0xffffffff, 0x00000023, 0),\n'
    ')')
source = source.replace(
    '    address, incoming = scenarios[scenario]',
    '    header, incoming, armed = scenarios[scenario]')
source = source.replace(
    '    header = (address << 24) | (0x55 << 16) | node\n', '')
source = source.replace(
    "u.mem_write(motor + 0x38, struct.pack('<II', 0,",
    "u.mem_write(motor + 0x38, struct.pack('<II', armed,")
source = source.replace(
    '        callback_args = []',
    '        callback_args = []\n        tail_hits = []')
source = source.replace(
    '        def callback(uc, _address, _size, _):\n'
    '            callback_args.append(tuple(uc.reg_read(reg) for reg in\n'
    '                (arm.UC_ARM_REG_R0, arm.UC_ARM_REG_R1, arm.UC_ARM_REG_R2)))\n'
    '            uc.emu_stop()',
    '        def callback(uc, _address, _size, _):\n'
    "            raise AssertionError(('unexpected send', case, original))\n"
    '        def tail(uc, _address, _size, _):\n'
    '            tail_hits.append(1)\n'
    '            uc.emu_stop()')
source = source.replace(
    '        u.hook_add(UC_HOOK_CODE, callback, begin=0x2c000, end=0x2c000)',
    '        u.hook_add(UC_HOOK_CODE, tail,\n'
    '                   begin=A(0x1fff97d8) if original else '
    "symbols['platform_ack_mcan_irq'],\n"
    '                   end=A(0x1fff97d8) if original else '
    "symbols['platform_ack_mcan_irq'])\n"
    '        u.hook_add(UC_HOOK_CODE, callback, begin=0x2c000, end=0x2c000)')
source = source.replace(
    '        assert len(callback_args) == 1, (case, original,\n'
    '                                         hex(u.reg_read(arm.UC_ARM_REG_PC)),\n'
    '                                         bytes(u.mem_read(0x4003a080, 2)))',
    '        assert not callback_args and len(tail_hits) == 1, (case, original,\n'
    '                                         hex(u.reg_read(arm.UC_ARM_REG_PC)))')
source = source.replace(
    "print(f'PASS: {len(scenarios) * 2} real IRQ003 parameter WRITE paths; accepted/rejected '\n"
    "      'values, relocated owners, fixed response/filter side effects and FPSCR')",
    "print('PASS: 32 malformed/mismatched/armed upgrade rejection IRQ paths; "
    "no send, boot-record, delay or reset entry')")
exec(compile(source, '<upgrade-reject-irq>', 'exec'))
