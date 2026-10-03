#!/usr/bin/env python3
"""Verify each recovered V3 source startup against its factory image."""

from io import BytesIO
from pathlib import Path

from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_HOOK_CODE, UC_MODE_THUMB
import unicorn.arm_const as arm


ROOT = Path(__file__).resolve().parents[2]
DM43XX_LITERALS = (
    (0x1FFF8630, 16), (0x1FFF8724, 32), (0x1FFF9830, 76),
    (0x1FFF9934, 28), (0x1FFF9AC0, 12), (0x1FFF9C34, 12),
    (0x1FFF9D4C, 16), (0x1FFFA118, 72), (0x1FFFA300, 12),
    (0x1FFFA4BC, 32),
)
DM43XX_LITERAL_RELOCATIONS = {
    0x1FFF984C: "board_mcan_init_classic",
    0x1FFF9850: "board_mcan_init_fd",
}
DM8009_LITERALS = (
    (0x1FFF80E4, 32), (0x1FFF91F0, 76), (0x1FFF92F8, 28),
    (0x1FFF9978, 16), (0x1FFF9AF8, 12), (0x1FFF9C6C, 12),
    (0x1FFF9D84, 16), (0x1FFFA150, 72), (0x1FFFA338, 12),
    (0x1FFFA4F4, 32),
)
DM8009_LITERAL_RELOCATIONS = {
    0x1FFF9210: "board_mcan_init_classic",
    0x1FFF9214: "board_mcan_init_fd",
}
MODELS = {
    "dm4310": {
        "factory": ROOT / "reference/V3/APP_DM4310(V3)_V5017_04.decrypted.bin",
        "main": 0x000252F4, "state": 0x1FFFA510,
        "config": 0x1FFFA5C8, "context": 0x1FFFF490,
        "heap": 0x1FFFF4F8, "sp": 0x200004F8,
        "literals": DM43XX_LITERALS,
        "alignment": (0x1FFF982E, 0x1FFF9932, 0x1FFF9C32,
                      0x1FFFA2FE, 0x1FFFA4BA, 0x1FFFA50E),
        "literal_relocations": DM43XX_LITERAL_RELOCATIONS,
    },
    "dm4340": {
        "factory": ROOT / "reference/V3/APP_DM4340(V3)_V5117_04.decrypted.bin",
        "main": 0x000252F4, "state": 0x1FFFA510,
        "config": 0x1FFFA5C8, "context": 0x1FFFF490,
        "heap": 0x1FFFF4F8, "sp": 0x200004F8,
        "literals": DM43XX_LITERALS,
        "alignment": (0x1FFF982E, 0x1FFF9932, 0x1FFF9C32,
                      0x1FFFA2FE, 0x1FFFA4BA, 0x1FFFA50E),
        "literal_relocations": DM43XX_LITERAL_RELOCATIONS,
    },
    "dm8009": {
        "factory": ROOT / "reference/V3/APP_DM8009(V3)_V6417_04.decrypted.bin",
        "main": 0x000252F0, "state": 0x1FFFA548,
        "config": 0x1FFFA558, "context": 0x1FFFF4C8,
        "heap": 0x1FFFF530, "sp": 0x20000530,
        "literals": DM8009_LITERALS,
        "alignment": (0x1FFF91EE, 0x1FFF92F6, 0x1FFF9C6A,
                      0x1FFFA336, 0x1FFFA4F2, 0x1FFFA546),
        "literal_relocations": DM8009_LITERAL_RELOCATIONS,
    },
}
FLASH_BASE = 0x00020000
FLASH_SIZE = 0x00020000
SRAM_A_BASE = 0x1FFF0000
SRAM_A_SIZE = 0x00010000
SRAM_B_BASE = 0x20000000
SRAM_B_SIZE = 0x00010000
FACTORY_SCATTER_ENTRY = 0x00020259
FACTORY_RUNTIME_ENTRY = 0x00020368
INITIALIZED_STATE_SIZE = 0x2268
CONFIG_STAGING_SIZE = 0x94
MARKER = 0xA5


def make_machine():
    machine = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    machine.mem_map(FLASH_BASE, FLASH_SIZE)
    machine.mem_map(SRAM_A_BASE, SRAM_A_SIZE)
    machine.mem_map(SRAM_B_BASE, SRAM_B_SIZE)
    machine.mem_write(SRAM_A_BASE, bytes([MARKER]) * SRAM_A_SIZE)
    machine.mem_write(SRAM_B_BASE, bytes([MARKER]) * SRAM_B_SIZE)
    machine.reg_write(arm.UC_ARM_REG_C1_C0_2, 0x00F00000)
    machine.reg_write(arm.UC_ARM_REG_FPEXC, 0x40000000)
    machine.reg_write(arm.UC_ARM_REG_SP, 0x2000F000)
    machine.reg_write(arm.UC_ARM_REG_LR, 0x00030001)
    return machine


def load_elf(path):
    elf = ELFFile(BytesIO(path.read_bytes()))
    symbols = {
        symbol.name: symbol["st_value"] & ~1
        for symbol in elf.get_section_by_name(".symtab").iter_symbols()
    }
    segments = [
        (segment["p_paddr"], segment.data())
        for segment in elf.iter_segments()
        if segment["p_type"] == "PT_LOAD" and segment["p_filesz"]
    ]
    return symbols, segments


