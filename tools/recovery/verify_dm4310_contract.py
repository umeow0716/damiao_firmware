#!/usr/bin/env python3
"""Verify recovered DM4310 facts without joining the normal build flow."""

from __future__ import annotations

import argparse
import hashlib
import pathlib
import re
import struct
import subprocess
import sys
import tempfile


FACTORY_BASE = 0x00020000
FACTORY_SHA256 = "65aab219268e9159b196d4578d3cd530e6fa90a731a82670a0d3d3be609b59d4"

# Values read directly from the V5017 factory APP literal pools.
FACTORY_WORDS = {
    0x00020000: 0x200004F8,  # initial MSP
    0x00020040: 0x1FFF8745,  # TMR4 IRQ RAM entry
    0x00020044: 0x1FFF8771,  # position DMA IRQ RAM entry
    0x00020048: 0x1FFF8137,  # ADC/FOC IRQ RAM entry
    0x0002004C: 0x1FFF88B9,  # MCAN IRQ RAM entry
    0x00020050: 0x00022245,  # USART1/DMA parser IRQ flash entry
    0x000221D0: 0x26003A13,  # MCAN NBTP
    0x000221D4: 0x80030080,  # MCAN RXF0C
    0x000221D8: 0x00040308,  # MCAN TXEFC
    0x000221DC: 0x03000328,  # MCAN TXBC
    0x000221E4: 0x880000FF,  # standard-filter base
    0x000221E8: 0x8FFF07FF,  # standard-filter mask
    0x000221EC: 0x02800011,  # MCAN IE
    0x0002251C: 0x1FFFA5B8,  # UART expected/received length pair
    0x00022520: 0x1FFFF3C7,  # USART1 DMA RX buffer
    0x00022524: 0x1FFFF104,  # ADC/sample/control runtime state
    0x00022528: 0x1FFFF088,  # motor/control runtime state
    0x00022538: 0x1FFFA5C8,  # live 37-word motor configuration
    0x00022544: 0x1FFFCC6C,  # Ud staging base
    0x0002254C: 0x1FFFD078,  # UM/runtime output table base
    0x00022554: 0x1FFFF29C,  # Vector_20 classic/FD dispatch pair
    0x00022838: 0x1FFFC84C,  # motor correction runtime table
    0x00022848: 0x1FFFF1A8,  # output-sensor runtime state
    0x00022854: 0x1FFFD078,  # validated output correction table
    0x00024ADC: 0x1FFFF190,  # SPI position scratch subobject
    0x00022BCC: 0x1FFFF23C,  # runtime derived-parameter cache
    0x00022BB4: 0x1FFFF29C,  # startup classic/FD dispatch pair
    0x00028CB0: 0x1FFFF380,  # motion-observer state
    0x000230BC: 0x0330033F,  # TMR4 output-control image
    0x000250A8: 0x1FFFF1A8,  # output helper call-site state
    0x000250AC: 0x40040400,  # ADC2 base
    0x000250B0: 0x40020000,  # SPI3 base
    0x000250B4: 0x40040444,  # ADC2 ISR
    0x000250B8: 0x40040000,  # ADC1 base
    0x000250BC: 0x40040800,  # ADC3 base
    0x000250C0: 0x40040454,  # ADC2 DR2
    0x000250C4: 0x40040446,  # ADC2 ISCLRR
    0x000250C8: 0x40040846,  # ADC3 ISCLRR
    0x00025120: 0x1FFFF088,  # motor/control runtime state
    0x00025124: 0x1FFFF190,  # position DMA scratch
    0x0002512C: 0x40048000,  # PWC base
    0x00025134: 0x40010878,  # AOS ADC1 trigger selection
    0x0002569C: 0x0003E000,  # FCB destination configuration sector
    0x000256A0: 0x40040000,  # ADC1 base; ISCLRR at +0x46
    0x000256A4: 0x40040446,  # ADC2 ISCLRR
    0x000256A8: 0x40038000,  # TMR4_1 base; CCSR at +0x58
    0x000256C4: 0x1FFFF190,  # position DMA/control scratch base
    0x00026A00: 0x40029000,  # MCAN1 base used for live baud calculation
    0x00026A1C: 0x1FFFA5BC,  # fixed hardware-variant byte
    0x00026A94: 0x1FFFF104,  # current offsets in sample runtime state
    0x00026B4C: 0x1FFFA5C8,  # live persistent configuration staging
}

