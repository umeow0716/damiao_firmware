#!/usr/bin/env python3
"""Audit a final DM43xx ELF/load image without joining normal CMake."""

from argparse import ArgumentParser
from hashlib import sha256
from io import BytesIO
from pathlib import Path

from elftools.elf.constants import SH_FLAGS
from elftools.elf.elffile import ELFFile


FLASH_START = 0x00020000
FLASH_END = 0x00030000
RAM_START = 0x1FFF8000
RAM_END = 0x20008000
RAMB_START = 0x200F0000
RAMB_END = 0x200F1000

MODELS = (
    "dm10010",
    "dm3507",
    "dm3507_48v",
    "dm4310",
    "dm4310_48v",
    "dm4340",
    "dm4340_48v",
    "dm8006",
    "dm8009",
)

FIXED_SECTION_ADDRESSES = {
    "dm4310": {
        ".dm4310_helper_fault_monitor": 0x1FFF8000,
        ".dm4310_irq002": 0x1FFF8136,
        ".dm4310_boot_record_writer": 0x1FFF8640,
        ".dm4310_irq000": 0x1FFF8744,
        ".dm4310_irq001": 0x1FFF8770,
        ".dm4310_irq003": 0x1FFF88B8,
        ".dm4310_helper_output_sensor": 0x1FFF987C,
        ".dm4310_flash_writer": 0x1FFF9950,
        ".dm4310_flash_erase_only": 0x1FFF9A38,
        ".dm4310_helper_mcan_send_classic": 0x1FFF9ACC,
        ".dm4310_helper_mcan_send_variable_fd": 0x1FFF9B0E,
        ".dm4310_helper_mcan_send_fd": 0x1FFF9BA0,
        ".dm4310_helper_svpwm": 0x1FFF9C40,
        ".dm4310_helper_clear_runtime_loop_states": 0x1FFF9D5C,
        ".dm4310_helper_reset_control_state": 0x1FFF9DC8,
        ".dm4310_helper_identification_filter": 0x1FFF9E44,
        ".dm4310_helper_derive_runtime_controller_states": 0x1FFF9EA6,
        ".dm4310_helper_motion_observer": 0x1FFF9FDE,
        ".dm4310_helper_current_controller": 0x1FFFA07A,
        ".dm4310_helper_rls2": 0x1FFFA160,
        ".dm4310_helper_flux_observer": 0x1FFFA234,
        ".dm4310_helper_sincos": 0x1FFFA30C,
        ".dm4310_helper_clamp": 0x1FFFA38A,
        ".dm4310_helper_wrap": 0x1FFFA3AA,
        ".dm4310_helper_limit_vector": 0x1FFFA414,
        ".dm4310_helper_float_to_uint": 0x1FFFA46A,
        ".dm4310_helper_uint_to_float": 0x1FFFA492,
        ".dm4310_helper_derive_control_parameters": 0x1FFFA4DC,
        ".dm4310_helper_delay_ms": 0x1FFFA4E6,
        ".dm4310_helper_select_configuration_bank_b": 0x1FFFA4F0,
        ".dm4310_helper_atan2": 0x1FFFA4FA,
        ".dm4310_helper_sqrt": 0x1FFFA504,
    },
    "dm8009": {
        ".dm4310_boot_record_writer": 0x1FFF8000,
        ".dm4310_irq000": 0x1FFF8104,
        ".dm4310_irq001": 0x1FFF8130,
        ".dm4310_irq003": 0x1FFF8278,
        ".dm4310_helper_output_sensor": 0x1FFF9240,
        ".dm4310_helper_fault_monitor": 0x1FFF9314,
        ".dm4310_irq002": 0x1FFF944A,
        ".dm4310_flash_writer": 0x1FFF9988,
        ".dm4310_flash_erase_only": 0x1FFF9A70,
        ".dm4310_helper_mcan_send_classic": 0x1FFF9B04,
        ".dm4310_helper_mcan_send_variable_fd": 0x1FFF9B46,
        ".dm4310_helper_mcan_send_fd": 0x1FFF9BD8,
        ".dm4310_helper_svpwm": 0x1FFF9C78,
        ".dm4310_helper_clear_runtime_loop_states": 0x1FFF9D94,
        ".dm4310_helper_reset_control_state": 0x1FFF9E00,
        ".dm4310_helper_identification_filter": 0x1FFF9E7C,
        ".dm4310_helper_derive_runtime_controller_states": 0x1FFF9EDE,
        ".dm4310_helper_motion_observer": 0x1FFFA016,
        ".dm4310_helper_current_controller": 0x1FFFA0B2,
        ".dm4310_helper_rls2": 0x1FFFA198,
        ".dm4310_helper_flux_observer": 0x1FFFA26C,
        ".dm4310_helper_sincos": 0x1FFFA344,
        ".dm4310_helper_clamp": 0x1FFFA3C2,
        ".dm4310_helper_wrap": 0x1FFFA3E2,
        ".dm4310_helper_limit_vector": 0x1FFFA44C,
        ".dm4310_helper_float_to_uint": 0x1FFFA4A2,
        ".dm4310_helper_uint_to_float": 0x1FFFA4CA,
        ".dm4310_helper_derive_control_parameters": 0x1FFFA514,
        ".dm4310_helper_delay_ms": 0x1FFFA51E,
        ".dm4310_helper_select_configuration_bank_b": 0x1FFFA528,
        ".dm4310_helper_atan2": 0x1FFFA532,
        ".dm4310_helper_sqrt": 0x1FFFA53C,
    },
}
FIXED_SECTION_ADDRESSES["dm4340"] = FIXED_SECTION_ADDRESSES["dm4310"]
FIXED_SECTION_ADDRESSES["dm3507"] = FIXED_SECTION_ADDRESSES["dm4310"]

