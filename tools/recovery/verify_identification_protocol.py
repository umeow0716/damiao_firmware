#!/usr/bin/env python3
"""Check the documented wire fixture and decoder against the native C implementation."""
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
document = (ROOT / "docs/SYSTEM_IDENTIFICATION_PROTOCOL.md").read_text()
fixture = re.search(r"6F2 FD64 BRS\n([0-9A-F \n]+)\n```", document)
assert fixture is not None
payload = bytes.fromhex(fixture.group(1))
assert len(payload) == 64
actual = subprocess.check_output(
    [str(ROOT / "build/tests/test_system_identification"), "--fixture"], text=True)
assert bytes.fromhex(actual) == payload, "documented sample differs from C encoder"
decoder = re.search(r"```python\n(.*?)\n```", document, re.S)
assert decoder is not None
namespace = {}
exec(decoder.group(1), namespace)
result = namespace["decode_sample"](payload)
assert result == dict(node=1, flags=9, sequence=0, tick=40, applied=1, dropped=0,
                      position=1.25, velocity=-2.0, iq_mean=3.0, torque=1.5,
                      command=-1.0, iq_instant=3.0, applied_tick=1,
                      command_kind="mit_feedforward_nm", velocity_setpoint_changed=False,
                      temperature=30.0, velocity_tick=40, position_tick=39,
                      period=40, mode=1, fault=0)
for bad in (payload[:-1], bytes([2]) + payload[1:], b"\x01\x80" + payload[2:]):
    try:
        namespace["decode_sample"](bad)
    except ValueError:
        pass
    else:
        raise AssertionError("decoder accepted an invalid packet")
print("documented FD64 fixture/decoder matches C encoder; length/version/type rejection passed")

velocity=bytes.fromhex(subprocess.check_output([str(ROOT / "build/tests/test_system_identification"),"--velocity-fixture"],text=True))
reference=namespace["decode_sample"](velocity)
assert reference["mode"]==3 and reference["command_kind"]=="velocity_setpoint_rad_s"
assert abs(reference["command"]+.06)<1e-7
print("documented VEL v2 decoder matches native firmware applied-reference encoder")