# Source images copied by the factory startup into executable SRAM, plus the
# adjacent sine/cosine lookup helper.  Pin complete bodies so later analysis
# cannot silently drift to a rebuilt image or to a misidentified boundary.
FACTORY_HELPERS = {
    # Complete output-sensor calibration outer flow through alignment.
    (0x000246F8, 0x0428):
        "9e41af9ee2e6ba42172102fda815a580b9643091a6717340b152fd51d35fb6f1",
    # Complete motor-identification outer flow through the next function.
    (0x000257C4, 0x0CE0):
        "cefdcfac070e2602783a014720b90b8f1aa60f18c9b9eea6be33f14a21e4562d",
    # Direction/pole-pair pre-worker through its following function boundary.
    (0x000264A4, 0x0244):
        "2c29856eebd2ac50466bc2f1819c02f505143a118e12e94fd013cbd39f99c117",
    # ARM runtime atan2f and checked-vsqrt bodies used by fixed SRAM veneers.
    (0x00023BE8, 0x01AC):
        "b4a58b506eed68a9d72cef0f4bd197af29e86c9d53b8013d90008073b87bc894",
    (0x00023FD0, 0x003A):
        "538937a1ee77a29403a41fe63873c8e64561bb1f9296f27ce74a473b78376936",
    # Deferred configuration writer and the two five-word boot selectors.
    (0x00026D00, 0x00CA):
        "b29d4866825810e3b9f19b513a96518f5b015acaf29fc5dfae57bf23d0ef0f65",
    (0x00026DE8, 0x0018):
        "e05b7cbc28ecec6b930a4428ca519bc3f3c15406f88cf0456b0cd07e69dc5fcc",
    (0x00026E04, 0x0018):
        "15efb744e5d235cccc2799a20f78ef7658066d953f97d77dbd1b5d429285c009",
    # The complete factory scatter-loaded executable image.  The source range
    # maps linearly to SRAM 0x1fff8000..0x1fffa50f.
    (0x00028680, 0x2510):
        "2c64ab2f521d423de50dc50fc1ca3fe879aa0c61c7d7d404a0b8ab256f1e1c1d",
    (0x0002A7E0, 0x00D4):
        "a7ead9c298b56691a18324f6981f4c01d6a0da760562450c2ce869601a51e732",
    (0x0002A8B4, 0x00C8):
        "2246aa7749105de953bc88e95261d4380ac4e76e9aa9f440683cddce40b5e825",
    (0x0002A98C, 0x007E):
        "65dd0ac5526a40220fe0b1d80196427f9d114bd561f87129d425dab6453305cc",
}

