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
    "dm10010": ROOT / "reference/APP_DM10010(V3)_V5617_04.decrypted.bin",
    "dm3507": ROOT / "reference/APP_DM3507(V3)_V5717_04.decrypted.bin",
    "dm3507_48v": ROOT / "reference/APP_DM3507(V3_48V)_V6517_04.decrypted.bin",
    "dm4310": ROOT / "reference/APP_DM4310_V3_V5017_04.decrypted.bin",
    "dm4310_48v": ROOT / "reference/APP_DM4310(48V)_V6017_04.decrypted.bin",
    "dm4340": ROOT / "reference/APP_DM4340_V3_V5117_04_decrypted.bin",
    "dm4340_48v": ROOT / "reference/APP_DM4340(48V)_V6117_04.decrypted.bin",
    "dm8006": ROOT / "reference/APP_DM8006(V3)_V6317_04.decrypted.bin",
    "dm8009": ROOT / "reference/APP_DM8009_V3_V6417_04_decrypted.bin",
}

DM43_48V_MODELS = frozenset(
    ("dm10010", "dm3507_48v", "dm4310_48v", "dm4340_48v")
)
DM800X_MODELS = frozenset(("dm8006", "dm8009"))

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
    if MODEL == "dm4310":
        return ()
    canonical = _elf(ROOT / "build/dm4310.elf")
    target = _elf(ROOT / f"build/{MODEL}.elf")
    target_sections = {section.name: section for section in target.iter_sections()}
    dm800x_section_aliases = {
        f".{suffix}": f".alternate_{suffix}"
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
        if not section.name.startswith("."):
            continue
        start = section["sh_addr"]
        if not 0x1FFF8000 <= start < 0x20000000:
            continue
        peer = target_sections.get(
            dm800x_section_aliases.get(section.name, section.name)
            if MODEL in DM800X_MODELS else section.name
        )
        if peer is None:
            continue
        size = min(section["sh_size"], peer["sh_size"])
        if size:
            ranges.append((start, start + size, peer["sh_addr"]))
    return tuple(sorted(ranges))


if MODEL != "dm4310":
    _SRAM_EXACT = _load_tsv_map(
        ROOT / f"recovered/{MODEL}/tables/dm4310_sram_address_map.tsv",
        "canonical_address",
        "target_address",
    )

if MODEL != "dm4310":
    _FLASH_EXACT = _load_tsv_map(
        ROOT / f"recovered/{MODEL}/tables/dm4310_function_matches.tsv",
        "left_entry",
        "right_entry",
    )
    _FLASH_INSTRUCTION_EXACT = _load_tsv_map(
        ROOT / f"recovered/{MODEL}/tables/"
        "dm4310_instruction_address_map.tsv",
        "canonical_address",
        "target_address",
    )
else:
    _FLASH_EXACT = {}
    _FLASH_INSTRUCTION_EXACT = {}

if MODEL == "dm4310":
    _SRAM_EXACT = {}

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
) if MODEL in DM800X_MODELS else ()

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
} if MODEL in DM800X_MODELS else {}

# Factory flash ranges not emitted by the automatic matcher.  V6417 removes
# four bytes immediately before this main-function region; the instructions
# within the recovered loop retain their relative offsets.
_FLASH_RANGES = (
    (0x000252F4, 0x000257C4, 0x000252F0),
) if MODEL in DM800X_MODELS else ()

# Small leaf helpers omitted from the function matcher because their bodies
# are below its minimum token count.  These entries are aligned directly from
# the factory disassembly.
_FLASH_FACTORY_EXACT = {
    # V6417 inserts four bytes before the PWM/sampling-timer initializer and
    # changes one instruction form inside it; the function role and MMIO
    # sequence remain the direct V5017 counterpart.
    0x00022FAC: 0x00022FAC if MODEL == "dm8006" else 0x00022FB0,
    0x0002A448: 0x0002A480,
} if MODEL in DM800X_MODELS else {}


def _matched_flash_ranges():
    """Build function-relative ranges from the persisted model match table."""
    if MODEL == "dm4310":
        return ()
    canonical_rows = {
        int(row["entry"], 16): row
        for row in csv.DictReader(
            (ROOT / "recovered/dm4310/tables/factory_function_inventory.tsv").open(
                newline=""
            ),
            delimiter="\t",
        )
    }
    ranges = []
    for canonical, target in _FLASH_EXACT.items():
        if not 0x00020000 <= canonical < 0x00040000:
            continue
        row = canonical_rows.get(canonical)
        if row is None:
            continue
        end = int(row["end"], 16) + 1
        ranges.append((canonical, end, target))
    return tuple(sorted(ranges))


