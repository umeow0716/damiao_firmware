from pathlib import Path

source = Path(__file__).with_name('dm4310_mcan_special_verify.py').read_text()
source = source.replace(
    'scenarios = tuple((address, incoming) for address, values in write_vectors\n'
    '                  for incoming in values)',
    'scenarios = tuple((0, 0x23 | (high << 16)) for high in range(16))')
source = source.replace(
    'header = (address << 24) | (0x55 << 16) | node',
    'header = 0xaa020155')
source = source.replace(
    "print(f'PASS: {len(scenarios) * 2} real IRQ003 parameter WRITE paths; accepted/rejected '\n"
    "      'values, relocated owners, fixed response/filter side effects and FPSCR')",
    "print('PASS: 32 real upgrade IRQ reply paths; retained node, "
    "relocated owners, ordered eight-byte payload and send ABI')")
exec(compile(source, '<upgrade-reply-irq>', 'exec'))