# The 48 V DM43-family image adds one 32-bit startup-threshold literal at
# 0x1fff9830.  Every fixed object at or above the former output helper moves by
# exactly four bytes; objects below that point keep the standard layout.
FIXED_SECTION_ADDRESSES["dm43_48v"] = {
    name: address + (4 if address >= 0x1FFF987C else 0)
    for name, address in FIXED_SECTION_ADDRESSES["dm4310"].items()
}
for _model in ("dm10010", "dm3507_48v", "dm4310_48v", "dm4340_48v"):
    FIXED_SECTION_ADDRESSES[_model] = FIXED_SECTION_ADDRESSES["dm43_48v"]

STATE_LAYOUTS = {
    "dm4310": {
        "state_base": 0x1FFFA510,
        "stack_top": 0x200004F8,
        "sections": {
            ".dm4310_runtime_drive_d": (0x1FFFA510, 0x28),
            ".dm4310_runtime_drive_q": (0x1FFFA538, 0x28),
            ".dm4310_runtime_speed_loop": (0x1FFFA568, 0x28),
            ".dm4310_runtime_position_loop": (0x1FFFA590, 0x28),
            ".dm4310_uart_length_state": (0x1FFFA5B8, 0x4),
            ".dm4310_hardware_variant": (0x1FFFA5BC, 0x1),
            ".dm4310_zero_position_staging": (0x1FFFA5C0, 0x8),
            ".dm4310_config_staging": (0x1FFFA5C8, 0x94),
            ".dm4310_position_sensor_expected": (0x1FFFA65C, 0xC),
            ".dm4310_sine_table": (0x1FFFA674, 0x2004),
            ".dm4310_aes_sbox": (0x1FFFC678, 0x100),
            ".dm4310_adc_raw": (0x1FFFC778, 0x14),
            ".dm4310_current_d": (0x1FFFC78C, 0x4C),
            ".dm4310_current_q": (0x1FFFC7D8, 0x4C),
            ".dm4310_boot_record_staging": (0x1FFFC824, 0x14),
            ".dm4310_short_response_buffer": (0x1FFFC838, 0x14),
            ".dm4310_motor_correction": (0x1FFFC84C, 0x400),
            ".dm4310_can_protocol_scratch": (0x1FFFCC4C, 0x20),
            ".dm4310_motor_record": (0x1FFFCC6C, 0x40C),
            ".dm4310_output_table": (0x1FFFD078, 0x2000),
            ".dm4310_output_calibration": (0x1FFFF078, 0x10),
            ".dm4310_motor_runtime_state": (0x1FFFF088, 0x7C),
            ".dm4310_sample_runtime_state": (0x1FFFF104, 0xA4),
            ".dm4310_output_sensor_state": (0x1FFFF1A8, 0x48),
            ".dm4310_runtime_status": (0x1FFFF1F0, 0x4C),
            ".dm4310_runtime_parameter_cache": (0x1FFFF23C, 0x60),
            ".dm4310_mcan_dispatch": (0x1FFFF29C, 0xA4),
            ".dm4310_calibration_frame_scratch": (0x1FFFF340, 0x40),
            ".dm4310_motion_observer": (0x1FFFF380, 0x3C),
            ".dm4310_position_sensor_observed": (0x1FFFF3BC, 0xB),
            ".dm4310_uart_rx_buffer": (0x1FFFF3C7, 0xC8),
            ".dm4310_c_runtime_state": (0x1FFFF490, 0x60),
            ".dm4310_system_core_clock": (0x1FFFF4F0, 0x4),
            ".dm4310_hrc_value": (0x1FFFF4F4, 0x4),
        },
    },
    "dm8009": {
        "state_base": 0x1FFFA548,
        "stack_top": 0x20000530,
        "sections": {
            ".dm4310_uart_length_state": (0x1FFFA548, 0x4),
            ".dm4310_hardware_variant": (0x1FFFA54C, 0x1),
            ".dm4310_zero_position_staging": (0x1FFFA550, 0x8),
            ".dm4310_config_staging": (0x1FFFA558, 0x94),
            ".dm4310_position_sensor_expected": (0x1FFFA5EC, 0xB),
            ".dm8009_runtime_drive_d": (0x1FFFA5F8, 0x28),
            ".dm8009_runtime_drive_q": (0x1FFFA620, 0x28),
            ".dm8009_runtime_speed_loop": (0x1FFFA650, 0x28),
            ".dm8009_runtime_position_loop": (0x1FFFA678, 0x28),
            ".dm4310_sine_table": (0x1FFFA6AC, 0x2004),
            ".dm4310_aes_sbox": (0x1FFFC6B0, 0x100),
            ".dm4310_boot_record_staging": (0x1FFFC7B0, 0x14),
            ".dm4310_short_response_buffer": (0x1FFFC7C4, 0x14),
            ".dm4310_motor_correction": (0x1FFFC7D8, 0x400),
            ".dm4310_can_protocol_scratch": (0x1FFFCBD8, 0x20),
            ".dm4310_motor_record": (0x1FFFCBF8, 0x40C),
            ".dm4310_output_table": (0x1FFFD004, 0x2000),
            ".dm4310_output_calibration": (0x1FFFF004, 0x10),
            ".dm4310_motor_runtime_state": (0x1FFFF014, 0x7C),
            ".dm4310_sample_runtime_state": (0x1FFFF090, 0xA4),
            ".dm4310_output_sensor_state": (0x1FFFF134, 0x48),
            ".dm4310_runtime_status": (0x1FFFF17C, 0x4C),
            ".dm4310_runtime_parameter_cache": (0x1FFFF1C8, 0x60),
            ".dm4310_mcan_dispatch": (0x1FFFF228, 0xA4),
            ".dm4310_calibration_frame_scratch": (0x1FFFF2CC, 0x40),
            ".dm4310_motion_observer": (0x1FFFF30C, 0x3C),
            ".dm8009_adc_raw": (0x1FFFF348, 0x14),
            ".dm8009_current_d": (0x1FFFF35C, 0x4C),
            ".dm8009_current_q": (0x1FFFF3A8, 0x4C),
            ".dm4310_position_sensor_observed": (0x1FFFF3F4, 0xB),
            ".dm4310_uart_rx_buffer": (0x1FFFF3FF, 0xC8),
            ".dm4310_c_runtime_state": (0x1FFFF4C8, 0x60),
            ".dm4310_system_core_clock": (0x1FFFF528, 0x4),
            ".dm4310_hrc_value": (0x1FFFF52C, 0x4),
        },
    },
}
STATE_LAYOUTS["dm4340"] = STATE_LAYOUTS["dm4310"]
STATE_LAYOUTS["dm3507"] = STATE_LAYOUTS["dm4310"]
STATE_LAYOUTS["dm43_48v"] = {
    "state_base": STATE_LAYOUTS["dm4310"]["state_base"] + 4,
    "stack_top": STATE_LAYOUTS["dm4310"]["stack_top"] + 8,
    "sections": {
        name: (address + 4, size)
        for name, (address, size) in
        STATE_LAYOUTS["dm4310"]["sections"].items()
    },
}
for _model in ("dm10010", "dm3507_48v", "dm4310_48v", "dm4340_48v"):
    STATE_LAYOUTS[_model] = STATE_LAYOUTS["dm43_48v"]
