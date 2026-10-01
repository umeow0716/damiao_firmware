"""Shared factory-layout translation for the DM43xx recovery regressions.

The regression corpus was originally written against DM4310 V5017.  DM4340
keeps that address layout, while DM8009 V6417 reorders the fixed-SRAM helper
image and most of the initialized runtime objects.  Keep that model knowledge
here so individual differential tests continue to describe one behaviour
instead of growing a second set of magic addresses.

``A`` translates a canonical DM4310 fixed-SRAM address.  Exact addresses seen
in aligned factory instruction streams take priority; the source ELF section
layout supplies the remaining byte offsets inside recovered objects.  ``F``
does the equivalent operation for matched factory flash functions.
"""

from io import BytesIO
from pathlib import Path
import csv
import os

from elftools.elf.elffile import ELFFile


ROOT = Path(__file__).resolve().parents[3]
MODEL = os.environ.get("DAMIAO_RECOVERY_MODEL", "dm4310").lower()

FACTORY_PATHS = {
    "dm4310": ROOT / "reference/APP_DM4310_V3_V5017_04.decrypted.bin",
    "dm4340": ROOT / "reference/APP_DM4340_V3_V5117_04_decrypted.bin",
    "dm8009": ROOT / "reference/APP_DM8009_V3_V6417_04_decrypted.bin",
}

if MODEL not in FACTORY_PATHS:
    raise ValueError(f"unsupported DAMIAO_RECOVERY_MODEL: {MODEL}")


def _elf(path):
    return ELFFile(BytesIO(path.read_bytes()))


def _load_tsv_map(path, left_column, right_column):
    with path.open(newline="") as source:
        return {
            int(row[left_column], 16): int(row[right_column], 16)
            for row in csv.DictReader(source, delimiter="\t")
        }


def _section_ranges():
    if MODEL != "dm8009":
        return ()
    canonical = _elf(ROOT / "build/dm4310.elf")
    target = _elf(ROOT / f"build/{MODEL}.elf")
    target_sections = {section.name: section for section in target.iter_sections()}
    dm8009_section_aliases = {
        f".dm4310_{suffix}": f".dm8009_{suffix}"
        for suffix in (
            "runtime_drive_d",
            "runtime_drive_q",
            "runtime_speed_loop",
            "runtime_position_loop",
            "adc_raw",
            "current_d",
            "current_q",
        )
    }
    ranges = []
    for section in canonical.iter_sections():
        if not section.name.startswith(".dm4310_"):
            continue
        start = section["sh_addr"]
        if not 0x1FFF8000 <= start < 0x20000000:
            continue
        peer = target_sections.get(
            dm8009_section_aliases.get(section.name, section.name)
        )
        if peer is None:
            continue
        size = min(section["sh_size"], peer["sh_size"])
        if size:
            ranges.append((start, start + size, peer["sh_addr"]))
    return tuple(sorted(ranges))


if MODEL == "dm8009":
    _SRAM_EXACT = _load_tsv_map(
        ROOT / "recovered/dm8009/tables/dm4310_sram_address_map.tsv",
        "dm4310_address",
        "dm8009_address",
    )
    _FLASH_EXACT = _load_tsv_map(
        ROOT / "recovered/dm8009/tables/dm4310_function_matches.tsv",
        "left_entry",
        "right_entry",
    )
else:
    _SRAM_EXACT = {}
    _FLASH_EXACT = {}

_SRAM_RANGES = _section_ranges()

# Factory-only ranges whose recovered source section is intentionally shorter
# or whose storage lives in linker padding.  IRQ3 is byte-for-byte the same
# size in V5017 and V6417; its one instruction-form change does not alter any
# following offsets.  The IRQ2 slice entries were aligned separately from the
# authoritative disassembly because V6417 adds instructions in that handler.
_SRAM_FACTORY_RANGES = (
    (0x1FFF80FA, 0x1FFF80FE, 0x1FFF940E),
    (0x1FFF8114, 0x1FFF8118, 0x1FFF9428),
    (0x1FFF88B8, 0x1FFF982E, 0x1FFF8278),
    (0x1FFF9878, 0x1FFF987C, 0x1FFF923C),
    (0x1FFFA560, 0x1FFFA568, 0x1FFFA648),
    (0x1FFFF4F8, 0x1FFFF8F8, 0x1FFFF530),
) if MODEL == "dm8009" else ()