ELF_SYMBOLS = {
    "dm4310_fault_monitor_helper": (0x1FFF8000, 0x0004),
    "IRQ000_Handler": (0x1FFF8744, 0x0004),
    "IRQ001_Handler": (0x1FFF8770, 0x0004),
    "IRQ002_Handler": (0x1FFF8136, 0x0004),
    "write_boot_record_from_sram": (0x1FFF8640, 0x00D8),
    "IRQ003_Handler": (0x1FFF88B8, 0x0004),
    "erase_and_program_from_sram": (0x1FFF9950, 0x00D8),
    "board_flash_erase_sector_from_sram": (0x1FFF9A38, 0x0088),
    "clear_runtime_loop_states_helper": (0x1FFF9D5C, 0x0004),
    "derive_runtime_controller_states_helper": (0x1FFF9EA6, 0x000A),
    "dm4310_mcan_send_variable_fd_helper": (0x1FFF9B0E, 0x000A),
    "dm4310_mcan_send_classic_helper": (0x1FFF9ACC, 0x0004),
    "dm4310_mcan_send_fd_helper": (0x1FFF9BA0, 0x0004),
    "output_sensor_helper": (0x1FFF987C, 0x00B8),
    "svpwm_helper": (0x1FFF9C40, 0x0004),
    "reset_control_state_helper": (0x1FFF9DC8, 0x0004),
    "dm4310_motion_observer_helper": (0x1FFF9FDE, 0x009C),
    "dm4310_current_controller_helper": (0x1FFFA07A, 0x009E),
    "dm4310_identification_filter_helper": (0x1FFF9E44, 0x0062),
    "dm4310_rls2_helper": (0x1FFFA160, 0x00D4),
    "dm4310_flux_observer_helper": (0x1FFFA234, 0x00CA),
    "dm4310_sincos_helper": (0x1FFFA30C, 0x007E),
    "dm4310_clamp_helper": (0x1FFFA38A, 0x0020),
    "dm4310_wrap_helper": (0x1FFFA3AA, 0x0034),
    "wrap_angle_helper": (0x1FFFA3DE, 0x0036),
    "dm4310_limit_vector_helper": (0x1FFFA414, 0x0056),
    "dm4310_float_to_uint_helper": (0x1FFFA46A, 0x0028),
    "dm4310_uint_to_float_helper": (0x1FFFA492, 0x004A),
    "derive_control_parameters_helper": (0x1FFFA4DC, 0x000A),
    "dm4310_delay_ms_helper": (0x1FFFA4E6, 0x000A),
    "select_configuration_bank_b_helper": (0x1FFFA4F0, 0x000A),
    "dm4310_atan2_helper": (0x1FFFA4FA, 0x000A),
    "dm4310_sqrt_helper": (0x1FFFA504, 0x000A),
    "runtime_drive_d": (0x1FFFA510, 0x0028),
    "runtime_drive_q": (0x1FFFA538, 0x0028),
    "runtime_speed_loop": (0x1FFFA568, 0x0028),
    "runtime_position_loop": (0x1FFFA590, 0x0028),
    "uart_length_state": (0x1FFFA5B8, 0x0004),
    "cached_hardware_variant": (0x1FFFA5BC, 0x0001),
    "config_staging_record": (0x1FFFA5C8, 0x0094),
    "motor_sine_table": (0x1FFFA674, 0x2004),
    "sbox": (0x1FFFC678, 0x0100),
    "position_sensor_expected": (0x1FFFA65C, 0x000B),
    "position_dma_word": (0x1FFFA666, 0x0002),
    "dm4310_current_d": (0x1FFFC78C, 0x004C),
    "dm4310_current_q": (0x1FFFC7D8, 0x004C),
    "boot_record_staging": (0x1FFFC824, 0x0014),
    "short_response_buffer": (0x1FFFC838, 0x0014),
    "motor_encoder_correction": (0x1FFFC84C, 0x0400),
    "motor_record": (0x1FFFCC6C, 0x040C),
    "output_table": (0x1FFFD078, 0x2000),
    "output_sensor_calibration": (0x1FFFF078, 0x0010),
    "dm4310_motor_runtime_state": (0x1FFFF088, 0x007C),
    "dm4310_sample_runtime_state": (0x1FFFF104, 0x00A4),
    "output_sensor": (0x1FFFF1A8, 0x0048),
    "dm4310_runtime_status": (0x1FFFF1F0, 0x004C),
    "runtime_parameter_cache": (0x1FFFF23C, 0x0060),
    "dm4310_mcan_dispatch": (0x1FFFF29C, 0x00A4),
    "calibration_frame_scratch": (0x1FFFF340, 0x0040),
    "dm4310_motion_observer": (0x1FFFF380, 0x003C),
    "position_sensor_observed": (0x1FFFF3BC, 0x000B),
    "uart_rx_dma_buffer": (0x1FFFF3C7, 0x00C8),
    "dm4310_c_runtime_state": (0x1FFFF490, 0x0060),
    "dm4310_runtime_errno": (0x1FFFF490, 0x0060),
    "SystemCoreClock": (0x1FFFF4F0, 0x0004),
    "HRC_VALUE": (0x1FFFF4F4, 0x0004),
}

ELF_ABSOLUTE_SYMBOLS = {
    "__StackTop": 0x200004F8,
    "__StackLimit": 0x1FFFF8F8,
    "__HeapBase": 0x1FFFF4F8,
    "__HeapLimit": 0x1FFFF8F8,
    "__factory_bss_end__": 0x1FFFF4F8,
    "dm4310_position_sensor_scratch": 0x1FFFF190,
}