FIXED_SECTION_ADDRESSES["dm8006"] = FIXED_SECTION_ADDRESSES["dm8009"]
STATE_LAYOUTS["dm8006"] = STATE_LAYOUTS["dm8009"]


def inside(start, end, region_start, region_end):
    return region_start <= start <= end <= region_end


def symbol_values(elf):
    table = elf.get_section_by_name(".symtab")
    assert table is not None
    return {entry.name: entry["st_value"] for entry in table.iter_symbols()}


def allocated_sections(elf):
    return [section for section in elf.iter_sections()
            if section["sh_size"] and
            section["sh_flags"] & SH_FLAGS.SHF_ALLOC]


def require_section(elf, name):
    section = elf.get_section_by_name(name)
    assert section is not None, f"missing allocated section {name}"
    return section


def assert_non_overlapping(ranges, label):
    ordered = sorted(ranges)
    for left, right in zip(ordered, ordered[1:]):
        assert left[1] <= right[0], (
            f"{label} overlap", left, right)


def audit(elf_path, binary_path, model):
    elf_data = elf_path.read_bytes()
    elf = ELFFile(BytesIO(elf_data))
    symbols = symbol_values(elf)
    sections = allocated_sections(elf)

    vma_ranges = []
    for section in sections:
        start = section["sh_addr"]
        end = start + section["sh_size"]
        assert (inside(start, end, FLASH_START, FLASH_END) or
                inside(start, end, RAM_START, RAM_END) or
                inside(start, end, RAMB_START, RAMB_END)), (
                    section.name, hex(start), hex(end))
        vma_ranges.append((start, end, section.name))
    assert_non_overlapping(vma_ranges, "allocated VMA")

    vectors = elf.get_section_by_name(".vectors")
    assert vectors is not None
    assert vectors["sh_addr"] == FLASH_START
    assert vectors["sh_size"] == 0x240

    load_segments = [segment for segment in elf.iter_segments()
                     if segment["p_type"] == "PT_LOAD" and
                     segment["p_filesz"]]
    lma_ranges = []
    for segment in load_segments:
        start = segment["p_paddr"]
        end = start + segment["p_filesz"]
        assert inside(start, end, FLASH_START, FLASH_END), (
            "load LMA outside APP", hex(start), hex(end))
        lma_ranges.append((start, end, hex(segment["p_vaddr"])))
    assert_non_overlapping(lma_ranges, "load LMA")

    image_end = max(end for _, end, _ in lma_ranges)
    image_size = image_end - FLASH_START
    assert image_size <= FLASH_END - FLASH_START
    assert symbols["__dm4310_ram_lma"] == symbols["__etext_ramb"]
    ramb_data_size = (symbols["__data_end_ramb__"] -
                      symbols["__data_start_ramb__"])
    assert symbols["__etext_ramb"] + ramb_data_size == image_end

    reconstructed = bytearray(image_size)
    for segment in load_segments:
        offset = segment["p_paddr"] - FLASH_START
        data = segment.data()
        reconstructed[offset:offset + len(data)] = data
    binary = binary_path.read_bytes()
    assert len(binary) == image_size, (len(binary), image_size)
    assert binary == reconstructed

    # Every initialized RAM section must have a file-backed LOAD segment;
    # every zero-filled section must lie in a Reset-cleared range.
    fixed_copy_sections = []
    zero_sections = []
    for section in sections:
        start = section["sh_addr"]
        end = start + section["sh_size"]
        if inside(start, end, FLASH_START, FLASH_END):
            continue
        if section["sh_type"] == "SHT_NOBITS":
            zero_sections.append((start, end, section.name))
            continue
        owners = [segment for segment in load_segments
                  if segment["p_vaddr"] <= start and
                  end <= segment["p_vaddr"] + segment["p_filesz"]]
        assert len(owners) == 1, (section.name, len(owners))
        fixed_copy_sections.append((start, end, section.name))

    bss_start = symbols["__bss_start__"]
    bss_end = symbols["__bss_end__"]
    ramb_bss_start = symbols["__bss_start_ramb__"]
    ramb_bss_end = symbols["__bss_end_ramb__"]
    for start, end, name in zero_sections:
        assert (inside(start, end, bss_start, bss_end) or
                inside(start, end, ramb_bss_start, ramb_bss_end)), (
                    "NOBITS outside Reset clear", name, hex(start), hex(end))

    assert symbols["__factory_bss_end__"] == symbols["__HeapBase"]
    assert symbols["__HeapBase"] < symbols["__HeapLimit"]
    assert symbols["__HeapLimit"] == symbols["__StackLimit"]
    assert symbols["__StackLimit"] < symbols["__StackTop"]
    for start, end, name in vma_ranges:
        assert end <= symbols["__HeapBase"] or start >= symbols["__StackTop"], (
            "section overlaps factory heap/stack", name, hex(start), hex(end))

    layout = STATE_LAYOUTS[model]
    state_base = layout["state_base"]
    helper_sections = [(start, end, name)
                       for start, end, name in fixed_copy_sections
                       if start < state_base]
    assert helper_sections
    assert all(RAM_START <= start < end <= state_base
               for start, end, _ in helper_sections)

    for name, expected_address in FIXED_SECTION_ADDRESSES[model].items():
        section = require_section(elf, name)
        assert section["sh_addr"] == expected_address, (
            name, hex(section["sh_addr"]), hex(expected_address))

    for name, (expected_address, expected_size) in layout["sections"].items():
        section = require_section(elf, name)
        assert section["sh_addr"] == expected_address, (
            name, hex(section["sh_addr"]), hex(expected_address))
        assert section["sh_size"] == expected_size, (
            name, hex(section["sh_size"]), hex(expected_size))

    if model in ("dm8006", "dm8009"):
        expected_dma_word = 0x1FFFA6A0
    elif model in ("dm10010", "dm3507_48v", "dm4310_48v", "dm4340_48v"):
        expected_dma_word = 0x1FFFA66A
    else:
        expected_dma_word = 0x1FFFA666
    assert symbols["position_dma_word"] == expected_dma_word
    assert symbols["__StackTop"] == layout["stack_top"]
    assert require_section(elf, ".dm4310_app_state")["sh_type"] == "SHT_NOBITS"

    print(
        f"PASS: {model.upper()} image layout; "
        f"{image_size} bytes, {FLASH_END - image_end} bytes free, "
        f"{len(load_segments)} file-backed LOAD segments, "
        f"{len(fixed_copy_sections)} initialized RAM sections, "
        f"{len(zero_sections)} zero-fill sections, no VMA/LMA overlap; "
        f"SHA-256 {sha256(binary).hexdigest()}"
    )


def main():
    parser = ArgumentParser()
    parser.add_argument("--model", choices=MODELS,
                        default="dm4310")
    parser.add_argument("--elf", type=Path)
    parser.add_argument("--bin", type=Path)
    args = parser.parse_args()
    elf_path = args.elf or Path(f"build/{args.model}.elf")
    binary_path = args.bin or Path(f"build/{args.model}.app.bin")
    audit(elf_path, binary_path, args.model)


if __name__ == "__main__":
    main()
