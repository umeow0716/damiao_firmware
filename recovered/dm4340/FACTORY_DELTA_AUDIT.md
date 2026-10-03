# DM4340 V3 V5117 Factory Delta Audit

Authoritative references:

- `reference/APP_DM4310_V3_V5017_04.decrypted.bin`
- `reference/APP_DM4340_V3_V5117_04_decrypted.bin`

Both images are 51,284 bytes and use the same vector table, fixed SRAM ABI,
entry points, peripheral access logic and runtime helpers.  The direct scan of
the uncompressed Flash prefix has four changed bytes:

| Address | V5017 | V5117 | Meaning |
|---:|---:|---:|---|
| `0x22b95` | `0x30` | `0x31` | ASCII software version `5017` -> `5117` |
| `0x255cd` | `0x30` | `0x31` | second ASCII software-version literal |
| `0x26818` | `0x99` | `0xfd` | `MOVW r1,#5017` -> `MOVW r1,#5117` |
| `0x28654` | `0x4c` | `0x50` | compressed scatter-stream end pointer |

The complete raw fixed-SRAM image at file offsets `0x8680..0xab8f` is
byte-identical.  Of the 170 inventoried factory functions, the only changed
instruction is the diagnostic version immediate in `FUN_00026758`; the other
differences are data literals or scatter metadata.

Executing each factory's real scatter loader produces exactly eight changed
32-bit words in the same staging record at `0x1fffa5c8`:

| Word | Address | DM4310 | DM4340 | Field |
|---:|---:|---:|---:|---|
| `0x0c` | `0x1fffa5f8` | `0x3796feb5` | `0x37a7c5ac` | rotor inertia |
| `0x11` | `0x1fffa60c` | `0x3f59999a` | `0x3f6147ae` | phase resistance |
| `0x12` | `0x1fffa610` | `0x39b4e11e` | `0x39bcbe62` | phase inductance |
| `0x13` | `0x1fffa614` | `0x3b9374bc` | `0x3b9eecc0` | flux linkage |
| `0x14` | `0x1fffa618` | `0x41200000` | `0x42200000` | gear ratio |
| `0x16` | `0x1fffa620` | `0x41f00000` | `0x41200000` | velocity maximum |
| `0x17` | `0x1fffa624` | `0x41200000` | `0x41e00000` | torque maximum |
| `0x19` | `0x1fffa62c` | `0x3b73cb3e` | `0x3b7ba882` | speed-loop Kp |

There are 18 changed bytes within those words; all remaining bytes across the
two mapped 64 KiB SRAM banks match after startup.  The source uses one shared
DM43xx implementation and keeps only these values plus version/application
identity in the DM4340 target profile.

Persistent verification:

- `tools/recovery/verify_dm4340_factory_delta.py`
- `tools/recovery/compare_factory_startup.py`
- `tools/recovery/verify_dm43xx_startup.py`
- `tools/recovery/verify_dm4310_full_regressions.py --model dm4340`
- `tools/recovery/verify_dm4310_core_regressions.py --model dm4340`

The full 65-script differential suite and all 15 composition regressions pass
against the DM4340 factory image and `build/dm4340.elf`.