ELF_VECTOR_WORDS = {
    0: 0x200004F8,
    16: 0x1FFF8745,
    17: 0x1FFF8771,
    18: 0x1FFF8137,
    19: 0x1FFF88B9,
}

# Flash handlers use ordinary sequential placement.  Check the vector-to-
# symbol relationship without turning the current Flash address into an ABI.
ELF_VECTOR_SYMBOLS = {
    20: "IRQ004_Handler",
}


def fail(message: str) -> None:
    raise RuntimeError(message)


def verify_factory(path: pathlib.Path) -> None:
    data = path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != FACTORY_SHA256:
        fail(f"factory SHA-256 mismatch: {digest}")
    for address, expected in FACTORY_WORDS.items():
        offset = address - FACTORY_BASE
        if offset < 0 or offset + 4 > len(data):
            fail(f"factory address outside image: 0x{address:08x}")
        actual = struct.unpack_from("<I", data, offset)[0]
        if actual != expected:
            fail(
                f"factory word 0x{address:08x}: "
                f"got 0x{actual:08x}, expected 0x{expected:08x}"
            )
    for (address, size), expected in FACTORY_HELPERS.items():
        offset = address - FACTORY_BASE
        if offset < 0 or offset + size > len(data):
            fail(f"factory helper outside image: 0x{address:08x}")
        actual = hashlib.sha256(data[offset:offset + size]).hexdigest()
        if actual != expected:
            fail(
                f"factory helper 0x{address:08x}/0x{size:x}: "
                f"got {actual}, expected {expected}"
            )


def read_symbols(
    elf: pathlib.Path, nm: str
) -> tuple[dict[str, tuple[int, int]], dict[str, int]]:
    result = subprocess.run(
        [nm, "-S", "--defined-only", str(elf)],
        check=True,
        capture_output=True,
        text=True,
    )
    symbols: dict[str, tuple[int, int]] = {}
    absolute: dict[str, int] = {}
    for line in result.stdout.splitlines():
        fields = line.split()
        if len(fields) == 4:
            address, size, _kind, name = fields
            symbols[name] = (int(address, 16), int(size, 16))
        elif len(fields) == 3:
            address, kind, name = fields
            if kind.upper() == "A":
                absolute[name] = int(address, 16)
    return symbols, absolute


