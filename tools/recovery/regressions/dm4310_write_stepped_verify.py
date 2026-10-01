from pathlib import Path
source = Path(__file__).with_name('dm4310_gain_stepped_verify.py').read_text()
source = source.replace('if 0x19 <= address <= 0x1c for incoming in values',
                        'if address not in (0x18, 0x21, 0x22) for incoming in values')
source = source.replace('PASS: 32 gain IRQ paths', 'PASS: 184 WRITE IRQ paths')
exec(compile(source, '<write-stepped-setup>', 'exec'))