_SRAM_FACTORY_EXACT = {
    0x1FFF8454: 0x1FFF976C,
    0x1FFF8458: 0x1FFF9770,
    0x1FFF845C: 0x1FFF9774,
    0x1FFF8492: 0x1FFF97DA,
    0x1FFF84EE: 0x1FFF9836,
    0x1FFF982E: 0x1FFF91EE,
    0x1FFF9C32: 0x1FFF9C6A,
    0x1FFFA2FE: 0x1FFFA336,
    0x1FFFA5BF: 0x1FFFA54F,
    # V6417 inserts its 65 V startup threshold in this literal pool, moving
    # the tail callback-owner word by four bytes.
    0x1FFF9878: 0x1FFF923C,
} if MODEL == "dm8009" else {}

# Factory flash ranges not emitted by the automatic matcher.  V6417 removes
# four bytes immediately before this main-function region; the instructions
# within the recovered loop retain their relative offsets.
_FLASH_RANGES = (
    (0x000252F4, 0x000257C4, 0x000252F0),
) if MODEL == "dm8009" else ()

# Small leaf helpers omitted from the function matcher because their bodies
# are below its minimum token count.  These entries are aligned directly from
# the factory disassembly.
_FLASH_FACTORY_EXACT = {
    # V6417 inserts four bytes before the PWM/sampling-timer initializer and
    # changes one instruction form inside it; the function role and MMIO
    # sequence remain the direct V5017 counterpart.
    0x00022FAC: 0x00022FB0,
    0x0002A448: 0x0002A480,
} if MODEL == "dm8009" else {}

# Address-space boundaries are deliberately model-independent and are often
# used as Unicorn map/hook bounds rather than as firmware objects.
_SRAM_BOUNDARIES = frozenset((0x1FFF0000, 0x1FFF7FFF, 0x1FFFFFFF))


def A(address):
    """Translate a canonical DM4310 fixed-SRAM address for ``MODEL``."""
    if MODEL != "dm8009" or address in _SRAM_BOUNDARIES:
        return address
    translated = _SRAM_FACTORY_EXACT.get(address)
    if translated is not None:
        return translated
    translated = _SRAM_EXACT.get(address)
    if translated is not None:
        return translated
    thumb = address & 1
    even_address = address & ~1
    translated = _SRAM_EXACT.get(even_address)
    if translated is not None:
        return translated | thumb
    translated = _SRAM_FACTORY_EXACT.get(even_address)
    if translated is not None:
        return translated | thumb
    for start, end, target in _SRAM_FACTORY_RANGES:
        if start <= even_address < end:
            return target + (even_address - start) + thumb
    for start, end, target in _SRAM_RANGES:
        if start <= even_address < end:
            return target + (even_address - start) + thumb
    raise ValueError(f"unmapped DM4310 SRAM address {address:#010x} for {MODEL}")


def F(address):
    """Translate a canonical DM4310 factory flash address for ``MODEL``."""
    if MODEL != "dm8009":
        return address
    translated = _FLASH_FACTORY_EXACT.get(address)
    if translated is not None:
        return translated
    translated = _FLASH_EXACT.get(address)
    if translated is not None:
        return translated
    thumb = address & 1
    even_address = address & ~1
    translated = _FLASH_FACTORY_EXACT.get(even_address)
    if translated is not None:
        return translated | thumb
    translated = _FLASH_EXACT.get(even_address)
    if translated is not None:
        return translated | thumb
    for start, end, target in _FLASH_RANGES:
        if start <= even_address < end:
            return target + (even_address - start) + thumb
    return address


FACTORY_FIXED_SOURCE = 0x8680
FIXED_IMAGE_BASE = 0x1FFF8000
FACTORY_FIXED_SIZE = {
    "dm4310": 0x2510,
    "dm4340": 0x2510,
    "dm8009": 0x2548,
}[MODEL]
FACTORY_STATE_SOURCE = FACTORY_FIXED_SOURCE + FACTORY_FIXED_SIZE
FACTORY_STATE_SIZE = {
    "dm4310": 0x2258,
    "dm4340": 0x2258,
    "dm8009": 0x2268,
}[MODEL]
FACTORY_STATE_BASE = {
    "dm4310": 0x1FFFA510,
    "dm4340": 0x1FFFA510,
    "dm8009": 0x1FFFA548,
}[MODEL]
FACTORY_STACK_TOP = {
    "dm4310": 0x200004F8,
    "dm4340": 0x200004F8,
    "dm8009": 0x20000530,
}[MODEL]

BUS_OVERVOLTAGE_BITS = {
    "dm4310": 0x42000000,
    "dm4340": 0x42000000,
    "dm8009": 0x42820000,
}[MODEL]