_FLASH_MATCHED_RANGES = _matched_flash_ranges()

# These three read-only objects immediately precede the scatter-loaded image.
# Their model displacement follows the factory fixed-image load address, even
# though they are not function entries and therefore are absent from the
# instruction/function match tables.
_FACTORY_TAIL_DATA_DELTA = {
    "dm10010": 0,
    "dm3507": -8,
    "dm3507_48v": 0,
    "dm4310": 0,
    "dm4310_48v": 8,
    "dm4340": 0,
    "dm4340_48v": 8,
    "dm8006": 0,
    "dm8009": 0,
}[MODEL]
_FLASH_DATA_EXACT = {
    address: address + _FACTORY_TAIL_DATA_DELTA
    for address in (0x00027CA0, 0x00028668, 0x00028670)
}

# Address-space boundaries are deliberately model-independent and are often
# used as Unicorn map/hook bounds rather than as firmware objects.
_SRAM_BOUNDARIES = frozenset((0x1FFF0000, 0x1FFF7FFF, 0x1FFFFFFF))


def A(address):
    """Translate a canonical DM4310 fixed-SRAM address for ``MODEL``."""
    if MODEL in ("dm4310", "dm3507", "dm4340") or address in _SRAM_BOUNDARIES:
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
    # Reference tables normally contain the aligned start of a 32-bit SRAM
    # object.  Verifiers also pass inclusive byte-range endpoints, so carry
    # the proven word relocation across the other three bytes.
    word_address = address & ~3
    translated = _SRAM_EXACT.get(word_address)
    if translated is not None:
        return translated + (address - word_address)
    translated = _SRAM_FACTORY_EXACT.get(word_address)
    if translated is not None:
        return translated + (address - word_address)
    for start, end, target in _SRAM_FACTORY_RANGES:
        if start <= even_address < end:
            return target + (even_address - start) + thumb
    for start, end, target in _SRAM_RANGES:
        if start <= even_address < end:
            return target + (even_address - start) + thumb
    if MODEL in DM43_48V_MODELS:
        if 0x1FFFF4F8 <= even_address < 0x1FFFF8F8:
            return even_address + 8 + thumb
        if 0x1FFF9878 <= even_address < 0x1FFFF4F8:
            return even_address + 4 + thumb
        if FIXED_IMAGE_BASE <= even_address < 0x1FFF9878:
            return even_address + thumb
    raise ValueError(f"unmapped DM4310 SRAM address {address:#010x} for {MODEL}")


def F(address):
    """Translate a canonical DM4310 factory flash address for ``MODEL``."""
    if MODEL == "dm4310":
        return address
    translated = _FLASH_DATA_EXACT.get(address)
    if translated is not None:
        return translated
    translated = _FLASH_INSTRUCTION_EXACT.get(address)
    if translated is not None:
        return translated
    translated = _FLASH_FACTORY_EXACT.get(address)
    if translated is not None:
        return translated
    translated = _FLASH_EXACT.get(address)
    if translated is not None:
        return translated
    thumb = address & 1
    even_address = address & ~1
    translated = _FLASH_INSTRUCTION_EXACT.get(even_address)
    if translated is not None:
        return translated | thumb
    translated = _FLASH_FACTORY_EXACT.get(even_address)
    if translated is not None:
        return translated | thumb
    translated = _FLASH_EXACT.get(even_address)
    if translated is not None:
        return translated | thumb
    for start, end, target in _FLASH_RANGES:
        if start <= even_address < end:
            return target + (even_address - start) + thumb
    for start, end, target in _FLASH_MATCHED_RANGES:
        if start <= even_address < end:
            return target + (even_address - start) + thumb
    return address


# These objects are intentionally allowed to move in source builds.  Factory
# traces and retained SRAM can nevertheless contain their addresses, so
# differential tests compare the object-relative address instead of requiring
# the source linker to reproduce the factory Flash layout.
_RELOCATED_FLASH_OBJECTS = (
    (0x00020368, "runtime_main_entry", True),
    (0x00021CC8, "board_mcan_init_classic", True),
    (0x00021FC4, "board_mcan_init_fd", True),
    (0x00022244, "IRQ004_Handler", True),
    (0x00027CA0, "temperature_celsius_table", False),
    (0x00028668, "c_locale_name", False),
    (0x00028670, "c_locale", False),
)


