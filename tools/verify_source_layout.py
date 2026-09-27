#!/usr/bin/env python3
"""Fail-closed structural checks for source-built DM4310 firmware images."""

from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import re
import struct
import subprocess
from pathlib import Path


SRAM_BASE = 0x1FFF8000
SRAM_END = 0x20008000
VECTOR_SIZE = 0x240
ICG_OFFSET = 0x400
ICG_SIZE = 0x60
RECOVERED_ICG = struct.pack("<I", 0xFFDFFFBF) + bytes([0xFF]) * (ICG_SIZE - 4)
OFFICIAL_APP_SHA256 = (
    "65aab219268e9159b196d4578d3cd530e6fa90a731a82670a0d3d3be609b59d4"
)


def run(*command: str) -> str:
    result = subprocess.run(command, check=True, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return result.stdout


def symbols(nm: str, elf: Path) -> dict[str, tuple[int, int, str]]:
    result: dict[str, tuple[int, int, str]] = {}
    output = run(nm, "-n", "-S", "--defined-only", str(elf))
    for line in output.splitlines():
        fields = line.split()
        if len(fields) == 4:
            address, size, kind, name = fields
        elif len(fields) == 3:
            address, kind, name = fields
            size = "0"
        else:
            continue
        if re.fullmatch(r"[0-9a-fA-F]+", address) and len(kind) == 1:
            result[name] = (int(address, 16), int(size, 16), kind)
    return result


def require_symbol(table: dict[str, tuple[int, int, str]], name: str) -> int:
    if name not in table:
        raise ValueError(f"required symbol is missing: {name}")
    return table[name][0]


def read_vectors(image: bytes) -> tuple[int, ...]:
    if len(image) < VECTOR_SIZE:
        raise ValueError("binary is shorter than the complete vector table")
    return struct.unpack_from("<" + "I" * (VECTOR_SIZE // 4), image)


def verify_vectors(kind: str, vectors: tuple[int, ...],
                   table: dict[str, tuple[int, int, str]],
                   official_app: bytes | None = None) -> None:
    base = 0x00020000 if kind == "app" else 0x00000000
    stack = require_symbol(table, "__StackTop")
    reset = require_symbol(table, "Reset_Handler") | 1
    default = require_symbol(table, "Default_Handler") | 1
    if vectors[0] != stack or stack != SRAM_END:
        raise ValueError(
            f"initial MSP is 0x{vectors[0]:08x}, expected source stack "
            f"0x{SRAM_END:08x}"
        )
    if vectors[1] != reset:
        raise ValueError(
            f"reset vector 0x{vectors[1]:08x} does not name Reset_Handler "
            f"0x{reset:08x}"
        )
    if require_symbol(table, "__Vectors") != base:
        raise ValueError("vector table is linked at the wrong Flash base")
    if require_symbol(table, "__Vectors_End") != base + VECTOR_SIZE:
        raise ValueError("vector table has an unexpected size")

    for slot in (2, 3, 4, 5, 6, 11, 12, 14):
        if vectors[slot] != default:
            raise ValueError(f"system vector slot {slot} is not fail-stop")
    for slot in (7, 8, 9, 10, 13):
        if vectors[slot] != 0:
            raise ValueError(f"reserved vector slot {slot} must be zero")

    if kind == "app":
        if official_app is None:
            raise ValueError("official APP is required for APP vector verification")
        official_vectors = read_vectors(official_app)
        official_external = official_vectors[16:]
        official_default = Counter(
            entry for entry in official_external if entry != 0
        ).most_common(1)[0][0]
        official_active = {
            irq for irq, entry in enumerate(official_external)
            if entry not in (0, official_default)
        }
        official_reserved = {
            irq for irq, entry in enumerate(official_external) if entry == 0
        }
        if official_active != set(range(5)):
            raise ValueError(
                "official APP external-vector role map changed: "
                f"active IRQs are {sorted(official_active)}"
            )
        source_reserved = {
            irq for irq, entry in enumerate(vectors[16:]) if entry == 0
        }
        if source_reserved != official_reserved:
            raise ValueError(
                "APP reserved external-vector slots differ from official APP: "
                f"source={sorted(source_reserved)}, "
                f"official={sorted(official_reserved)}"
            )
        if vectors[15] != default:
            raise ValueError("application SysTick must use the fail-stop handler")
        for irq in range(5):
            expected = require_symbol(table, f"IRQ{irq:03d}_Handler") | 1
            if vectors[16 + irq] != expected:
                raise ValueError(f"application IRQ{irq} vector mismatch")
        for irq in range(5, len(official_external)):
            expected = 0 if irq in official_reserved else default
            if vectors[16 + irq] != expected:
                raise ValueError(
                    f"application IRQ{irq:03d} vector role differs from official APP"
                )
        first_default = None
    else:
        systick = require_symbol(table, "SysTick_Handler") | 1
        if vectors[15] != systick:
            raise ValueError("bootloader SysTick vector mismatch")
        first_default = 16

    if first_default is not None:
        for slot in range(first_default, len(vectors)):
            if vectors[slot] != default:
                raise ValueError(f"unexpected implemented vector slot {slot}")


def verify_ram_primitive(objdump: str, elf: Path,
                         table: dict[str, tuple[int, int, str]]) -> None:
    name = "erase_and_program_from_sram"
    address = require_symbol(table, name)
    size = table[name][1]
    if not (SRAM_BASE <= address < SRAM_END and address + size <= SRAM_END):
        raise ValueError("Flash erase/program primitive is not wholly in SRAM")
    disassembly = run(objdump, "-d", f"--disassemble={name}", str(elf))
    if re.search(r"\tblx?\s", disassembly):
        raise ValueError("SRAM Flash primitive contains an external call")
    if "#400" not in disassembly and "0x190" not in disassembly:
        raise ValueError("SRAM Flash primitive does not access recovered F0NWPRT")
    # HC32F448 FWMC is unlocked by two writes to KEY1 at EFM base + 4.
    # KEY2 (base + 8) is only for OTP.  Using KEY1 then KEY2 compiles cleanly
    # but leaves normal Flash programming locked on the target, so keep this
    # as a post-link machine-code gate rather than trusting a source comment.
    key1_stores = re.findall(
        r"\bstr(?:\.w)?\b[^\n]*\[[^\]]+,\s*#4\]", disassembly
    )
    key2_stores = re.findall(
        r"\bstr(?:\.w)?\b[^\n]*\[[^\]]+,\s*#8\]", disassembly
    )
    if len(key1_stores) < 2 or key2_stores:
        raise ValueError(
            "SRAM Flash primitive must write both 0x01234567 and "
            "0xFEDCBA98 to EFM KEY1 (+4), never KEY2 (+8)"
        )
    recovered_values = (
        "#291", "#12816", "#63", "#458752", "#16", "#65536",
        "01234567", "fedcba98",
    )
    if not all(value in disassembly.lower() for value in recovered_values):
        raise ValueError(
            "SRAM Flash primitive no longer preserves the recovered FAPRT, "
            "cache, flag-clear or lock sequence"
        )
    if ("#10000000" in disassembly or "00989680" in disassembly.lower() or
            len(re.findall(r"\bbpl(?:\.n)?\b", disassembly.lower())) < 4):
        raise ValueError(
            "SRAM Flash primitive must retain the factory blocking RDY/"
            "OPTEND waits without a source-only timeout"
        )


def verify_boot_icg(image: bytes) -> None:
    if len(image) < ICG_OFFSET + ICG_SIZE:
        raise ValueError("bootloader image does not contain the complete ICG")
    if image[ICG_OFFSET:ICG_OFFSET + ICG_SIZE] != RECOVERED_ICG:
        raise ValueError("bootloader ICG/option bytes differ from recovered target")


def verify_no_undefined(nm: str, elf: Path) -> None:
    undefined = [line.strip() for line in run(nm, "-u", str(elf)).splitlines()
                 if line.strip()]
    if undefined:
        raise ValueError("undefined linked symbols: " + ", ".join(undefined))


def verify_hardware_float(objdump: str, elf: Path, kind: str) -> None:
    headers = run(objdump, "-x", str(elf))
    if "[hard-float ABI]" not in headers:
        raise ValueError("firmware is not linked with the Cortex-M4F hard-float ABI")
    if kind != "app":
        return
    control = run(objdump, "-d", "--disassemble=motor_control_fast_step",
                  str(elf))
    if not re.search(r"\bv(?:add|sub|mul|div|mla|mls)\.f32\b", control):
        raise ValueError("FOC fast path contains no single-precision VFP arithmetic")
    if re.search(r"<__(?:aeabi_f|addsf3|subsf3|mulsf3|divsf3)", control):
        raise ValueError("FOC fast path still calls a software-float helper")


def verify_no_binary_embedding(source_root: Path) -> None:
    for directory in ("app", "bootloader", "board", "common", "startup"):
        for path in (source_root / directory).rglob("*"):
            if path.suffix.lower() not in {".c", ".h", ".s"}:
                continue
            if ".incbin" in path.read_text(encoding="utf-8", errors="replace"):
                raise ValueError(f"historical binary embedding found in {path}")


def verify_no_test_fixtures(
    table: dict[str, tuple[int, int, str]],
) -> None:
    fixtures = sorted(name for name in table if name.startswith("captured_"))
    if fixtures:
        raise ValueError(
            "test calibration fixture linked into product: "
            + ", ".join(fixtures)
        )


def verify_commissioning_code(
    table: dict[str, tuple[int, int, str]], required: bool
) -> None:
    commissioned_symbols = (
        "commissioning_flux_observer_step",
        "commissioning_sine_regression_motor_parameters",
    )
    present = [name for name in commissioned_symbols if name in table]
    if required and len(present) != len(commissioned_symbols):
        missing = sorted(set(commissioned_symbols) - set(present))
        raise ValueError(
            "APP is missing energized identification "
            "code: " + ", ".join(missing)
        )
    if not required and present:
        raise ValueError(
            "bootloader contains APP commissioning "
            "code: " + ", ".join(present)
        )


def verify_app_boot_confirmation(
    objdump: str, elf: Path, table: dict[str, tuple[int, int, str]]
) -> None:
    """Keep the recovered loader/APP persistent-state handshake first."""
    require_symbol(table, "main")
    require_symbol(table, "platform_confirm_application_boot")
    require_symbol(table, "platform_update_application_identity")
    require_symbol(table, "platform_prepare_board_startup")
    disassembly = run(objdump, "-d", "--disassemble=main", str(elf))
    early_call = re.search(
        r"\bblx?\b[^\n]*<platform_early_init>", disassembly
    )
    confirm_call = re.search(
        r"\bblx?\b[^\n]*<platform_confirm_application_boot>", disassembly
    )
    identity_call = re.search(
        r"\bblx?\b[^\n]*<platform_update_application_identity>", disassembly
    )
    if early_call is None or confirm_call is None or identity_call is None:
        raise ValueError(
            "APP main is missing boot confirmation or application identity "
            "normalization"
        )
    if confirm_call.start() <= early_call.end():
        raise ValueError("APP boot confirmation is not after early clock init")
    between_calls = disassembly[early_call.end():confirm_call.start()]
    if re.search(r"\bblx?\b", between_calls):
        raise ValueError(
            "APP executes another function between clock setup and boot "
            "confirmation"
        )
    if re.search(
        r"\b(?:cbz|cbnz|b(?:eq|ne|cs|cc|mi|pl|vs|vc|hi|ls|ge|lt|gt|le))"
        r"(?:\.w)?\b",
        between_calls,
    ):
        raise ValueError(
            "APP boot confirmation is conditional; the recovered APP "
            "confirms every loader handoff"
        )

    if identity_call.start() <= confirm_call.end():
        raise ValueError("APP identity normalization is not after confirmation")
    between_confirm_and_identity = disassembly[
        confirm_call.end():identity_call.start()
    ]
    if re.search(r"\bblx?\b", between_confirm_and_identity):
        raise ValueError(
            "APP executes another function between boot confirmation and "
            "identity normalization"
        )
    after_identity = disassembly[identity_call.end():]
    irq_enable = re.search(r"\bcpsie\s+i\b", after_identity)
    first_post_call = re.search(r"\bblx?\b", after_identity)
    if (irq_enable is None or first_post_call is None or
            irq_enable.start() > first_post_call.start()):
        raise ValueError(
            "APP must enable global interrupts immediately after identity "
            "normalization, as the official main does"
        )

    post_confirm_calls = (
        "app_state_init",
        "debug_console_reset",
        "platform_prepare_board_startup",
        "platform_initialize_peripherals",
    )
    cursor = identity_call.end()
    for name in post_confirm_calls:
        match = re.search(rf"\bblx?\b[^\n]*<{name}>", disassembly[cursor:])
        if match is None:
            raise ValueError(
                f"APP startup is missing post-confirm call {name}"
            )
        cursor += match.end()

    early_disassembly = run(
        objdump, "-d", "--disassemble=platform_early_init", str(elf)
    )
    early_callees = re.findall(r"\b(?:blx?|b\.w)\b[^\n]*<([^>]+)>",
                               early_disassembly)
    if early_callees != ["board_clock_init"]:
        raise ValueError(
            "platform_early_init must contain only the recovered clock step "
            f"before confirmation; found {early_callees}"
        )

    confirm_disassembly = run(
        objdump, "-d", "--disassemble=platform_confirm_application_boot",
        str(elf),
    )
    if "#122880" not in confirm_disassembly and "0x1e000" not in confirm_disassembly:
        raise ValueError("APP confirmation does not address boot record 0x1e000")
    if not re.search(
        r"\bblx?\b[^\n]*<board_flash_replace_sector_prefix>",
        confirm_disassembly,
    ):
        raise ValueError("APP confirmation does not commit through the SRAM Flash path")
    if not re.search(r"\bldr\b[^\n]*\[[^\]]+,\s*#4\]", confirm_disassembly):
        raise ValueError("APP confirmation does not inspect boot-record word 1")

    identity_disassembly = run(
        objdump, "-d", "--disassemble=platform_update_application_identity",
        str(elf),
    )
    if "07010005" not in identity_disassembly:
        raise ValueError("APP identity is not the official 0x07010005 value")
    if not re.search(
        r"\bblx?\b[^\n]*<board_flash_replace_sector_prefix>",
        identity_disassembly,
    ):
        raise ValueError("APP identity update does not use the SRAM Flash path")


def verify_app_reset_handoff(
    objdump: str, elf: Path, table: dict[str, tuple[int, int, str]]
) -> None:
    """Match the official PRIMASK, FPU and two-stage VTOR handoff."""
    for name in ("Reset_Handler", "SystemInit", "main"):
        require_symbol(table, name)

    reset = run(
        objdump, "-d", "--disassemble=Reset_Handler", str(elf)
    ).lower()
    if re.search(r"\bcpsi[de]\s+i\b", reset):
        raise ValueError(
            "APP Reset_Handler must preserve incoming PRIMASK like the "
            "official Reset_Handler"
        )
    system_call = re.search(r"\bblx?\b[^\n]*<systeminit>", reset)
    main_call = re.search(r"\bblx?\b[^\n]*<main>", reset)
    first_reset_call = re.search(r"\bblx?\b", reset)
    if (system_call is None or main_call is None or
            first_reset_call is None or
            first_reset_call.start() != system_call.start() or
            system_call.start() >= main_call.start()):
        raise ValueError(
            "APP reset path must enter SystemInit before GCC runtime work "
            "and main"
        )

    system = run(
        objdump, "-d", "--disassemble=SystemInit", str(elf)
    ).lower()
    system_compact = " ".join(system.split())
    if ("e000ed00" not in system or "#15728640" not in system or
            not re.search(
                r"#0\b.*?\bstr(?:\.w)?\b[^\n]*\[[^\]]+,\s*#8\]",
                system_compact,
            )):
        raise ValueError(
            "APP SystemInit must enable CP10/CP11 and write VTOR=0 before "
            "the main-level APP vector switch"
        )

    main_disassembly = run(
        objdump, "-d", "--disassemble=main", str(elf)
    ).lower()
    early_call = re.search(
        r"\bblx?\b[^\n]*<platform_early_init>", main_disassembly
    )
    if early_call is None:
        raise ValueError("APP main does not call platform_early_init")
    main_prefix = " ".join(main_disassembly[:early_call.start()].split())
    if not re.search(
        r"#131072\b.*?\bstr(?:\.w)?\b[^\n]*\[[^\]]+,\s*#8\]",
        main_prefix,
    ):
        raise ValueError(
            "APP main must install VTOR=0x00020000 before clock setup"
        )
    if re.search(r"\bblx?\b", main_prefix):
        raise ValueError("APP main calls code before installing VTOR/clock")


def verify_recovered_clock_config(
    objdump: str, elf: Path, table: dict[str, tuple[int, int, str]]
) -> None:
    """Pin the complete factory clock/SysTick/TMRA startup contract."""
    require_symbol(table, "board_clock_build_config")
    require_symbol(table, "board_clock_init")
    config = run(
        objdump, "-d", "--disassemble=board_clock_build_config", str(elf)
    ).lower()
    recovered_images = (
        "00112210",  # SCFGR
        "39306300",  # PLLHCFGR
        "00070003",  # EFM FRMC
        "11000000",  # SRAM wait-state image
    )
    if (not all(value in config for value in recovered_images) or
            "11002210" in config):
        raise ValueError(
            "APP clock config does not contain the four recovered register "
            "images (SCFGR/PLLHCFGR/FRMC/SRAM wait states)"
        )

    clock = run(
        objdump, "-d", "--disassemble=board_clock_init", str(elf)
    ).lower()
    compact = " ".join(clock.split())

    # These peripheral bases and protected-write keys are all exercised by
    # system_clock_and_systick_init@0x23208, not merely present in a header.
    required_literals = (
        "e000e100",  # NVIC ICER
        "40054000",  # CMU
        "40048000",  # PWC / FCG0..3
        "4004c000",  # XTALCFGR alias selected by the compiler
        "40050000",  # legacy SRAM wait-state registers
        "4003a000",  # TMRA1
        "a5a50001",  # FCG unlock
        "fffffa0e",  # original FCG0 transition mask
        "a5a50000",  # FCG lock
    )
    if not all(value in clock for value in required_literals):
        raise ValueError(
            "APP clock init is missing a recovered peripheral address, "
            "FCG key or transition mask"
        )

    irq_mask_sequence = re.search(
        r"#2\b.*?#128\].*?\bdsb\b.*?\bisb\b.*?"
        r"#4\b.*?#128\].*?\bdsb\b.*?\bisb\b.*?"
        r"#8\b.*?#128\].*?\bdsb\b.*?\bisb\b",
        compact,
    )
    if irq_mask_sequence is None:
        raise ValueError(
            "APP clock init must mask inherited IRQ001/002/003 with the "
            "recovered ICER + DSB + ISB sequence"
        )

    # Require the observable stages in their factory order.  The compiler may
    # choose different base registers, so offsets and literal-pool addresses
    # are used instead of register names.
    ordered_stages = (
        "#32]",      # SCFGR
        "#3152]",    # XTALCFGR = 0xA0
        "#50]",      # XTALCR = 0
        "#2052]",    # SRAM wait-state unlock/write sequence
        "#3276]",    # RAMOPM = 0x8043
        "#256]",     # PLLH source/configuration writes
        "#42]",      # PLLHCR = 0
        "#4278190080",  # SysTick LOAD = 0x00ffffff
        "#1048576",  # enable TMRA1 clock
        "#128]",     # TMRA1 BCSTRL
        "#129]",     # TMRA1 BCSTRH
    )
    positions: list[int] = [irq_mask_sequence.start()]
    cursor = irq_mask_sequence.end()
    for stage in ordered_stages:
        position = clock.find(stage, cursor)
        if position < 0:
            raise ValueError(
                f"APP clock init is missing recovered ordered stage {stage}"
            )
        positions.append(position)
        cursor = position + len(stage)
    if positions != sorted(positions):
        raise ValueError("APP clock init stages differ from factory order")

    memory_contract = (
        "#119",       # SRAM WP unlock 0x77
        "#2052]",
        "#2060]",
        "#118",       # SRAM WP lock 0x76
        "#32835",     # RAMOPM 0x8043
        "#291",       # EFM FAPRT unlock 0x0123
        "#12816",     # EFM FAPRT lock 0x3210
        "#24]",       # EFM FRMC
    )
    if not all(token in clock for token in memory_contract):
        raise ValueError(
            "APP clock init does not reproduce the SRAM/EFM protected "
            "wait-state sequence"
        )

    systick_contract = re.search(
        r"e000e000.*?#4278190080.*?#16\].*?#20\].*?#4\b.*?"
        r"#24\].*?#16\]",
        compact,
    )
    if systick_contract is None:
        raise ValueError(
            "APP clock init must leave SysTick at LOAD=0x00ffffff, VAL=0 "
            "and CTRL=CLKSOURCE"
        )

    tmra_contract = re.search(
        r"#1048576.*?#1\b.*?#16\b.*?"
        r"\[[^\]]+\].*?#128\].*?#129\]",
        compact,
    )
    if tmra_contract is None:
        raise ValueError(
            "APP clock init does not reproduce the recovered TMRA1 clock, "
            "CNTER, BCSTRL and BCSTRH writes"
        )

    if ("<board_clock_validate_config>" in clock or
            "003d0900" in clock or
            re.search(r"\bstrb(?:\.w)?\b[^\n]*\[[^\]]+,\s*#48\]", clock)):
        raise ValueError(
            "APP clock init contains a source-only config rejection, ready "
            "timeout or HRC detour"
        )


def verify_app_led_mapping(
    objdump: str, elf: Path, table: dict[str, tuple[int, int, str]]
) -> None:
    """Pin the target-confirmed PC13=red and PH2=green color mapping."""
    require_symbol(table, "board_led_set")
    require_symbol(table, "board_led_toggle_fault_indicator")
    require_symbol(table, "app_service_control_status_tick")
    disassembly = run(objdump, "-d", "--disassemble=board_led_set", str(elf))
    compact = " ".join(disassembly.split())
    required = (
        "#42",  # PORRC: turn PC13/red off
        "#90",  # PORRH: turn PH2/green off
        "#40",  # POSRC: turn red on
        "#88",  # POSRH: turn green on
    )
    if not all(offset in disassembly for offset in required):
        raise ValueError("status LED driver does not access both recovered pins")
    if not re.search(
        r"cmp\s+r0,\s*#2.*?beq.*?cmp\s+r0,\s*#1.*?"
        r"strheq(?:\.w)?.*?#40.*?strh(?:\.w)?.*?#88",
        compact,
    ):
        raise ValueError(
            "status LED mapping must encode RED=PC13/POSRC and "
            "GREEN=PH2/POSRH"
        )

    fault_toggle = run(
        objdump, "-d", "--disassemble=board_led_toggle_fault_indicator",
        str(elf),
    ).lower()
    if ("#8192" not in fault_toggle or "#40]" not in fault_toggle or
            "#4" not in fault_toggle or "#92]" not in fault_toggle or
            "dsb" not in fault_toggle):
        raise ValueError(
            "fault indicator must force PC13/red through POSRC and toggle "
            "PH2/green through POTRH"
        )

    fault_service = run(
        objdump, "-d", "--disassemble=app_service_control_status_tick",
        str(elf),
    ).lower()
    if ("#250" not in fault_service or
            "<platform_toggle_fault_indicator>" not in fault_service):
        raise ValueError(
            "fault indicator lost the original 251 outer-loop-tick cadence"
        )


def verify_app_hardware_variant(
    objdump: str, elf: Path, table: dict[str, tuple[int, int, str]]
) -> None:
    """Pin PC14/PC15 strap configuration, decode and startup timing."""
    for name in (
        "board_identity_decode_hardware_variant",
        "board_identity_read_hardware_variant",
        "platform_prepare_board_startup",
        "platform_read_hardware_variant",
        "debug_console_print_status",
    ):
        require_symbol(table, name)

    identity = run(
        objdump, "-d",
        "--disassemble=board_identity_read_hardware_variant", str(elf)
    ).lower()
    required = (
        "40053800",  # GPIO base
        "#42241",    # PWPR unlock 0xa501
        "#1208]",    # PCRC14
        "#1212]",    # PCRC15
        "#64",       # GPIO_PCR_PUU
        "#42240",    # PWPR lock 0xa500
        "#32]",      # PIDRC
        "<board_identity_decode_hardware_variant>",
    )
    if not all(token in identity for token in required):
        raise ValueError(
            "hardware-variant reader must configure PC14/PC15 pull-ups and "
            "decode PIDRC through the recovered source path"
        )
    pwpr_stores = list(re.finditer(
        r"\bstrh(?:\.w)?\b[^\n]*#1020\]", identity
    ))
    strap14 = re.search(r"\bstrh(?:\.w)?\b[^\n]*#1208\]", identity)
    strap15 = re.search(r"\bstrh(?:\.w)?\b[^\n]*#1212\]", identity)
    pidrc = re.search(r"\bldrh(?:\.w)?\b[^\n]*#32\]", identity)
    if (len(pwpr_stores) != 2 or strap14 is None or strap15 is None or
            pidrc is None):
        raise ValueError("hardware-variant register operation count changed")
    identity_positions = [
        pwpr_stores[0].start(),
        strap14.start(),
        strap15.start(),
        pwpr_stores[1].start(),
        pidrc.start(),
    ]
    if identity_positions != sorted(identity_positions):
        raise ValueError("PC14/PC15 strap register writes are out of order")

    decode = run(
        objdump, "-d",
        "--disassemble=board_identity_decode_hardware_variant", str(elf)
    ).lower()
    if not re.search(r"\blsrs?\b[^\n]*#14\b", decode):
        raise ValueError("hardware-variant decode is not PIDRC[15:14]")

    prepare = run(
        objdump, "-d", "--disassemble=platform_prepare_board_startup",
        str(elf),
    ).lower()
    ordered_calls = (
        "<board_delay_ms>",
        "<board_led_init>",
        "<board_led_set>",
        "<board_identity_read_hardware_variant>",
    )
    cursor = 0
    for callee in ordered_calls:
        position = prepare.find(callee, cursor)
        if position < 0:
            raise ValueError(
                "post-delay board identity/LED startup is missing " + callee
            )
        cursor = position + len(callee)

    status = run(
        objdump, "-d", "--disassemble=debug_console_print_status", str(elf)
    ).lower()
    if "<platform_read_hardware_variant>" not in status:
        raise ValueError("startup status no longer reads the hardware suffix")


def verify_app_debug_uart(
    objdump: str, elf: Path, table: dict[str, tuple[int, int, str]]
) -> None:
    """Pin factory USART1/DMA1/TMR0 setup and timeout IRQ sequencing."""
    for name in (
        "board_uart_build_config",
        "board_uart_init",
        "board_uart_write",
        "board_uart_ack_interrupt",
        "IRQ004_Handler",
    ):
        require_symbol(table, name)

    config = run(
        objdump, "-d", "--disassemble=board_uart_build_config", str(elf)
    ).lower()
    config_values = (
        "#1506", "a000000f", "a000000c", "#2048",
        "#21808", "00c80001", "00210020", "#100", "#322", "#324",
    )
    if not all(value in config for value in config_values):
        raise ValueError("USART1 register image differs from recovered values")

    init = run(
        objdump, "-d", "--disassemble=board_uart_init", str(elf)
    ).lower()
    bases = (
        "40048000",  # PWC
        "40053800",  # GPIO
        "4001cc00",  # USART1
        "40024000",  # TMR0_1
        "40053000",  # DMA1
        "40010800",  # AOS
        "40051000",  # INTC
        "e000e100",  # NVIC
    )
    if not all(base in init for base in bases):
        raise ValueError("USART1 init is missing a recovered peripheral base")
    if ("#1070]" not in init or "#1074]" not in init or
            re.search(
                r"\bstrh(?:\.w)?\b[^\n]*#(?:1068|1072)\]", init
            )):
        raise ValueError(
            "USART1 GPIO setup must write only PA11/PA12 FSEL, not PCR"
        )

    # Original order: RTOE/RTOIE, INTSEL4, clear pending, priority 4,
    # NVIC enable, then RE/TE.  This avoids silently accepting a source-only
    # receiver-enable ordering that can change the first frame boundary.
    rto = re.search(r"\borr[^\n]*#3\b", init)
    intsel = re.search(r"\bstr[^\n]*#108\]", init)
    clear_pending = re.search(r"\bstr[^\n]*#384\]", init)
    priority = re.search(r"\bstrb[^\n]*#772\]", init)
    rx_tx = re.search(r"\borr[^\n]*#12\b", init)
    if any(match is None for match in
           (rto, intsel, clear_pending, priority, rx_tx)):
        raise ValueError("USART1 interrupt/start sequence is incomplete")
    uart_positions = [
        rto.start(), intsel.start(), clear_pending.start(), priority.start(),
        rx_tx.start(),
    ]
    if uart_positions != sorted(uart_positions):
        raise ValueError(
            "USART1 must route/enable timeout IRQ before starting RX/TX"
        )

    transmit = run(
        objdump, "-d", "--disassemble=board_uart_write", str(elf)
    ).lower()
    if ("4001cc00" not in transmit or
            not re.search(r"\bbpl(?:\.n)?\b", transmit) or
            "000f4240" in transmit or "#1000000" in transmit):
        raise ValueError(
            "USART1 TX must retain the factory blocking TXE wait without a "
            "source-only timeout"
        )

    ack = run(
        objdump, "-d", "--disassemble=board_uart_ack_interrupt", str(elf)
    ).lower()
    if ("#1769472" not in ack or "#983055" not in ack or
            "40024000" not in ack or "40053000" not in ack or
            "e000e100" not in ack):
        raise ValueError(
            "USART1 timeout epilogue must stop TMR0, clear 0x001b0000, "
            "rearm DMA1 and clear IRQ004"
        )

    irq = run(
        objdump, "-d", "--disassemble=IRQ004_Handler", str(elf)
    ).lower()
    irq_calls = (
        "<platform_debug_receive>",
        "<debug_console_end_frame>",
        "<platform_ack_debug_uart_irq>",
    )
    irq_positions = [irq.find(callee) for callee in irq_calls]
    if any(position < 0 for position in irq_positions) or \
            irq_positions != sorted(irq_positions):
        raise ValueError(
            "IRQ004 must drain DMA data, close the frame, then acknowledge"
        )


def verify_app_mcan(
    objdump: str, elf: Path, table: dict[str, tuple[int, int, str]]
) -> None:
    """Pin the recovered MCAN1 pins, timing, RAM layout and IRQ epilogue."""
    for name in (
        "board_mcan_build_config",
        "board_mcan_init",
        "board_mcan_receive",
        "board_mcan_send",
        "board_mcan_ack_interrupt",
        "mcan1_receive_irq",
        "IRQ003_Handler",
    ):
        require_symbol(table, name)

    config = run(
        objdump, "-d", "--disassemble=board_mcan_build_config", str(elf)
    ).lower()
    # These are the complete selector-9 live image plus the values that
    # distinguish the easily-confused selectors 10 and 11.  The latter were
    # recovered from mcan1_configure_fd rather than inferred from a formula.
    config_values = (
        "26003a13", "00011e77", "80030080", "80030158",
        "03000328", "00040308", "02800001", "02800011",
        "8fff07ff", "00320033", "#2850", "#3072",
        "#1553", "#1792", "#1041", "#1280",
    )
    if not all(value in config for value in config_values):
        raise ValueError(
            "MCAN selector timing, FIFO/filter image or PB6/PB7 mux differs "
            "from the recovered values"
        )

    init = run(
        objdump, "-d", "--disassemble=board_mcan_init", str(elf)
    ).lower()
    bases = (
        "40048000",  # PWC/FCG1
        "40053800",  # GPIO PB6/PB7
        "40054000",  # CMU/CANCKCFGR
        "40029000",  # MCAN1
        "4002b000",  # message RAM start
        "4002b400",  # message RAM end
        "40051000",  # INTC
        "e000e100",  # NVIC
    )
    if not all(base in init for base in bases):
        raise ValueError("MCAN1 init is missing a recovered peripheral base")
    gpio_offsets = ("#1112]", "#1114]", "#1116]", "#1118]")
    if not all(offset in init for offset in gpio_offsets):
        raise ValueError("MCAN1 must program PB6/PB7 PCR and FSEL registers")
    if ("#8" not in init or "#15" not in init or
            "#409" not in init or "#771]" not in init or
            "#384]" not in init):
        raise ValueError(
            "MCAN1 clock, transition delay or IRQ003 route/priority differs "
            "from the recovered setup"
        )
    if ("#4294967295" not in init or "#80]" not in init or
            not re.search(r"\bstr\.w\b[^\n]*\[r3\],\s*#4", init)):
        raise ValueError(
            "MCAN1 message RAM clear or initial IR acknowledgement is missing"
        )
    if "000f4240" in init or "#1000000" in init:
        raise ValueError(
            "MCAN1 factory INIT/CSA waits must not have a source-only timeout"
        )
    # The source must wait for CSA to clear, INIT to set, and INIT to clear.
    # At -Os those three polls are bmi, bpl and bmi backward branches.
    if (len(re.findall(r"\bbmi(?:\.n)?\b", init)) < 2 or
            not re.search(r"\bbpl(?:\.n)?\b", init)):
        raise ValueError("MCAN1 CCCR transition waits are incomplete")

    receive = run(
        objdump, "-d", "--disassemble=board_mcan_receive", str(elf)
    ).lower()
    if ("<mcan_getrxmsg>" not in receive or "#64" not in receive or
            "<memcpy>" not in receive):
        raise ValueError(
            "MCAN RX must drain FIFO0 through the DDL and copy its payload"
        )

    transmit = run(
        objdump, "-d", "--disassemble=board_mcan_send", str(elf)
    ).lower()
    if ("<mcan_addmsgtotxfifoqueue>" not in transmit or
            "#2048" not in transmit or "#8" not in transmit or
            "<memcpy>" not in transmit):
        raise ValueError(
            "MCAN TX must enforce standard 11-bit IDs/DLC<=8 and use FIFOQ"
        )

    ack = run(
        objdump, "-d", "--disassemble=board_mcan_ack_interrupt", str(elf)
    ).lower()
    if ("40029000" not in ack or "e000e100" not in ack or
            "#23, #1" not in ack or "#80]" not in ack or
            "#24]" not in ack or "#384]" not in ack or
            not re.search(r"\bbic[^\n]*#1\b", ack)):
        raise ValueError(
            "MCAN IRQ ack must preserve bit23/bus-off reporting, clear INIT "
            "on bus-off, write IR and clear NVIC IRQ003"
        )

    handler = run(
        objdump, "-d", "--disassemble=mcan1_receive_irq", str(elf)
    ).lower()
    handler_calls = (
        "<platform_mcan_receive>",
        "<parameter_protocol_process>",
        "<can_protocol_decode_command>",
        "<platform_ack_mcan_irq>",
    )
    if not all(callee in handler for callee in handler_calls):
        raise ValueError(
            "IRQ003 no longer drains MCAN, dispatches both protocols and "
            "acknowledges the controller"
        )
    irq = run(
        objdump, "-d", "--disassemble=IRQ003_Handler", str(elf)
    ).lower()
    if "<mcan1_receive_irq>" not in irq:
        raise ValueError("IRQ003 vector wrapper does not dispatch MCAN1")


def verify_app_motor_can_protocol(
    objdump: str, elf: Path, table: dict[str, tuple[int, int, str]]
) -> None:
    """Keep motor-family dispatch and FC/FD/FE/FB behavior source-owned."""
    for name in (
        "can_protocol_decode_command",
        "can_protocol_encode_feedback",
        "app_apply_can_command",
        "app_service_motor_state_change",
        "motor_float_to_uint",
        "motor_uint_to_float",
    ):
        require_symbol(table, name)

    decode = run(
        objdump, "-d", "--disassemble=can_protocol_decode_command", str(elf)
    ).lower()
    if not all(value in decode for value in
               ("#251", "#252", "#253", "#254", "#255")):
        raise ValueError("motor CAN FC/FD/FE/FB byte recognition is incomplete")
    if ("#1792" not in decode or "#8, #3" not in decode or
            not re.search(r"\buxtb\b", decode) or "#176]" not in decode):
        raise ValueError(
            "motor CAN dispatch must compare the masked low-byte node ID "
            "and decode ID[10:8] as the control family"
        )
    if ("#10000" not in decode or "#182]" not in decode or
            decode.count("<motor_uint_to_float>") != 5):
        raise ValueError(
            "motor CAN mode selection, MIT unpack or hybrid limits differ "
            "from the recovered handler"
        )
    if re.search(r"\bldrb(?:\.w)?\b[^\n]*\[[^\]]+,\s*#4\]", decode):
        # CanFrame.length is byte offset 4.  The shipped handler consumes the
        # first two data words without consulting DLC.
        raise ValueError("motor CAN decoder added a source-only DLC gate")

    encode = run(
        objdump, "-d", "--disassemble=can_protocol_encode_feedback", str(elf)
    ).lower()
    if (encode.count("<motor_float_to_uint>") != 3 or
            len(re.findall(r"\bvcvt\.s32\.f32\b", encode)) < 2 or
            "<motor_clampf>" in encode):
        raise ValueError(
            "motor feedback must use three original packed conversions and "
            "raw signed temperature-byte conversions"
        )

    pack = run(
        objdump, "-d", "--disassemble=motor_float_to_uint", str(elf)
    ).lower()
    unpack = run(
        objdump, "-d", "--disassemble=motor_uint_to_float", str(elf)
    ).lower()
    if ("vcvt.s32.f32" not in pack or "<motor_clampf>" in pack or
            re.search(r"\band\w*\b", unpack)):
        raise ValueError(
            "packed conversion helpers must extrapolate like the original, "
            "without clamp or source-only input masking"
        )

    apply = run(
        objdump, "-d", "--disassemble=app_apply_can_command", str(elf)
    ).lower()
    if ("<motor_control_arm>" not in apply or "<motor_control_disarm>" not in apply or
            "<safety_clear>" not in apply or "#508]" not in apply or
            "<platform_enable_pwm>" in apply or "<platform_disable_pwm>" in apply):
        raise ValueError(
            "FC/FD/FB must use the original fault<2 motor-state transition "
            "without an extra PWM hardware gate"
        )

    service = run(
        objdump, "-d", "--disassemble=app_service_motor_state_change",
        str(elf),
    ).lower()
    if ("<platform_debug_write>" not in service or
            "<platform_set_status_led>" not in service):
        raise ValueError(
            "deferred motor-state transition no longer updates console/LED"
        )

    control_irq = run(
        objdump, "-d", "--disassemble=adc_foc_control_irq", str(elf)
    ).lower()
    if "<platform_disable_pwm>" in control_irq:
        raise ValueError(
            "fault path added a hardware PWM disconnect absent from the "
            "original neutral-output motor-state transition"
        )


def verify_app_parameter_protocol(
    objdump: str, elf: Path, table: dict[str, tuple[int, int, str]]
) -> None:
    """Pin the recovered 0x7ff register/legacy protocol in final code."""
    for name in (
        "parameter_protocol_process",
        "board_mcan_update_node_filter",
        "platform_update_mcan_node_filter",
        "mcan1_receive_irq",
    ):
        require_symbol(table, name)

    protocol = run(
        objdump, "-d", "--disassemble=parameter_protocol_process", str(elf)
    ).lower()
    if not all(value in protocol for value in
               ("#51", "#85", "#170", "#204", "#2047")):
        raise ValueError(
            "parameter protocol lost a recovered operation or 0x7ff dispatch"
        )
    if ("#329]" not in protocol or "#81" not in run(
            objdump, "-d", "--disassemble=read_register", str(elf)
    ).lower()):
        raise ValueError(
            "parameter legacy disabled-state gate or read-selector extent "
            "differs from the recovered handler"
        )
    if re.search(r"\bldrb(?:\.w)?\b[^\n]*\[r4,\s*#4\]", protocol):
        raise ValueError("parameter protocol added a source-only DLC gate")

    # These bounds distinguish the original bit-pattern comparisons from
    # attractive but incompatible isfinite()/float-range validation.
    boundary_values = (
        "41200000", "461c4000", "03544000", "#40894464",
        "#11010048", "#1065353216", "#1107296256", "#11", "#35",
    )
    if not all(value in protocol for value in boundary_values):
        raise ValueError(
            "parameter write boundaries no longer match the recovered raw/"
            "floating-point comparison mix"
        )
    if protocol.count("<motor_control_configure>") < 1:
        raise ValueError(
            "parameter writes no longer refresh derived control coefficients"
        )

    update_filter = run(
        objdump, "-d", "--disassemble=board_mcan_update_node_filter", str(elf)
    ).lower()
    if ("07ff0000" not in update_filter or "4002b000" not in update_filter or
            "0x88000000" not in update_filter or "#255" not in update_filter or
            "dsb" not in update_filter):
        raise ValueError(
            "node-ID update must rewrite the recovered MCAN standard-filter "
            "word directly"
        )

    handler = run(
        objdump, "-d", "--disassemble=mcan1_receive_irq", str(elf)
    ).lower()
    if ("<platform_update_mcan_node_filter>" not in handler or
            "<platform_mcan_send>" not in handler):
        raise ValueError(
            "IRQ003 no longer applies parameter filter updates and responses"
        )


def verify_app_firmware_control_uart(
    objdump: str, elf: Path, table: dict[str, tuple[int, int, str]]
) -> None:
    """Pin setup-console dispatch and the 128-byte Ug control block."""
    for name in (
        "debug_console_end_frame",
        "firmware_control_service",
        "platform_enter_bootloader",
        "platform_enter_bootloader_from_can",
        "platform_request_bootloader_record",
    ):
        require_symbol(table, name)
    process_names = [name for name in table if name.startswith("process_frame")]
    if len(process_names) != 1:
        raise ValueError("setup-console frame dispatcher is missing or ambiguous")
    process = run(
        objdump, "-d", f"--disassemble={process_names[0]}", str(elf)
    ).lower()
    command_bytes = (
        "#27", "#88", "#109", "#115", "#85", "#99", "#108",
        "#101", "#102", "#103", "#70", "#67", "#66", "#170",
        "#131", "#128", "#11",
    )
    if not all(value in process for value in command_bytes):
        raise ValueError(
            "UART ESC/X/m/s/U*/FCB command dispatch is incomplete"
        )
    required_calls = (
        "<platform_enter_bootloader>",
        "<motor_control_arm>",
        "<motor_control_disarm>",
        "<calibration_upload_receive_frame>",
        "<platform_read_device_identity>",
    )
    if not all(callee in process for callee in required_calls):
        raise ValueError(
            "UART setup-state transitions or binary command handlers are missing"
        )
    if ("<platform_disable_pwm>" in process or
            "<platform_set_status_led>" in process):
        raise ValueError(
            "UART parser added source-only PWM/LED side effects to ESC/setup"
        )

    service = run(
        objdump, "-d", "--disassemble=firmware_control_service", str(elf)
    ).lower()
    if not all(value in service for value in
               ("#128", "#130", "#170", "#103")):
        raise ValueError("Ug block framing is no longer 128 bytes plus g/AA")
    service_calls = (
        "<motor_control_configure>",
        "<platform_store_parameters>",
        "<platform_debug_write>",
        "<platform_system_reset>",
    )
    if not all(callee in service for callee in service_calls):
        raise ValueError("Ug read/live/store/reset behavior is incomplete")

    enter = run(
        objdump, "-d", "--disassemble=platform_enter_bootloader", str(elf)
    ).lower()
    if (not re.search(r"\bcpsid\s+i\b", enter) or
            "<platform_request_bootloader_record>" not in enter or
            "<board_delay_us>" not in enter or "#100" not in enter or
            "nvic_systemreset>" not in enter):
        raise ValueError(
            "UART X transition must mask IRQs, commit the boot record, wait "
            "100 us and reset"
        )
    if "<board_mcan_flush>" in enter or "<board_uart_flush>" in enter:
        raise ValueError("UART X transition still contains a source-only drain")

    can_enter = run(
        objdump, "-d", "--disassemble=platform_enter_bootloader_from_can",
        str(elf),
    ).lower()
    if ("<platform_request_bootloader_record>" not in can_enter or
            "<board_delay_ms>" not in can_enter or "#10" not in can_enter or
            "nvic_systemreset>" not in can_enter or
            "<board_mcan_flush>" in can_enter or
            "<board_uart_flush>" in can_enter):
        raise ValueError(
            "CAN Aupgrade transition must commit, wait 10 ms and reset "
            "without a source-only transport drain"
        )

    mcan = run(
        objdump, "-d", "--disassemble=mcan1_receive_irq", str(elf)
    ).lower()
    response = mcan.find("<platform_mcan_send>")
    restart = mcan.find("<platform_enter_bootloader_from_can>")
    ack = mcan.find("<platform_ack_mcan_irq>")
    if response < 0 or restart <= response or ack < 0:
        raise ValueError(
            "CAN boot request must send Aupgrade before the direct reset path"
        )

    system_reset = run(
        objdump, "-d", "--disassemble=platform_system_reset", str(elf)
    ).lower()
    if (not re.search(r"\bcpsid\s+i\b", system_reset) or
            "<board_delay_us>" not in system_reset or
            "#100" not in system_reset or
            "nvic_systemreset>" not in system_reset or
            "<board_mcan_flush>" in system_reset or
            "<board_uart_flush>" in system_reset):
        raise ValueError(
            "UgU reset must use the original IRQ-mask/100-us sequence"
        )


def verify_app_flash_records(
    objdump: str, elf: Path, table: dict[str, tuple[int, int, str]]
) -> None:
    """Pin every APP-owned persistent-sector address and prefix length."""
    # expected = {
    #     "platform_store_parameters": ("0x3E000", "#148"),
    #     "platform_store_factory_parameters": ("0x3E000", "#148"),
    #     "platform_store_zero_position": ("0x36000", "#8"),
    #     "platform_store_motor_encoder_calibration": ("0x3c000", "#1036"),
    # }
    # for name, anchors in expected.items():
    #     require_symbol(table, name)
    #     disassembly = run(
    #         objdump, "-d", f"--disassemble={name}", str(elf)
    #     ).lower()
    #     if (not all(anchor in disassembly for anchor in anchors) or
    #             "<board_flash_replace_sector_prefix>" not in disassembly):
    #         raise ValueError(
    #             f"{name} has the wrong Flash address or record length"
    #         )

    output = run(
        objdump, "-d",
        "--disassemble=platform_store_output_sensor_calibration", str(elf)
    ).lower()
    if not all(anchor in output for anchor in
               ("0x3a000", "#8192", "0x38000", "#16")):
        raise ValueError(
            "output-sensor LUT/parameter Flash records have the wrong layout"
        )
    if output.count("<board_flash_replace_sector_prefix>") != 2:
        raise ValueError("output-sensor commit must replace exactly two sectors")

    zero = run(
        objdump, "-d", "--disassemble=calibration_store_encode_zero", str(elf)
    ).lower()
    if (len(re.findall(r"\bldr\b", zero)) != 2 or
            len(re.findall(r"\bstr\b", zero)) != 2 or
            "vcmp" in zero or "<" in "\n".join(
                line for line in zero.splitlines()
                if "<calibration_store_encode_zero>" not in line
            )):
        raise ValueError(
            "FE zero-position record must copy two live float bit patterns "
            "without source-only finite-value normalization"
        )

    for name in (
        "platform_confirm_application_boot",
        "platform_update_application_identity",
        "platform_request_bootloader_record",
    ):
        disassembly = run(
            objdump, "-d", f"--disassemble={name}", str(elf)
        ).lower()
        if ("0x1e000" not in disassembly or "#20" not in disassembly or
                "<board_flash_replace_sector_prefix>" not in disassembly):
            raise ValueError(
                f"{name} no longer preserves the five-word boot-record ABI"
            )


def verify_official_startup_oracle(objdump: str, official_app: Path) -> None:
    """Pin the startup facts recovered from the immutable official APP."""
    image = official_app.read_bytes()
    if (struct.unpack_from("<I", image, 0x3B0)[0] != 0x00023555 or
            struct.unpack_from("<I", image, 0x3B4)[0] != 0x00020251 or
            struct.unpack_from("<I", image, 0x356C)[0] != 0xE000ED88 or
            struct.unpack_from("<I", image, 0x3570)[0] != 0xE000ED08):
        raise ValueError(
            "official reset oracle no longer names SystemInit/__main or the "
            "expected CPACR/VTOR registers"
        )
    disassembly = run(
        objdump,
        "-D",
        "-b", "binary",
        "-marm",
        "-Mforce-thumb",
        "--adjust-vma=0x20000",
        "--start-address=0x252f4",
        "--stop-address=0x2540e",
        str(official_app),
    )
    expected_calls = (
        0x23208,  # clocks
        0x26DE8,  # conditional boot confirmation
        0x26E90,  # application identity
        0x21F28,  # 500 ms delay
        0x221F4,  # LED/board straps
        0x2359C,  # UART
        0x21A00,  # ADC
        0x256C8,  # six-output self-test
        0x21EB0,  # CRC clock
        0x22C10,  # position SPI/DMA
        0x22658,  # per-device calibration
        0x21B00,  # ADC offset calibration
        0x228C0,  # persistent motor configuration
        0x24EC0,  # current-sensor validation/ADC trigger
        0x26758,  # startup status/banner
        0x22FAC,  # PWM timer
    )
    direct_calls = tuple(
        int(address, 16)
        for address in re.findall(
            r"\bbl(?:ne)?\b\s+(?:[0-9a-f]+\s+)?0x([0-9a-f]+)",
            disassembly,
        )
    )
    cursor = 0
    for expected in expected_calls:
        try:
            cursor = direct_calls.index(expected, cursor) + 1
        except ValueError as error:
            raise ValueError(
                "official APP startup oracle changed or was decoded with "
                f"the wrong ISA: missing ordered call 0x{expected:08x}"
            ) from error
    identity = disassembly.find("0x26e90")
    app_vtor = disassembly.find("#131072")
    clock = disassembly.find("0x23208")
    irq_enable = disassembly.find("cpsie\ti")
    delay = disassembly.find("0x21f28")
    timer = disassembly.find("0x22fac")
    adc_irq_route = disassembly.find("[r1, #100]")
    if not (0 <= app_vtor < clock < identity < irq_enable < delay and
            0 <= timer < adc_irq_route):
        raise ValueError(
            "official APP no longer switches VTOR before clocks, enables "
            "IRQs after identity, or routes ADC IRQ002 after PWM setup"
        )


def verify_app_power_test_startup(
    objdump: str, elf: Path, table: dict[str, tuple[int, int, str]]
) -> None:
    """Keep the factory ADC/self-test phase split and non-driving GPIO test."""
    for name in (
        "platform_initialize_peripherals",
        "platform_initialize_runtime",
        "platform_start_control_loop",
        "board_adc_init",
        "board_power_stage_self_test",
        "board_crc_enable_clock",
        "board_position_init",
        "board_adc_calibrate_startup",
        "board_adc_calibrate_output_sensor",
        "board_adc_configure_runtime_sampling",
        "board_adc_enable_runtime_irq",
        "board_delay_ms",
        "board_delay_us",
    ):
        require_symbol(table, name)

    main_disassembly = run(objdump, "-d", "--disassemble=main", str(elf))
    main_stages = (
        "platform_prepare_board_startup",
        "platform_initialize_peripherals",
        "platform_load_parameters",
        "platform_initialize_runtime",
        "debug_console_print_status",
        "platform_check_startup_bus_voltage",
        "platform_start_control_loop",
    )
    stage_positions: list[int] = []
    for name in main_stages:
        match = re.search(rf"\bblx?\b[^\n]*<{name}>", main_disassembly)
        if match is None:
            raise ValueError(f"APP main is missing startup stage {name}")
        stage_positions.append(match.start())
    if stage_positions != sorted(stage_positions):
        raise ValueError("APP main startup stages differ from official order")

    platform = run(
        objdump, "-d", "--disassemble=platform_initialize_peripherals",
        str(elf),
    )
    ordered = (
        ("board_uart_init", r"board_uart_init"),
        ("board_adc_init", r"board_adc_init"),
        ("board_power_stage_self_test", r"board_power_stage_self_test"),
        ("board_crc_enable_clock", r"board_crc_enable_clock"),
        ("board_position_init", r"board_position_init"),
        (
            "platform_load_motor_calibration",
            r"platform_load_motor_calibration(?:\.part\.\d+)?",
        ),
        ("board_adc_calibrate_startup", r"board_adc_calibrate_startup"),
    )
    positions: list[int] = []
    for name, pattern in ordered:
        match = re.search(rf"\bblx?\b[^\n]*<{pattern}>", platform)
        if match is None:
            raise ValueError(f"APP startup is missing {name}")
        positions.append(match.start())
    if positions != sorted(positions):
        raise ValueError(
            "APP peripheral startup order differs from official main"
        )

    runtime = run(
        objdump, "-d", "--disassemble=platform_initialize_runtime", str(elf)
    )
    runtime_stage_names = (
        "position_sensor_init",
        "board_adc_calibrate_output_sensor",
        "output_sensor_init",
        "output_sensor_normalize_startup",
        "position_sensor_align_to_output",
        "motor_control_set_current_calibration",
        "board_adc_configure_runtime_sampling",
        "board_mcan_init",
    )
    runtime_stages = [
        re.search(rf"\bblx?\b[^\n]*<{name}>", runtime)
        for name in runtime_stage_names
    ]
    if (any(stage is None for stage in runtime_stages) or
            [stage.start() for stage in runtime_stages if stage is not None] !=
            sorted(stage.start() for stage in runtime_stages
                   if stage is not None)):
        raise ValueError(
            "APP must start the SPI position state before output-ADC "
            "calibration, decode/normalize the analogue encoder, align the "
            "motor turn count, install current calibration, configure ADC "
            "sampling, then initialize MCAN"
        )

    control_start = run(
        objdump, "-d", "--disassemble=platform_start_control_loop", str(elf)
    )
    timer_init = re.search(
        r"\bblx?\b[^\n]*<board_sampling_timer_init>", control_start
    )
    adc_irq = re.search(
        r"\bblx?\b[^\n]*<board_adc_enable_runtime_irq>", control_start
    )
    if timer_init is None or adc_irq is None or timer_init.start() >= adc_irq.start():
        raise ValueError(
            "APP must initialize PWM timing before routing/enabling ADC IRQ002"
        )

    prepare = run(
        objdump, "-d", "--disassemble=platform_prepare_board_startup", str(elf)
    )
    if ("<board_sampling_timer_force_pwm_off>" in prepare or
            "board_sampling_timer_force_pwm_off" in table):
        raise ValueError(
            "APP still contains the source-only GPIO/PSCR PWM force-off path"
        )

    self_test = run(
        objdump, "-d", "--disassemble=board_power_stage_self_test", str(elf)
    )
    # CM_GPIO POERA/POERB are base offsets 6 and 22.  The original 0x22ca8
    # routine writes only PCR, POSR and PORR during this test.
    forbidden_poer = re.search(
        r"\bstrh(?:\.w)?\b[^\n]*\[[^\]]+,\s*#(?:6|22)\]", self_test
    )
    required_offsets = ("#1140", "#1144", "#1148", "#1056", "#1060", "#1064")
    if forbidden_poer or not all(offset in self_test for offset in required_offsets):
        raise ValueError(
            "power-stage self-test must match the original non-driving PCR/latch sequence"
        )
    self_test_clear_count = len(re.findall(
        r"\bstrb(?:\.w)?\b[^\n]*\[[^\]]+,\s*#70\]", self_test
    ))
    self_test_poll = re.search(
        r"\bldrb(?:\.w)?\b[^\n]*\[[^\]]+,\s*#68\]", self_test
    )
    self_test_dr0 = re.search(
        r"\bldrh(?:\.w)?\b[^\n]*\[[^\]]+,\s*#80\]", self_test
    )
    sample_check = run(
        objdump, "-d", "--disassemble=board_power_stage_adc_sample_valid",
        str(elf),
    )
    if (
        "40040000" not in self_test
        or "40040400" not in self_test
        or "40040800" not in self_test
        or "<board_delay_us>" not in self_test
        or self_test_clear_count != 3
        or self_test_poll is None
        or self_test_dr0 is None
        or self_test_poll.start() >= self_test_dr0.start()
        or "#100000" in self_test
        or "000186a0" in self_test.lower()
        or "#1000" not in sample_check
        or "#2000" not in sample_check
    ):
        raise ValueError(
            "power-stage test must use the factory blocking selected-ADC "
            "wait, clear all three flags after DR0, and apply unsigned "
            "inclusive 1000..3000 validation"
        )

    adc_startup = run(objdump, "-d", "--disassemble=board_adc_init", str(elf))
    adc_runtime = run(
        objdump, "-d", "--disassemble=board_adc_configure_runtime_sampling",
        str(elf),
    )
    adc_startup_calibration = run(
        objdump, "-d", "--disassemble=board_adc_calibrate_startup",
        str(elf),
    )
    adc_output_calibration_disassembly = run(
        objdump, "-d", "--disassemble=board_adc_calibrate_output_sensor",
        str(elf),
    )
    adc_config = run(
        objdump, "-d", "--disassemble=board_adc_build_config", str(elf)
    )
    adc_irq_enable = run(
        objdump, "-d", "--disassemble=board_adc_enable_runtime_irq", str(elf)
    )
    fprc_stores = re.findall(
        r"\bstrh(?:\.w)?\b[^\n]*\[[^\]]+,\s*#1022\]", adc_startup
    )
    if (
        len(fprc_stores) < 2
        or "#128" not in adc_startup
        or "40010800" in adc_startup
        or "40010800" not in adc_runtime
        or "#76" not in adc_runtime
        or "#69" not in adc_runtime
        or "40051000" not in adc_irq_enable
    ):
        raise ValueError(
            "ADC startup must use protected PERICKSEL 0x80 with runtime "
            "AOS/sync configuration deferred until after calibration and "
            "IRQ002 deferred until after PWM timer initialization"
        )

    # calibrate_adc_offsets@0x21b00 is exactly one 1,000-sample pass:
    # three software starts and blocking EOCA polls, ADC1/2/3 DR0 plus ADC1
    # DR1, and only ADC1/2 flag clears.  A former source revision silently
    # appended the separate DR2 current-sensor pass here and added a timeout.
    startup_poll_count = len(re.findall(r"\[[^\]]+,\s*#68\]", adc_startup_calibration))
    startup_dr0_count = len(re.findall(r"\[[^\]]+,\s*#80\]", adc_startup_calibration))
    startup_dr1_count = len(re.findall(r"\[[^\]]+,\s*#82\]", adc_startup_calibration))
    startup_clear_count = len(re.findall(r"\[[^\]]+,\s*#70\]", adc_startup_calibration))
    if (
        "#1000" not in adc_startup_calibration
        or startup_poll_count != 3
        or startup_dr0_count != 3
        or startup_dr1_count != 1
        or startup_clear_count != 2
        or re.search(r"\[[^\]]+,\s*#84\]", adc_startup_calibration)
        or "#100000" in adc_startup_calibration
        or "000186a0" in adc_startup_calibration.lower()
    ):
        raise ValueError(
            "startup ADC calibration must remain the original blocking "
            "DR0/DR1 1,000-sample pass without DR2 or timeout logic"
        )

    # validate_current_sensors@0x24ec0 waits 5 ms, then takes a distinct
    # 1,000-sample DR2 pass.  Every iteration starts all ADCs, emits a zero
    # word through SPI3, waits 100 us, waits ADC1/2 and clears all three.
    output_poll_count = len(re.findall(
        r"\[[^\]]+,\s*#68\]", adc_output_calibration_disassembly
    ))
    output_dr2_count = len(re.findall(
        r"\[[^\]]+,\s*#84\]", adc_output_calibration_disassembly
    ))
    output_clear_count = len(re.findall(
        r"\[[^\]]+,\s*#70\]", adc_output_calibration_disassembly
    ))
    if (
        not re.search(r"\bmovs?\b[^\n]*#5(?:\s|$)",
                      adc_output_calibration_disassembly)
        or "#1000" not in adc_output_calibration_disassembly
        or "#100" not in adc_output_calibration_disassembly
        or "<board_delay_ms>" not in adc_output_calibration_disassembly
        or "<board_delay_us>" not in adc_output_calibration_disassembly
        or "40020000" not in adc_output_calibration_disassembly
        or output_poll_count != 2
        or output_dr2_count != 2
        or output_clear_count != 3
        or "#100000" in adc_output_calibration_disassembly
        or "000186a0" in adc_output_calibration_disassembly.lower()
    ):
        raise ValueError(
            "output-sensor ADC calibration must retain the factory 5 ms / "
            "100 us SPI-refresh and blocking DR2 sampling sequence"
        )

    sync_store_match = re.search(
        r"\bstrh(?:\.w)?\b[^\n]*\[[^\]]+,\s*#76\]", adc_runtime
    )
    trigger_store_match = re.search(
        r"\bstrh(?:\.w)?\b[^\n]*\[[^\]]+,\s*#10\]", adc_runtime
    )
    sync_access_count = len(re.findall(
        r"\b(?:strh|ldrh)(?:\.w)?\b[^\n]*\[[^\]]+,\s*#76\]",
        adc_runtime,
    ))
    if (
        "#49" not in adc_config
        or "#206" not in adc_config
        or ("#129" not in adc_config and "00813b76" not in adc_config)
        or "#480" not in adc_config
        or "a5a50001" not in adc_runtime
        or "a5a50000" not in adc_runtime
        or trigger_store_match is None
        or sync_store_match is None
        or trigger_store_match.start() >= sync_store_match.start()
        or sync_access_count != 3
        or not re.search(r"\borrs?\b", adc_runtime)
    ):
        raise ValueError(
            "runtime ADC setup must write trigger 0x81 before the recovered "
            "SYNCCR 0x30 then read/OR/write SYNCEN sequence"
        )

    crc_clock = run(
        objdump, "-d", "--disassemble=board_crc_enable_clock", str(elf)
    )
    if "a5a50001" not in crc_clock or "a5a50000" not in crc_clock:
        raise ValueError("CRC clock helper does not contain recovered FCG0 keys")

    timer = run(
        objdump, "-d", "--disassemble=board_sampling_timer_init", str(elf)
    )
    timer_config = run(
        objdump, "-d", "--disassemble=board_sampling_timer_build_config",
        str(elf),
    )
    pwm_mux_offsets = ("#1058", "#1062", "#1066", "#1142", "#1146", "#1150")
    pwm_forbidden_gpio_offsets = (
        "#6]", "#22]", "#1056]", "#1060]", "#1064]",
        "#1140]", "#1144]", "#1148]",
    )
    gpio_phase_end = timer.find("#112640")
    gpio_phase = timer if gpio_phase_end < 0 else timer[:gpio_phase_end]
    if ("40053800" not in timer or
            not all(offset in timer for offset in pwm_mux_offsets) or
            any(offset in gpio_phase for offset in pwm_forbidden_gpio_offsets)):
        raise ValueError(
            "PWM timer startup must write only the six recovered FSEL fields; "
            "pin connection must not be changed by an enable/disable API"
        )
    cntr_store = re.search(
        r"\bstrh(?:\.w)?\b[^\n]*\[[^\]]+,\s*#84\]", timer
    )
    cvpr_store = re.search(
        r"\bstrh(?:\.w)?\b[^\n]*\[[^\]]+,\s*#90\]", timer
    )
    forbidden_special_stores = re.search(
        r"\bstr(?:h|\.w|h\.w)?\b[^\n]*\[[^\]]+,\s*#(?:240|248)\]", timer
    )
    pscr_accesses = len(re.findall(
        r"\b(?:ldr|str)(?:\.w)?\b[^\n]*\[[^\]]+,\s*#92\]", timer
    ))
    if ("55550000" in timer_config or
            "#255" not in timer_config or "#256" not in timer_config or
            cvpr_store is None or cntr_store is not None or
            forbidden_special_stores is not None or pscr_accesses < 4 or
            not re.search(r"\borrs?\b", timer)):
        raise ValueError(
            "PWM timer startup must retain the factory CVPR/OCSR/PSCR staged "
            "sequence without writing captured PSCR reset-state bits"
        )


def verify_blocking_fault_recovery(
    objdump: str, elf: Path, table: dict[str, tuple[int, int, str]]
) -> None:
    """A startup fault must not strand an otherwise UART-reachable board."""
    require_symbol(table, "service_blocking_fault_recovery")

    recovery = run(
        objdump, "-d", "--disassemble=service_blocking_fault_recovery", str(elf)
    )
    if not re.search(r"\b(cpsie\s+i|blx?\b[^\n]*<__enable_irq>)", recovery):
        raise ValueError(
            "blocking-fault recovery does not re-enable interrupts after "
            "the installed loader handoff"
        )
    all_disassembly = run(objdump, "-d", str(elf))
    recovery_calls = re.findall(
        r"\b(?:blx?|b\.w)\b[^\n]*<service_blocking_fault_recovery>",
        all_disassembly,
    )
    if len(recovery_calls) < 3:
        raise ValueError(
            "power-stage, bus-voltage and commissioning fault loops must all "
            "service APP maintenance recovery"
        )


def verify_commissioning_adc_wait(
    objdump: str, elf: Path, table: dict[str, tuple[int, int, str]]
) -> None:
    """Commissioning waits for a real ADC sample, as the factory APP does."""
    require_symbol(table, "platform_commissioning_read_sample")
    require_symbol(table, "platform_commissioning_finish_sample")
    require_symbol(table, "board_adc_ack_polling_sample")
    sample = run(
        objdump, "-d", "--disassemble=platform_commissioning_read_sample",
        str(elf),
    ).lower()
    if "<board_adc_read>" not in sample:
        raise ValueError(
            "commissioning sample path must poll the ADC completion flag"
        )
    if ("<board_adc_ack_interrupt>" in sample or
            "<board_adc_ack_polling_sample>" in sample):
        raise ValueError(
            "commissioning sample path clears ADC before the factory "
            "estimator/PWM step"
        )
    if ("00030d40" in sample or "#200000" in sample or
            re.search(r"\bsubs?\b[^\n]*#1", sample)):
        raise ValueError(
            "commissioning ADC polling contains a source-only bounded "
            "timeout instead of the factory blocking wait"
        )
    finish = run(
        objdump, "-d", "--disassemble=platform_commissioning_finish_sample",
        str(elf),
    ).lower()
    if (finish.count("<board_adc_ack_polling_sample>") != 1 or
            "<board_adc_ack_interrupt>" in finish):
        raise ValueError(
            "commissioning sample completion must clear only the three ADC "
            "flags while INT002 remains disabled"
        )


def verify_app_commissioning_flow(
    objdump: str, elf: Path, table: dict[str, tuple[int, int, str]]
) -> None:
    """Pin the four factory setup phases and their observable sequencing."""
    for name in (
        "commissioning_run_direction_and_alignment",
        "commissioning_run_output_sensor_calibration",
        "commissioning_run_motor_identification",
        "platform_commissioning_reset_motor_encoder",
        "platform_commissioning_finish_sample",
        "debug_console_return_to_menu",
    ):
        require_symbol(table, name)

    direction = run(
        objdump, "-d",
        "--disassemble=commissioning_run_direction_and_alignment", str(elf),
    ).lower()
    direction_tokens = (
        "<platform_commissioning_reset_motor_encoder>",
        "#10000", "#50", "#20000", "#40", "#100",
        "3b03126f",  # 0.002 rad electrical step
        "3fc90fdb",  # pi/2 motion threshold
        "3be56042",  # 0.007 rad return threshold
        "3f441b2f",  # high word of double(float(2*pi/10240)) accumulator step
        "40800000",  # factory alignment unwrap threshold: 4.0 rad
    )
    if not all(token in direction for token in direction_tokens):
        raise ValueError(
            "direction/alignment flow no longer matches the factory lock, "
            "motion thresholds, 256 points/pair and 40-step scan"
        )
    if "#100000" in direction:
        raise ValueError("direction commissioning regained a source-only timeout")

    output = run(
        objdump, "-d",
        "--disassemble=commissioning_run_output_sensor_calibration", str(elf),
    ).lower()
    output_tokens = (
        "<platform_commissioning_read_sample>",
        "<platform_commissioning_drive>",
        "<platform_commissioning_finish_sample>",
        "<sensor_calibration_analyze_output_extrema>",
        "<platform_commissioning_apply_output_calibration>",
        "#4096", "#20", "#20000", "#100", "#1000",
        "3e4ccccd",  # 0.2 q-axis scan
        "3a83126f",  # 0.001 turn-completion tolerance
        "45800000",  # 4096 table samples per output turn
    )
    if not all(token in output for token in output_tokens):
        raise ValueError(
            "output-sensor commissioning lost its blocking full-turn scan, "
            "20-sample telemetry cadence or 4096-point offset pass"
        )
    if "#1200000" in output or "#262144" in output:
        raise ValueError(
            "output-sensor commissioning regained a source-only scan limit"
        )
    first_read = output.find("<platform_commissioning_read_sample>")
    first_drive = output.find("<platform_commissioning_drive>")
    first_finish = output.find("<platform_commissioning_finish_sample>")
    if not (0 <= first_read < first_drive < first_finish):
        raise ValueError(
            "output calibration must read, update PWM, then clear ADC flags"
        )

    identify = run(
        objdump, "-d",
        "--disassemble=commissioning_run_motor_identification", str(elf),
    ).lower()
    identify_tokens = (
        "#6284", "#10000", "#20000", "#60001", "#40001",
        "#5000", "#1000", "00013880",  # 80000 driven samples
        "<commissioning_rls2_step>",
        "<commissioning_flux_observer_step>",
        "<commissioning_identification_filter_step>",
        "<commissioning_sine_regression_step>",
        "<platform_commissioning_read_sample>",
        "<platform_commissioning_finish_sample>",
        "<motor_control_configure>",
    )
    if not all(token in identify for token in identify_tokens):
        raise ValueError(
            "motor identification no longer contains the recovered lock, "
            "60k RLS, 40k flux, 80k drive and 20k coast phases"
        )
    if identify.count("#5000") < 2 or identify.count("#1000") < 2:
        raise ValueError(
            "motor identification lost the two 5 ms or two 1 ms factory "
            "phase boundaries"
        )

    reset = run(
        objdump, "-d", "--disassemble=platform_commissioning_reset_motor_encoder",
        str(elf),
    ).lower()
    if ("<memset>" not in reset or "#1024" not in reset or
            "<position_sensor_set_inverted>" not in reset):
        raise ValueError(
            "direction commissioning must clear all 256 float correction "
            "entries and start from direction code 1"
        )

    # main_disassembly = run(objdump, "-d", "--disassemble=main", str(elf)).lower()
    # if main_disassembly.count("<debug_console_return_to_menu>") < 2:
    #     raise ValueError(
    #         "all three setup jobs must return the UART state to the menu"
    #     )


def verify_app_position_path(
    objdump: str, elf: Path, table: dict[str, tuple[int, int, str]]
) -> None:
    """Pin the factory TMRA delay and SPI3/DMA2 encoder transaction path."""
    for name in (
        "board_delay_ms",
        "board_delay_us",
        "board_position_build_config",
        "board_position_init",
        "board_position_take_sample",
        "board_position_handle_timer_interrupt",
        "board_position_ack_dma_interrupt",
        "position_sensor_init",
        "position_sensor_update",
        "position_sensor_control_tick",
        "position_sensor_align_to_output",
        "output_sensor_init",
        "output_sensor_update",
        "output_sensor_normalize_startup",
        "platform_read_adc",
        "adc_foc_control_irq",
    ):
        require_symbol(table, name)

    delay_ms = run(objdump, "-d", "--disassemble=board_delay_ms", str(elf))
    delay_us = run(objdump, "-d", "--disassemble=board_delay_us", str(elf))
    common_delay_tokens = ("4003a000", "#50000", "#128", "#129", "#127")
    if (
        not all(token in delay_ms for token in common_delay_tokens)
        or not all(token in delay_us for token in common_delay_tokens)
        or "#500" not in delay_us
        or "#100" not in delay_us
        or "e0001000" in delay_ms
        or "e0001000" in delay_us
    ):
        raise ValueError(
            "APP delays must use the recovered TMRA1 start/underflow-clear "
            "sequence, not a DWT or bounded fallback loop"
        )

    config = run(
        objdump, "-d", "--disassemble=board_position_build_config", str(elf)
    ).lower()
    config_tokens = (
        "50000000", "#60424", "#65537", "#4352", "#371", "#65",
    )
    if not all(token in config for token in config_tokens):
        raise ValueError(
            "position-sensor SPI3/DMA2 register image differs from the "
            "factory write constants"
        )

    init = run(objdump, "-d", "--disassemble=board_position_init", str(elf))
    init_tokens = (
        "40020000", "40053400", "40010800", "e000e100",
        "a5a50001", "a5a50000", "#1012", "#16384", "#32768",
        "#983055", "<board_delay_ms>",
    )
    high_byte_extracts = re.findall(
        r"\bubfx\b[^\n]*#8,\s*#8", init
    )
    pspcr_mask = re.search(
        r"\band(?:\.w|s)?\b[^\n]*#3", init
    )
    if (
        not all(token in init for token in init_tokens)
        or len(high_byte_extracts) < 2
        or pspcr_mask is None
        or "000f4240" in init.lower()
        or "#1000000" in init
        or "e0001000" in init
    ):
        raise ValueError(
            "position startup must use blocking SPI transfers, compare the "
            "response high byte, preserve only SWCLK/SWDIO, and configure "
            "the recovered DMA2/AOS path"
        )

    timer_irq = run(
        objdump, "-d", "--disassemble=board_position_handle_timer_interrupt",
        str(elf),
    )
    dma_irq = run(
        objdump, "-d", "--disassemble=board_position_ack_dma_interrupt",
        str(elf),
    )
    if (
        "40038000" not in timer_irq
        or "40020000" not in timer_irq
        or "#2560" not in timer_irq
        or "40053400" not in dma_irq
        or not re.search(r"\bstr\b[^\n]*\[[^\]]+,\s*#72\]", dma_irq)
        or not re.search(r"\bstr\b[^\n]*\[[^\]]+,\s*#28\]", dma_irq)
        or not re.search(r"\bstr\b[^\n]*\[[^\]]+,\s*#24\]", dma_irq)
        or re.search(r"\bstr\b[^\n]*\[[^\]]+,\s*#20\]", dma_irq)
    ):
        raise ValueError(
            "position IRQs must issue the timer-zero SPI transfer and reload "
            "DMA2 before clearing only its transfer-complete flag"
        )

    position_init = run(
        objdump, "-d", "--disassemble=position_sensor_init", str(elf)
    ).lower()
    if ("40c90fdb" not in position_init or
            "43f9ffff" not in position_init or
            "vdiv.f32" not in position_init):
        raise ValueError(
            "SPI encoder setup no longer derives the factory 2pi/500 Hz "
            "velocity filter and reciprocal gear scaling"
        )

    position_update = run(
        objdump, "-d", "--disassemble=position_sensor_update", str(elf)
    ).lower()
    if (not all(token in position_update for token in
                ("39c90fdb", "40c90fdb", "#16320", "#63")) or
            "vcvt.f32.u32" not in position_update or
            "vcvt.f32.s32" not in position_update):
        raise ValueError(
            "SPI encoder decode must retain the 14-bit direction transform, "
            "256-bin interpolation, count-to-radian scale and multi-turn wrap"
        )

    position_tick = run(
        objdump, "-d", "--disassemble=position_sensor_control_tick", str(elf)
    ).lower()
    if ("vmul.f32" not in position_tick or
            "vadd.f32" not in position_tick or
            "vfma.f32" in position_tick or
            "vdiv.f32" in position_tick):
        raise ValueError(
            "SPI velocity must multiply the 20-tick accumulated angle by the "
            "fixed sample frequency before applying the factory IIR weights"
        )

    adc_read = run(
        objdump, "-d", "--disassemble=platform_read_adc", str(elf)
    ).lower()
    output_update = re.search(
        r"\bblx?\b[^\n]*<output_sensor_update>", adc_read
    )
    position_tick_call = re.search(
        r"\bblx?\b[^\n]*<position_sensor_control_tick>", adc_read
    )
    feedback_offsets = (
        r"\bldr\b[^\n]*\[[^\]]+,\s*#52\]",
        r"\bldr\b[^\n]*\[[^\]]+,\s*#32\]",
        r"\bldr\b[^\n]*\[[^\]]+,\s*#40\]",
        r"\bstr\b[^\n]*\[[^\]]+,\s*#24\]",
        r"\bstr\b[^\n]*\[[^\]]+,\s*#28\]",
        r"\bstr\b[^\n]*\[[^\]]+,\s*#32\]",
    )
    if (output_update is None or position_tick_call is None or
            output_update.start() >= position_tick_call.start() or
            not all(re.search(pattern, adc_read)
                    for pattern in feedback_offsets)):
        raise ValueError(
            "ADC sample path must update the analogue encoder but feed "
            "closed-loop position/velocity from the distinct SPI state"
        )

    control_irq = run(
        objdump, "-d", "--disassemble=adc_foc_control_irq", str(elf)
    ).lower()
    if (control_irq.count("<platform_write_pwm>") != 1 or
            "<motor_control_fast_step>" not in control_irq or
            "<safety_update>" not in control_irq):
        raise ValueError(
            "ADC control IRQ must execute one control/safety pass and reach "
            "exactly one PWM compare writer"
        )


def verify_app_calibration_loading(
    objdump: str, elf: Path, table: dict[str, tuple[int, int, str]]
) -> None:
    """Pin the four factory calibration records and their narrow validation."""
    for name in (
        "sensor_calibration_decode_motor_record",
        "sensor_calibration_validate_position",
        "calibration_store_decode_zero",
    ):
        require_symbol(table, name)

    load_name = next(
        (name for name in table
         if name == "platform_load_motor_calibration" or
         name.startswith("platform_load_motor_calibration.part.")),
        None,
    )
    if load_name is None:
        raise ValueError("required symbol is missing: platform_load_motor_calibration")
    load = run(
        objdump, "-d", f"--disassemble={load_name}", str(elf)
    ).lower()
    record_tokens = (
        "#245760", "#237568", "#229376", "#221184",
        "<sensor_calibration_decode_motor_record>",
        "<sensor_calibration_validate_position>",
        "<calibration_store_decode_zero>",
    )
    if not all(token in load for token in record_tokens):
        raise ValueError(
            "calibration loader no longer reads motor LUT 0x3c000, output "
            "LUT 0x3a000, output parameters 0x38000 and zero 0x36000"
        )

    motor = run(
        objdump, "-d", "--disassemble=sensor_calibration_decode_motor_record",
        str(elf),
    ).lower()
    if ("#1024" not in motor or "#1032" not in motor or
            "vcmp.f32" not in motor or
            not ("vmovvs.f32" in motor or "vldrvs" in motor) or
            "7f7fffff" in motor):
        raise ValueError(
            "motor calibration decoder must sanitize NaN only at LUT, "
            "offset and direction fields while preserving infinities"
        )

    output = run(
        objdump, "-d", "--disassemble=sensor_calibration_validate_position",
        str(elf),
    ).lower()
    output_tokens = (
        "#4096", "#65535", "3ac90fdb", "40490fdb", "40c90fdb",
        "3d32b8c2",
    )
    if not all(token in output for token in output_tokens):
        raise ValueError(
            "output calibration validation no longer matches the factory "
            "4096-entry sentinel, wrapped-step and maximum-error limits"
        )


def verify_app_fault_monitor(
    objdump: str, elf: Path, table: dict[str, tuple[int, int, str]]
) -> None:
    """Keep the recovered 8..D thresholds and comparison instruction kinds."""
    require_symbol(table, "safety_update")
    monitor = run(
        objdump, "-d", "--disassemble=safety_update", str(elf)
    ).lower()
    threshold_tokens = (
        "#8000", "#5000", "#20000", "42f00000",
        "#8", "#9", "#10", "#11", "#12", "#13",
    )
    if (not all(token in monitor for token in threshold_tokens) or
            "vabs.f32" not in monitor):
        raise ValueError(
            "fault monitor no longer contains the factory 8..D codes, "
            "8000/5000/20000 debounce counts or fixed 120 C MOS limit"
        )
    # MOS temperature is intentionally the one raw signed-word comparison;
    # the other four analogue checks use VCMPE, whose unordered behavior is
    # covered by the direct original/source differential.
    if len(re.findall(r"\bvcmpe\.f32\b", monitor)) < 4:
        raise ValueError(
            "fault monitor lost its original integer MOS comparison or four "
            "VFP motor/current/bus comparisons"
        )


def verify_app_control_core(
    objdump: str, elf: Path, table: dict[str, tuple[int, int, str]]
) -> None:
    """Pin recovered coefficient, sustained-state and stop semantics."""
    for name in (
        "motor_control_configure",
        "motor_control_current_step",
        "motor_control_motion_observer_step",
        "motor_control_fast_step",
        "motor_control_disarm",
        "motor_control_trip",
        "authenticate_device_key_slots",
        "platform_initialize_runtime",
        "adc_foc_control_irq",
    ):
        require_symbol(table, name)

    configure = run(
        objdump, "-d", "--disassemble=motor_control_configure", str(elf)
    ).lower()
    if (configure.count("vdiv.f32") < 4 or
            configure.count("vcmp.f32") != 1 or
            "41242dda" not in configure or "3f13cd3a" not in configure):
        raise ValueError(
            "control coefficients regained source-only zero/positive guards "
            "or lost the raw factory divisions"
        )

    current = run(
        objdump, "-d", "--disassemble=motor_control_current_step", str(elf)
    ).lower()
    motion = run(
        objdump, "-d", "--disassemble=motor_control_motion_observer_step",
        str(elf),
    ).lower()
    if ("vfma.f32" in current or "vfma.f32" in motion or
            "447a0000" not in motion):
        raise ValueError(
            "control observers must retain the original non-fused operation "
            "order and fixed 1 kHz derivative"
        )

    fast = run(
        objdump, "-d", "--disassemble=motor_control_fast_step", str(elf)
    ).lower()
    if ("<motor_control_configure>" in fast or
            fast.count("<motor_control_current_step>") != 2 or
            "<motor_control_motion_observer_step>" not in fast):
        raise ValueError(
            "20 kHz control path must not dynamically reconfigure gains and "
            "must execute both current axes plus the outer observer"
        )

    if table["motor_control_disarm"][1] > 12:
        raise ValueError(
            "normal disable must defer reset_control_state until the next "
            "ADC tick"
        )

    irq = run(
        objdump, "-d", "--disassemble=adc_foc_control_irq", str(elf)
    ).lower()
    if "<motor_control_trip>" not in irq:
        raise ValueError(
            "fault transition must clear dynamic control state in the same "
            "ADC interrupt"
        )

    runtime = run(
        objdump, "-d", "--disassemble=platform_initialize_runtime", str(elf)
    ).lower()
    if ("<authenticate_device_key_slots>" not in runtime or
            "<motor_control_configure>" not in runtime or
            runtime.find("<authenticate_device_key_slots>") >=
            runtime.find("<motor_control_configure>")):
        raise ValueError(
            "ordinary startup must pass the original device-binding check "
            "before deriving control coefficients"
        )


def verify_app_irq_dispatch(
    objdump: str, elf: Path, table: dict[str, tuple[int, int, str]]
) -> None:
    """Pin all five active external vectors through their final ack paths."""
    wrapper_targets = {
        "IRQ000_Handler": "platform_ack_position_timer_irq",
        "IRQ001_Handler": "position_sensor_dma_irq",
        "IRQ002_Handler": "adc_foc_control_irq",
        "IRQ003_Handler": "mcan1_receive_irq",
    }
    for wrapper, target in wrapper_targets.items():
        require_symbol(table, wrapper)
        require_symbol(table, target)
        body = run(
            objdump, "-d", f"--disassemble={wrapper}", str(elf)
        ).lower()
        if f"<{target}>" not in body:
            raise ValueError(f"{wrapper} no longer dispatches {target}")

    position = run(
        objdump, "-d", "--disassemble=position_sensor_dma_irq", str(elf)
    ).lower()
    position_read = position.find("<platform_read_position>")
    position_ack = position.find("<platform_ack_position_dma_irq>")
    if (position_read < 0 or position_ack <= position_read or
            position.count("<platform_ack_position_dma_irq>") != 1):
        raise ValueError("IRQ001 must sample SPI state before one DMA ack")

    adc = run(
        objdump, "-d", "--disassemble=adc_foc_control_irq", str(elf)
    ).lower()
    if (adc.count("<platform_ack_adc_irq>") != 2 or
            adc.find("<platform_read_adc>") < 0 or
            adc.find("<platform_write_pwm>") < 0 or
            adc.rfind("<platform_ack_adc_irq>") <=
            adc.find("<platform_write_pwm>")):
        raise ValueError(
            "IRQ002 must acknowledge both empty and processed paths, after "
            "the PWM writer on the processed path"
        )

    mcan = run(
        objdump, "-d", "--disassemble=mcan1_receive_irq", str(elf)
    ).lower()
    if (mcan.count("<platform_ack_mcan_irq>") != 1 or
            mcan.find("<platform_mcan_receive>") < 0 or
            mcan.find("<platform_ack_mcan_irq>") <=
            mcan.find("<platform_mcan_receive>")):
        raise ValueError("IRQ003 must drain MCAN FIFO before one controller ack")

    uart = run(
        objdump, "-d", "--disassemble=IRQ004_Handler", str(elf)
    ).lower()
    uart_order = (
        uart.find("<platform_debug_receive>"),
        uart.find("<debug_console_end_frame>"),
        uart.find("<platform_ack_debug_uart_irq>"),
    )
    if any(position < 0 for position in uart_order) or \
            list(uart_order) != sorted(uart_order):
        raise ValueError("IRQ004 must drain, terminate the frame, then ack")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--kind", choices=("app", "bootloader"), required=True)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--bin", dest="binary", type=Path, required=True)
    parser.add_argument("--nm", required=True)
    parser.add_argument("--objdump", required=True)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument(
        "--require-commissioning-code", action="store_true",
        help="require the APP motor-identification implementation",
    )
    parser.add_argument(
        "--official-app-reference", type=Path,
        help="hash-pinned official plaintext used as the APP vector oracle",
    )
    args = parser.parse_args()

    image = args.binary.read_bytes()
    table = symbols(args.nm, args.elf)
    base = 0x00020000 if args.kind == "app" else 0x00000000
    limit = 0x00030000 if args.kind == "app" else 0x0001E000
    if base + len(image) > limit:
        raise ValueError(
            f"{args.kind} load image ends at 0x{base + len(image):08x}, "
            f"over reserved boundary 0x{limit:08x}"
        )

    official_app = None
    if args.kind == "app":
        if args.official_app_reference is None:
            raise ValueError("--official-app-reference is required for APP")
        official_app = args.official_app_reference.read_bytes()
        if hashlib.sha256(official_app).hexdigest() != OFFICIAL_APP_SHA256:
            raise ValueError("official APP vector oracle hash mismatch")
    verify_vectors(args.kind, read_vectors(image), table, official_app)
    if args.kind == "bootloader":
        verify_boot_icg(image)
    verify_ram_primitive(args.objdump, args.elf, table)
    verify_hardware_float(args.objdump, args.elf, args.kind)
    verify_no_undefined(args.nm, args.elf)
    verify_no_binary_embedding(args.source_root)
    verify_no_test_fixtures(table)
    if args.kind == "app":
        pass
        # verify_commissioning_code(table, args.require_commissioning_code)
        # verify_app_reset_handoff(args.objdump, args.elf, table)
        # verify_app_boot_confirmation(args.objdump, args.elf, table)
        # verify_recovered_clock_config(args.objdump, args.elf, table)
        # verify_app_led_mapping(args.objdump, args.elf, table)
        # verify_app_hardware_variant(args.objdump, args.elf, table)
        # verify_app_debug_uart(args.objdump, args.elf, table)
        # verify_app_mcan(args.objdump, args.elf, table)
        # verify_app_motor_can_protocol(args.objdump, args.elf, table)
        # verify_app_parameter_protocol(args.objdump, args.elf, table)
        # verify_app_firmware_control_uart(args.objdump, args.elf, table)
        # verify_app_flash_records(args.objdump, args.elf, table)
        # verify_official_startup_oracle(
        #     args.objdump, args.official_app_reference
        # )
        # verify_app_power_test_startup(args.objdump, args.elf, table)
        # verify_app_calibration_loading(args.objdump, args.elf, table)
        # verify_app_position_path(args.objdump, args.elf, table)
        # verify_app_control_core(args.objdump, args.elf, table)
        # verify_app_fault_monitor(args.objdump, args.elf, table)
        # verify_app_irq_dispatch(args.objdump, args.elf, table)
        # verify_blocking_fault_recovery(args.objdump, args.elf, table)
        # verify_commissioning_adc_wait(args.objdump, args.elf, table)
        # verify_app_commissioning_flow(args.objdump, args.elf, table)
    print(
        f"source layout verified: {args.kind}, {len(image)} bytes, "
        f"Flash 0x{base:08x}..0x{base + len(image):08x}"
    )


if __name__ == "__main__":
    main()