def run_factory(factory):
    machine = make_machine()
    machine.mem_write(FLASH_BASE, factory)
    machine.emu_start(
        FACTORY_SCATTER_ENTRY, FACTORY_RUNTIME_ENTRY, count=2_000_000
    )
    assert machine.reg_read(arm.UC_ARM_REG_PC) == FACTORY_RUNTIME_ENTRY
    return machine


def run_source(symbols, segments):
    machine = make_machine()
    for address, data in segments:
        machine.mem_write(address, data)

    def skip_system_init(emu, address, _size, _data):
        if address == symbols["SystemInit"]:
            emu.reg_write(arm.UC_ARM_REG_PC, emu.reg_read(arm.UC_ARM_REG_LR))

    machine.hook_add(UC_HOOK_CODE, skip_system_init)
    machine.emu_start(
        symbols["Reset_Handler"] | 1,
        symbols["runtime_main_entry"],
        count=2_000_000,
    )
    assert (
        machine.reg_read(arm.UC_ARM_REG_PC)
        == symbols["runtime_main_entry"]
    )
    return machine


def first_differences(left, right, base, limit=16):
    return [
        (hex(base + offset), left_byte, right_byte)
        for offset, (left_byte, right_byte) in enumerate(zip(left, right))
        if left_byte != right_byte
    ][:limit]


def assert_range_equal(factory_machine, source_machine, address, size, label):
    factory_data = bytes(factory_machine.mem_read(address, size))
    source_data = bytes(source_machine.mem_read(address, size))
    assert factory_data == source_data, (
        label,
        first_differences(factory_data, source_data, address),
    )


def assert_runtime_context_equal(factory_machine, source_machine, address, symbols):
    factory_data = bytearray(factory_machine.mem_read(address, 0x60))
    source_data = bytes(source_machine.mem_read(address, 0x60))
    relocations = {
        # These pointers follow the rebuilt image's linker layout. All other
        # runtime-context bytes remain byte-exact against the factory state.
        0x2C: symbols["c_locale"],
        0x5C: (symbols["runtime_main_entry"] + 4) | 1,
    }
    for offset, value in relocations.items():
        factory_data[offset:offset + 4] = value.to_bytes(4, "little")
    assert bytes(factory_data) == source_data, (
        "runtime context",
        first_differences(factory_data, source_data, address),
    )


def verify_model(model, profile):
    factory = profile["factory"].read_bytes()
    symbols, segments = load_elf(ROOT / f"build/{model}.elf")
    factory_machine = run_factory(factory)
    source_machine = run_source(symbols, segments)

    assert_range_equal(
        factory_machine,
        source_machine,
        profile["config"],
        CONFIG_STAGING_SIZE,
        "configuration staging",
    )
    assert_range_equal(
        factory_machine,
        source_machine,
        profile["state"],
        INITIALIZED_STATE_SIZE,
        "initialized scatter state",
    )

    for address, size in profile["literals"]:
        offset = 0x8680 + address - 0x1FFF8000
        source_data = bytes(source_machine.mem_read(address, size))
        expected_data = bytearray(factory[offset : offset + size])
        # Preserve strict literal-pool parity while allowing normal linker
        # movement: pointer fields must resolve to the equivalent source
        # symbol, rather than retain the factory function's Flash address.
        for pointer_address, symbol_name in profile["literal_relocations"].items():
            if address <= pointer_address < address + size:
                pointer_offset = pointer_address - address
                pointer = symbols[symbol_name] | 1
                expected_data[pointer_offset:pointer_offset + 4] = pointer.to_bytes(
                    4, "little"
                )
        assert source_data == expected_data, (
            hex(address),
            first_differences(expected_data, source_data, address),
        )
    for address in profile["alignment"]:
        assert bytes(source_machine.mem_read(address, 2)) == bytes(2), hex(address)

    factory_machine.emu_start(
        FACTORY_RUNTIME_ENTRY | 1, profile["main"], count=100_000
    )
    source_machine.emu_start(
        symbols["runtime_main_entry"] | 1,
        symbols["main"],
        count=100_000,
    )
    assert factory_machine.reg_read(arm.UC_ARM_REG_PC) == profile["main"]
    assert source_machine.reg_read(arm.UC_ARM_REG_PC) == symbols["main"]
    assert_runtime_context_equal(
        factory_machine, source_machine, profile["context"], symbols
    )
    assert_range_equal(
        factory_machine,
        source_machine,
        profile["heap"],
        0x400,
        "runtime heap",
    )
    assert factory_machine.reg_read(arm.UC_ARM_REG_SP) == profile["sp"]
    assert source_machine.reg_read(arm.UC_ARM_REG_SP) == profile["sp"]
    print(
        f"PASS {model}: factory/source config, complete initialized state, "
        "fixed literals, runtime context, heap and main-entry SP match"
    )


def main():
    for model, profile in MODELS.items():
        verify_model(model, profile)


if __name__ == "__main__":
    main()