def elf_symbol_sizes(elf):
    """Return non-zero symbol extents used by Flash-pointer normalization."""
    return {
        symbol.name: max(symbol["st_size"], 1)
        for symbol in elf.get_section_by_name(".symtab").iter_symbols()
        if symbol.name
    }


def normalize_source_flash_value(value, symbols, symbol_sizes):
    """Translate a movable source Flash pointer to its factory counterpart."""
    for factory_address, name, is_function in _RELOCATED_FLASH_OBJECTS:
        if name not in symbols:
            continue
        source_address = symbols[name]
        size = symbol_sizes.get(name, 1)
        candidate = value & ~1 if is_function else value
        if source_address <= candidate < source_address + size:
            offset = candidate - source_address
            thumb = value & 1 if is_function else 0
            return F(factory_address) + offset + thumb
    return value


def normalize_source_flash_words(data, symbols, symbol_sizes):
    """Normalize aligned 32-bit Flash pointers embedded in a byte snapshot."""
    normalized = bytearray(data)
    for offset in range(0, len(normalized) - 3, 4):
        value = int.from_bytes(normalized[offset:offset + 4], "little")
        value = normalize_source_flash_value(value, symbols, symbol_sizes)
        normalized[offset:offset + 4] = value.to_bytes(4, "little")
    return bytes(normalized)


FACTORY_FIXED_SOURCE = {
    "dm10010": 0x8680,
    "dm3507": 0x8678,
    "dm3507_48v": 0x8680,
    "dm4310": 0x8680,
    "dm4310_48v": 0x8688,
    "dm4340": 0x8680,
    "dm4340_48v": 0x8688,
    "dm8006": 0x8680,
    "dm8009": 0x8680,
}[MODEL]
FIXED_IMAGE_BASE = 0x1FFF8000
FACTORY_FIXED_SIZE = {
    "dm10010": 0x2514,
    "dm3507": 0x2510,
    "dm3507_48v": 0x2514,
    "dm4310": 0x2510,
    "dm4310_48v": 0x2514,
    "dm4340": 0x2510,
    "dm4340_48v": 0x2514,
    "dm8006": 0x2548,
    "dm8009": 0x2548,
}[MODEL]
FACTORY_STATE_SOURCE = FACTORY_FIXED_SOURCE + FACTORY_FIXED_SIZE
FACTORY_STATE_SIZE = {
    "dm10010": 0x2268,
    "dm3507": 0x2268,
    "dm3507_48v": 0x2268,
    "dm4310": 0x2268,
    "dm4310_48v": 0x2268,
    "dm4340": 0x2268,
    "dm4340_48v": 0x2268,
    "dm8006": 0x2268,
    "dm8009": 0x2268,
}[MODEL]
FACTORY_STATE_BASE = {
    "dm10010": 0x1FFFA514,
    "dm3507": 0x1FFFA510,
    "dm3507_48v": 0x1FFFA514,
    "dm4310": 0x1FFFA510,
    "dm4310_48v": 0x1FFFA514,
    "dm4340": 0x1FFFA510,
    "dm4340_48v": 0x1FFFA514,
    "dm8006": 0x1FFFA548,
    "dm8009": 0x1FFFA548,
}[MODEL]
FACTORY_STACK_TOP = {
    "dm10010": 0x20000500,
    "dm3507": 0x200004F8,
    "dm3507_48v": 0x20000500,
    "dm4310": 0x200004F8,
    "dm4310_48v": 0x20000500,
    "dm4340": 0x200004F8,
    "dm4340_48v": 0x20000500,
    "dm8006": 0x20000530,
    "dm8009": 0x20000530,
}[MODEL]

BUS_OVERVOLTAGE_BITS = {
    "dm10010": 0x42820000,
    "dm3507": 0x42000000,
    "dm3507_48v": 0x42820000,
    "dm4310": 0x42000000,
    "dm4310_48v": 0x42820000,
    "dm4340": 0x42000000,
    "dm4340_48v": 0x42820000,
    "dm8006": 0x42820000,
    "dm8009": 0x42820000,
}[MODEL]
