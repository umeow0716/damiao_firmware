"""Shared Unicorn loader for isolated DM4310 factory/source comparisons."""

from io import BytesIO
import os
from pathlib import Path

from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB

from regressions.dm4310_model_layout import (
    A,
    FACTORY_FIXED_SOURCE,
    FACTORY_STATE_BASE,
    FACTORY_STATE_SOURCE,
    FIXED_IMAGE_BASE,
    elf_symbol_sizes,
)


ROOT = Path(__file__).resolve().parents[2]
MODEL = os.environ.get("DAMIAO_RECOVERY_MODEL", "dm4310").lower()
FACTORY_PATHS = {
    "dm4310": ROOT / "reference/APP_DM4310_V3_V5017_04.decrypted.bin",
    "dm4340": ROOT / "reference/APP_DM4340_V3_V5117_04_decrypted.bin",
    "dm8009": ROOT / "reference/APP_DM8009_V3_V6417_04_decrypted.bin",
}
if MODEL not in FACTORY_PATHS:
    raise ValueError(f"unsupported DAMIAO_RECOVERY_MODEL: {MODEL}")
FACTORY_PATH = FACTORY_PATHS[MODEL]
DEFAULT_ELF_PATH = ROOT / f"build/{MODEL}.elf"


def load_images(elf_path=DEFAULT_ELF_PATH):
    factory = FACTORY_PATH.read_bytes()
    elf = ELFFile(BytesIO(Path(elf_path).read_bytes()))
    symbols = {
        symbol.name: symbol["st_value"] & ~1
        for symbol in elf.get_section_by_name(".symtab").iter_symbols()
    }
    segments = [
        (segment["p_paddr"], segment.data())
        for segment in elf.iter_segments()
        if segment["p_type"] == "PT_LOAD" and segment["p_filesz"]
    ]
    return factory, symbols, segments


def load_symbol_sizes(elf_path=DEFAULT_ELF_PATH):
    """Load source symbol extents without changing the loader's public tuple."""
    elf = ELFFile(BytesIO(Path(elf_path).read_bytes()))
    return elf_symbol_sizes(elf)


def make_machine(original, factory, segments):
    machine = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    machine.mem_map(0x00020000, 0x00020000)
    machine.mem_map(0x1FFF0000, 0x00010000)
    machine.mem_map(0x20000000, 0x00010000)
    if original:
        machine.mem_write(0x00020000, factory[:0x8680])
    else:
        for address, data in segments:
            machine.mem_write(address, data)
    return machine


def load_fixed_sram_runtime(machine, original, factory,
                            elf_path=DEFAULT_ELF_PATH):
    """Install the factory copy-down image or source SRAM sections in place."""
    if original:
        machine.mem_write(
            FIXED_IMAGE_BASE,
            factory[FACTORY_FIXED_SOURCE:FACTORY_STATE_SOURCE],
        )
        return

    elf = ELFFile(BytesIO(Path(elf_path).read_bytes()))
    for section in elf.iter_sections():
        address = section["sh_addr"]
        if (FIXED_IMAGE_BASE <= address < FACTORY_STATE_BASE and
                section["sh_type"] != "SHT_NOBITS"):
            machine.mem_write(address, section.data())