def verify_elf(
    path: pathlib.Path, nm: str, objcopy: str, objdump: str
) -> None:
    symbols, absolute = read_symbols(path, nm)
    for name, expected in ELF_SYMBOLS.items():
        actual = symbols.get(name)
        if actual != expected:
            fail(f"ELF symbol {name}: got {actual!r}, expected {expected!r}")
    for name, expected in ELF_ABSOLUTE_SYMBOLS.items():
        actual = absolute.get(name)
        if actual != expected:
            fail(
                f"ELF absolute symbol {name}: "
                f"got {actual!r}, expected 0x{expected:08x}"
            )
    with tempfile.TemporaryDirectory() as temporary_directory:
        vector_path = pathlib.Path(temporary_directory) / "vectors.bin"
        subprocess.run(
            [objcopy, "--dump-section", f".vectors={vector_path}", str(path)],
            check=True,
            capture_output=True,
            text=True,
        )
        vectors = vector_path.read_bytes()
    for index, expected in ELF_VECTOR_WORDS.items():
        offset = index * 4
        if offset + 4 > len(vectors):
            fail(f"ELF vector {index} outside .vectors section")
        actual = struct.unpack_from("<I", vectors, offset)[0]
        if actual != expected:
            fail(
                f"ELF vector {index}: got 0x{actual:08x}, "
                f"expected 0x{expected:08x}"
            )
    for index, symbol_name in ELF_VECTOR_SYMBOLS.items():
        symbol = symbols.get(symbol_name)
        if symbol is None:
            fail(f"ELF vector target symbol is missing: {symbol_name}")
        expected = symbol[0] | 1
        offset = index * 4
        if offset + 4 > len(vectors):
            fail(f"ELF vector {index} outside .vectors section")
        actual = struct.unpack_from("<I", vectors, offset)[0]
        if actual != expected:
            fail(
                f"ELF vector {index}: got 0x{actual:08x}, "
                f"expected {symbol_name}|1 = 0x{expected:08x}"
            )

    disassembly = subprocess.run(
        [objdump, "-d", str(path)],
        check=True,
        capture_output=True,
        text=True,
    ).stdout
    reset_match = re.search(
        r"^[0-9a-f]+ <Reset_Handler>:\n(.*?)(?=^[0-9a-f]+ <)",
        disassembly,
        re.MULTILINE | re.DOTALL,
    )
    if reset_match is None:
        fail("ELF Reset_Handler disassembly is missing")
    reset_body = reset_match.group(1)
    entry_branch = re.search(
        r"\bb(?:\.w)?\s+[0-9a-f]+\s+<runtime_main_entry>",
        reset_body,
    )
    if entry_branch is None:
        fail("ELF Reset_Handler lacks runtime main entry")
    entry_match = re.search(
        r"^[0-9a-f]+ <runtime_main_entry>:\n(.*?)"
        r"(?=^[0-9a-f]+ <)",
        disassembly,
        re.MULTILINE | re.DOTALL,
    )
    if entry_match is None:
        fail("ELF runtime main entry disassembly is missing")
    entry_body = entry_match.group(1)
    runtime_call = re.search(
        r"\bbl(?:\.w)?\s+[0-9a-f]+\s+<dm4310_runtime_initialize>",
        entry_body,
    )
    main_call = re.search(
        r"\bbl(?:\.w)?\s+[0-9a-f]+\s+<main>", entry_body
    )
    if runtime_call is None or main_call is None:
        fail("ELF runtime main entry lacks initialization or main call")
    if runtime_call.start() >= main_call.start():
        fail("ELF runtime initialization does not precede main")
    all_runtime_calls = re.findall(
        r"\bbl(?:\.w)?\s+[0-9a-f]+\s+<dm4310_runtime_initialize>",
        disassembly,
    )
    if len(all_runtime_calls) != 1:
        fail(
            "ELF dm4310_runtime_initialize callsites: "
            f"got {len(all_runtime_calls)}, expected 1"
        )
    configure_calls = re.findall(
        r"\bb(?:l)?(?:\.w)?\s+[0-9a-f]+\s+<motor_control_configure>",
        disassembly,
    )
    # The source-owned no-argument derive wrapper is the sole direct caller;
    # startup, motor-ID, UgQ and the RAM parser enter through 0x1fffa4dc.
    if len(configure_calls) != 1:
        fail(
            "ELF motor_control_configure callsites: "
            f"got {len(configure_calls)}, expected 1"
        )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--factory", type=pathlib.Path, required=True)
    parser.add_argument("--elf", type=pathlib.Path, required=True)
    parser.add_argument("--nm", default="arm-none-eabi-nm")
    parser.add_argument("--objcopy", default="arm-none-eabi-objcopy")
    parser.add_argument("--objdump", default="arm-none-eabi-objdump")
    args = parser.parse_args()
    try:
        verify_factory(args.factory)
        verify_elf(args.elf, args.nm, args.objcopy, args.objdump)
        # These arithmetic sections must be original instruction bodies, not
        # ABI veneers. This check stays outside the production build graph.
        factory = args.factory.read_bytes()
        with tempfile.TemporaryDirectory() as temporary_directory:
            for name in ("sincos", "clamp", "wrap", "limit_vector",
                         "float_to_uint", "uint_to_float"):
                address, size = ELF_SYMBOLS[f"dm4310_{name}_helper"]
                if name == "wrap":
                    size += ELF_SYMBOLS["wrap_angle_helper"][1]
                section_path = pathlib.Path(temporary_directory) / name
                subprocess.run(
                    [args.objcopy, "--dump-section",
                     f".dm4310_helper_{name}={section_path}", str(args.elf)],
                    check=True, capture_output=True, text=True,
                )
                offset = 0x8680 + address - 0x1FFF8000
                if section_path.read_bytes() != factory[offset:offset + size]:
                    fail(f"SRAM arithmetic body differs: {name}")
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"DM4310 contract verification failed: {error}", file=sys.stderr)
        return 1
    print("DM4310 factory literals and fixed SRAM contract verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
