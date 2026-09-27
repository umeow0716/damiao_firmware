#!/usr/bin/env python3
"""Fail-closed audit of a DM4310 APP package for the installed UART loader.

This tool deliberately separates facts that can be proven from repository
artifacts from observations that require the physical target.  A successful
exit means the encrypted payload, loader contract, image layout and early
boot-confirmation path passed offline verification.  It does not claim motor
or peripheral behavior has been validated on silicon.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import subprocess
from dataclasses import asdict, dataclass
from datetime import date
from pathlib import Path
from typing import Any

from pack_update import (
    APP_BASE,
    APP_END,
    CHUNK_SIZE,
    aes256_ctr_transform,
    build_update_records,
    crc8_maxim,
    extract_bootloader_material,
    load_update_profile,
    validate_plain_app,
)


ROOT = Path(__file__).resolve().parents[1]
REFERENCE_APP_SHA256 = (
    "65aab219268e9159b196d4578d3cd530e6fa90a731a82670a0d3d3be609b59d4"
)
BOOTLOADER_SHA256 = (
    "4bee47463774574683d1b5a96ba63b1f231cedcdc422d589acb230c0835fa463"
)
FACTORY_CIPHERTEXT_SHA256 = (
    "61f049d7f5da1a918af8fc8d47af1d3ba060e80ccc344f41c82859a09576817c"
)

# Fingerprints of only the reviewed functions that define UART parsing,
# update completion, loader handoff, and original APP confirmation.  The full
# image hashes above remain the primary identity check; these make the exact
# audited locations explicit in the generated report.
CRITICAL_FINGERPRINTS = (
    ("bootloader", "UART byte receiver/decrypt/write/completion", 0x00005C30, 296,
     "370a1bf4868828d258594f1e6db0d0115a35a2555c28848278e64aaa4e28a07c"),
    ("bootloader", "UART command/parser", 0x000064C4, 248,
     "547d1c44620abd9358db89f0f0873a072977a6fe028c5af48299d01c4a8670bf"),
    ("bootloader", "clear record before APP jump", 0x00006410, 28,
     "932a2337c7cf912fb6353d5e736c2a185059192fcc93d4827caff7b4f38c619e"),
    ("bootloader", "loader state machine", 0x0000684C, 648,
     "204e5372c2ea4660bf6d1f0d87ccec9453a7071e0c952760cf6d0abd37cc7f2b"),
    ("bootloader", "store update-complete record", 0x00006C34, 28,
     "d4afcc43afcdf7c2b7ab2dac7be4f3b320e3868e4b010f11332a9bee5dc6153b"),
    ("app", "original clock startup", 0x00023208, 554,
     "36aa0abab34db33d919cbee6203e792b3f5c03caa047af4610d3ccb57f369374"),
    ("app", "original ADC startup configuration", 0x00021A00, 226,
     "686f5c72c7d1199102beaa016f5cf2ded3bf257ee50a55443f89e4b354755a61"),
    ("app", "original six-output startup test", 0x00022CA8, 642,
     "8fb93d4894d04112a8b6ec31afce82ea8db5fcd9e4b945a3bc1d26f0fc2c4a29"),
    ("app", "original current validation and runtime ADC transition",
     0x00024EC0, 484,
     "4c384e872f8347945791afd4fbf495123406c5ac269b96284e93a659816c2042"),
    ("app", "original main startup", 0x000252F4, 720,
     "0b79a77776081f7acc4a3630822c0909ba6be69bb61b73043def488e24838f9a"),
    ("app", "original APP confirmation", 0x00026DE8, 34,
     "fcee8e46098377205764a73cfe748226cef1532c993686a8b179cbdfed05f655"),
    ("app", "original APP five-word confirmation Flash writer load image",
     0x00028CC0, 228,
     "cf949388887a222514c85a97a7fff956976e361f6404be1171520434f0613a50"),
)


@dataclass
class Check:
    id: str
    passed: bool
    detail: str


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def command_output(*command: str) -> str:
    result = subprocess.run(
        command,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    return result.stdout


def symbol_table(nm: str, elf: Path) -> dict[str, tuple[int, int]]:
    result: dict[str, tuple[int, int]] = {}
    for line in command_output(nm, "-n", "-S", "--defined-only", str(elf)).splitlines():
        fields = line.split()
        if len(fields) != 4:
            continue
        address, size, _kind, name = fields
        if re.fullmatch(r"[0-9a-fA-F]+", address):
            result[name] = (int(address, 16), int(size, 16))
    return result


def parse_record(record: bytes) -> tuple[int, bytes]:
    if len(record) < 6 or record[0] != ord("#") or record[2] != ord("#"):
        raise ValueError("malformed update-record header")
    length = struct.unpack_from("<H", record, 3)[0]
    if len(record) != length + 6:
        raise ValueError("update-record length field does not match record")
    payload = record[5:-1]
    if crc8_maxim(payload) != record[-1]:
        raise ValueError("update-record CRC-8/MAXIM mismatch")
    return record[1], payload


def add(checks: list[Check], check_id: str, condition: bool, detail: str) -> None:
    checks.append(Check(check_id, bool(condition), detail))


def verify_critical_fingerprints(
    checks: list[Check], bootloader: bytes, reference: bytes
) -> None:
    images = {
        "bootloader": (bootloader, 0),
        "app": (reference, APP_BASE),
    }
    for image_name, label, address, size, expected in CRITICAL_FINGERPRINTS:
        image, base = images[image_name]
        begin = address - base
        actual = digest(image[begin:begin + size])
        add(
            checks,
            f"fingerprint_{image_name}_{address:08x}",
            actual == expected,
            f"{label}: 0x{address:08x}, {size} bytes, SHA-256 {actual}",
        )


def verify_source_startup(
    checks: list[Check], elf: Path, objdump: str, nm: str,
    stack: int, reset: int,
) -> None:
    symbols = symbol_table(nm, elf)
    reset_symbol = symbols.get("Reset_Handler", (0, 0))[0] | 1
    add(
        checks,
        "candidate_reset_vector_symbol",
        reset == reset_symbol,
        f"vector reset 0x{reset:08x}; Reset_Handler 0x{reset_symbol:08x}",
    )
    add(
        checks,
        "candidate_stack_contract",
        0x1FFF8000 <= stack <= 0x20008000 and (stack & 7) == 0,
        f"initial MSP 0x{stack:08x}",
    )

    main_disassembly = command_output(
        objdump, "-d", "--disassemble=main", str(elf)
    )
    early = re.search(r"\bblx?\b[^\n]*<platform_early_init>", main_disassembly)
    confirm = re.search(
        r"\bblx?\b[^\n]*<platform_confirm_application_boot>",
        main_disassembly,
    )
    immediate = early is not None and confirm is not None and confirm.start() > early.end()
    between = "" if not immediate else main_disassembly[early.end():confirm.start()]
    immediate = immediate and not re.search(
        r"\b(?:blx?|cbz|cbnz|b(?:eq|ne|cs|cc|mi|pl|vs|vc|hi|ls|ge|lt|gt|le))"
        r"(?:\.w)?\b",
        between,
    )
    add(
        checks,
        "candidate_clock_to_confirm_path",
        immediate,
        "main calls clock-only early init and then boot confirmation with no call/conditional branch",
    )

    ordered_names = (
        "platform_confirm_application_boot",
        "platform_update_application_identity",
        "app_state_init",
        "debug_console_reset",
        "platform_prepare_board_startup",
        "platform_initialize_peripherals",
        "platform_load_parameters",
        "platform_initialize_runtime",
        "debug_console_print_status",
        "platform_check_startup_bus_voltage",
        "platform_start_control_loop",
    )
    positions: list[int] = []
    for name in ordered_names:
        match = re.search(rf"\bblx?\b[^\n]*<{name}>", main_disassembly)
        positions.append(-1 if match is None else match.start())
    add(
        checks,
        "candidate_post_confirm_order",
        all(position >= 0 for position in positions) and positions == sorted(positions),
        "confirmation/identity/IRQ handoff precedes the recovered peripheral, configuration, status, voltage and control-loop stages",
    )

    identity = re.search(
        r"\bblx?\b[^\n]*<platform_update_application_identity>",
        main_disassembly,
    )
    state_init = re.search(
        r"\bblx?\b[^\n]*<app_state_init>", main_disassembly
    )
    irq_window = "" if identity is None or state_init is None else \
        main_disassembly[identity.end():state_init.start()]
    add(
        checks,
        "candidate_early_global_irq_enable",
        re.search(r"\bcpsie\s+i\b", irq_window) is not None,
        "global IRQ enable follows application identity and precedes the 500 ms startup settling phase",
    )

    early_disassembly = command_output(
        objdump, "-d", "--disassemble=platform_early_init", str(elf)
    )
    early_callees = re.findall(
        r"\b(?:blx?|b\.w)\b[^\n]*<([^>]+)>", early_disassembly
    )
    add(
        checks,
        "candidate_preconfirm_side_effects",
        early_callees == ["board_clock_init"],
        f"pre-confirm call list: {early_callees}",
    )

    confirm_disassembly = command_output(
        objdump, "-d", "--disassemble=platform_confirm_application_boot",
        str(elf),
    )
    confirm_contract = (
        ("#122880" in confirm_disassembly or "0x1e000" in confirm_disassembly)
        and re.search(r"\bldr\b[^\n]*\[[^\]]+,\s*#4\]", confirm_disassembly)
        is not None
        and re.search(
            r"\bblx?\b[^\n]*<board_flash_replace_sector_prefix>",
            confirm_disassembly,
        )
        is not None
    )
    add(
        checks,
        "candidate_boot_record_commit",
        confirm_contract,
        "reads word 1 at 0x1e000, normalizes the 20-byte record to (0,1), and calls Flash commit",
    )

    identity_disassembly = command_output(
        objdump, "-d", "--disassemble=platform_update_application_identity",
        str(elf),
    )
    add(
        checks,
        "candidate_application_identity_commit",
        "07010005" in identity_disassembly
        and re.search(
            r"\bblx?\b[^\n]*<board_flash_replace_sector_prefix>",
            identity_disassembly,
        ) is not None,
        "boot-record word 3 is normalized to official application identity 0x07010005 through the SRAM Flash path",
    )

    ram_address, ram_size = symbols.get("erase_and_program_from_sram", (0, 0))
    ram_disassembly = command_output(
        objdump, "-d", "--disassemble=erase_and_program_from_sram", str(elf)
    )
    ram_ok = (
        0x1FFF8000 <= ram_address < 0x20008000
        and ram_address + ram_size <= 0x20008000
        and re.search(r"\bblx?\b", ram_disassembly) is None
    )
    add(
        checks,
        "candidate_flash_primitive_in_sram",
        ram_ok,
        f"erase/program primitive 0x{ram_address:08x}..0x{ram_address + ram_size:08x}; no external call",
    )
    key1_stores = re.findall(
        r"\bstr(?:\.w)?\b[^\n]*\[[^\]]+,\s*#4\]", ram_disassembly
    )
    key2_stores = re.findall(
        r"\bstr(?:\.w)?\b[^\n]*\[[^\]]+,\s*#8\]", ram_disassembly
    )
    add(
        checks,
        "candidate_flash_fwmc_unlock_sequence",
        len(key1_stores) >= 2 and not key2_stores,
        "compiled SRAM writer stores both FWMC unlock words to EFM KEY1 (+4) and never OTP KEY2 (+8)",
    )

    platform_disassembly = command_output(
        objdump, "-d", "--disassemble=platform_initialize_peripherals", str(elf)
    )
    startup_names = (
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
    startup_positions: list[int] = []
    for _, pattern in startup_names:
        match = re.search(rf"\bblx?\b[^\n]*<{pattern}>", platform_disassembly)
        startup_positions.append(-1 if match is None else match.start())
    add(
        checks,
        "candidate_adc_self_test_phase_order",
        all(position >= 0 for position in startup_positions)
        and startup_positions == sorted(startup_positions),
        "UART -> ADC -> six-output test -> CRC -> position -> calibration load -> ADC calibration",
    )

    prepare_disassembly = command_output(
        objdump, "-d", "--disassemble=platform_prepare_board_startup", str(elf)
    )
    add(
        checks,
        "candidate_no_pretest_pwm_forceoff",
        "<board_sampling_timer_force_pwm_off>" not in prepare_disassembly,
        "startup settling phase does not use the removed source-only GPIO force-off path",
    )

    power_test_disassembly = command_output(
        objdump, "-d", "--disassemble=board_power_stage_self_test", str(elf)
    )
    poer_store = re.search(
        r"\bstrh(?:\.w)?\b[^\n]*\[[^\]]+,\s*#(?:6|22)\]",
        power_test_disassembly,
    )
    pcr_offsets = ("#1140", "#1144", "#1148", "#1056", "#1060", "#1064")
    add(
        checks,
        "candidate_power_test_gpio_contract",
        poer_store is None
        and all(offset in power_test_disassembly for offset in pcr_offsets),
        "compiled startup test writes the six recovered PCRs/latches and never enables GPIO POERA/POERB",
    )

    adc_startup = command_output(
        objdump, "-d", "--disassemble=board_adc_init", str(elf)
    )
    adc_runtime = command_output(
        objdump, "-d", "--disassemble=board_adc_configure_runtime_sampling", str(elf)
    )
    adc_irq = command_output(
        objdump, "-d", "--disassemble=board_adc_enable_runtime_irq", str(elf)
    )
    fprc_stores = re.findall(
        r"\bstrh(?:\.w)?\b[^\n]*\[[^\]]+,\s*#1022\]", adc_startup
    )
    add(
        checks,
        "candidate_adc_register_phase_contract",
        len(fprc_stores) >= 2
        and "#128" in adc_startup
        and "40010800" not in adc_startup
        and "40010800" in adc_runtime
        and "#76" in adc_runtime
        and "#69" in adc_runtime
        and "40051000" in adc_irq,
        "PERICKSEL=0x80 is in ADC init, AOS/sync follows calibration, and IRQ002 routing is a separate post-PWM phase",
    )


def markdown_report(report: dict[str, Any]) -> str:
    checks = report["offline_checks"]
    passed = sum(1 for item in checks if item["passed"])
    lines = [
        "# DM4310 UART 更新套件稽核",
        "",
        f"生成日期：{report['generated_date']}",
        "",
        "## 結論",
        "",
        f"離線硬性檢查：**{passed}/{len(checks)} PASS**。",
        "",
        ("這份候選套件已通過原始 bootloader 身分、原廠密文重現、候選加解密、"
         "UART record、Flash 範圍、vector 與早期 boot-record confirmation 路徑檢查。"
         if report["offline_uart_package_ready"] else
         "離線硬性檢查有失敗；不得刷寫這份候選套件。"),
        "",
        "它仍不等於『實機已驗證』。只有裝置真的從 APP 啟動、停止輸出 `Upgrade failed`，"
        "而且下一次重開仍直接進 APP，才能把 UART handoff 標成通過。控制與功率行為另需"
        "通訊端逐項驗證。",
        "",
        "## 本次阻斷缺陷",
        "",
        "舊 source writer 把第二個 FWMC key 寫到 OTP `KEY2`，因此 APP 無法把 `0x1E000` "
        "confirmation 從 `(0,0)` 提交成 `(0,1)`。目前候選已改成和原 APP／DDL 相同的 "
        "`KEY1=0x01234567; KEY1=0xFEDCBA98`，且下方 "
        "`candidate_flash_fwmc_unlock_sequence` 直接檢查編譯後指令。",
        "",
        "第一次實機 handoff 隨後揭露第二個缺陷：source 在六路自檢前提前啟用 ADC runtime "
        "trigger／IRQ，並額外開啟六個 GPIO output driver；原 APP 此時仍以軟體觸發 ADC，"
        "且只操作未 output-enable 的 latch。這會造成 WH/WL/VH/VL/UH/UL 全部誤判。"
        "目前已拆開 startup/runtime ADC 階段並移除自檢前的 POER 寫入，且以下三項"
        "編譯後 gate 會阻止回歸。",
        "",
        "## 映像與分包",
        "",
        "| 項目 | 原廠 APP | 本次 source APP |",
        "|---|---:|---:|",
        f"| 明文大小 | {report['reference']['size']:,} bytes | {report['candidate']['size']:,} bytes |",
        f"| 8192-byte packs | {report['reference']['chunk_count']} | {report['candidate']['chunk_count']} |",
        f"| 寫入終點 | 0x{report['reference']['flash_end']:08x} | 0x{report['candidate']['flash_end']:08x} |",
        f"| 明文 SHA-256 | `{report['reference']['plaintext_sha256']}` | `{report['candidate']['plaintext_sha256']}` |",
        f"| 密文 SHA-256 | `{report['reference']['ciphertext_sha256']}` | `{report['candidate']['ciphertext_sha256']}` |",
        "",
        "包數只由映像大小決定；6 包本身不是錯誤。每包均已重建成 "
        "`# + sequence + # + uint16_le(length) + ciphertext + CRC-8/MAXIM`，"
        "sequence 由 N-1 遞減到 0。",
        "",
        "## 離線硬性檢查",
        "",
        "| 結果 | 檢查 | 證據 |",
        "|---|---|---|",
    ]
    for item in checks:
        lines.append(
            f"| {'PASS' if item['passed'] else 'FAIL'} | `{item['id']}` | "
            f"{item['detail'].replace('|', '/')} |"
        )
    lines += [
        "",
        "## 仍需裝置回報的 gate",
        "",
    ]
    for item in report["target_required"]:
        lines.append(f"- **{item['id']}**：{item['reason']}")
    lines += [
        "",
        "離線工具不能讀到 MCU 真正執行到哪一條指令，也不能證明片上 EFM 在這塊板子上"
        "完成了 `0x1E000` sector 的 erase/program。這正是之前把『套件可解密』誤寫成"
        "『APP 可正常啟動』的缺口。",
        "",
    ]
    return "\n".join(lines)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app", type=Path, required=True)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--encrypted", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--package-plain", type=Path, required=True)
    parser.add_argument(
        "--reference-app", type=Path,
        default=ROOT / "reference" / "official" /
        "APP_DM4310_V3_V5017_04.decrypted.bin",
    )
    parser.add_argument("--bootloader", type=Path, default=ROOT / "bootloader.bin")
    parser.add_argument("--profile", type=Path,
                        default=ROOT / "config" / "update_profile.json")
    parser.add_argument("--factory-cipher", type=Path,
                        default=ROOT / "dist" / "development" /
                        "APP_DM4310(V3)_V5017_04.bin")
    parser.add_argument("--objdump", default="arm-none-eabi-objdump")
    parser.add_argument("--nm", default="arm-none-eabi-nm")
    parser.add_argument("--output-json", type=Path, required=True)
    parser.add_argument("--output-markdown", type=Path, required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_arguments()
    checks: list[Check] = []

    bootloader = args.bootloader.read_bytes()
    reference = args.reference_app.read_bytes()
    candidate = args.app.read_bytes()
    package_plain = args.package_plain.read_bytes()
    encrypted = args.encrypted.read_bytes()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))

    add(checks, "bootloader_identity", digest(bootloader) == BOOTLOADER_SHA256,
        f"bootloader SHA-256 {digest(bootloader)}")
    add(checks, "reference_app_identity", digest(reference) == REFERENCE_APP_SHA256,
        f"official decrypted APP SHA-256 {digest(reference)}")
    verify_critical_fingerprints(checks, bootloader, reference)

    loader_key, loader_counter = extract_bootloader_material(bootloader)
    profile_key, profile_counter, profile = load_update_profile(args.profile)
    profile_matches = (
        profile_key == loader_key
        and profile_counter == loader_counter
        and profile.get("provenance_bootloader_sha256") == BOOTLOADER_SHA256
    )
    add(checks, "profile_matches_audited_loader", profile_matches,
        "profile key/counter and provenance equal bytes extracted from bootloader.bin")

    reference_cipher = aes256_ctr_transform(reference, loader_key, loader_counter)
    add(checks, "factory_cipher_reproduced",
        digest(reference_cipher) == FACTORY_CIPHERTEXT_SHA256,
        f"recomputed factory ciphertext SHA-256 {digest(reference_cipher)}")
    if args.factory_cipher.is_file():
        factory_cipher = args.factory_cipher.read_bytes()
        add(checks, "factory_cipher_byte_exact",
            factory_cipher == reference_cipher,
            f"{args.factory_cipher}: {len(factory_cipher)} bytes, byte-for-byte equal")
    else:
        add(checks, "factory_cipher_byte_exact", False,
            f"factory ciphertext evidence is missing: {args.factory_cipher}")

    stack, reset = validate_plain_app(candidate)
    add(checks, "package_plain_matches_build", package_plain == candidate,
        f"build/package plaintext SHA-256 {digest(candidate)}")
    expected_cipher = aes256_ctr_transform(candidate, loader_key, loader_counter)
    add(checks, "candidate_cipher_exact", encrypted == expected_cipher,
        f"candidate ciphertext SHA-256 {digest(encrypted)}")
    add(checks, "candidate_cipher_round_trip",
        aes256_ctr_transform(encrypted, loader_key, loader_counter) == candidate,
        "decrypting package with actual loader key/counter reproduces build BIN")

    records = build_update_records(encrypted)
    parsed = [parse_record(record) for record in records]
    sequences = [sequence for sequence, _payload in parsed]
    reconstructed = b"".join(payload for _sequence, payload in parsed)
    expected_sequences = list(range(len(records) - 1, -1, -1))
    add(checks, "candidate_uart_records", reconstructed == encrypted,
        f"{len(records)} records reconstruct all {len(encrypted)} ciphertext bytes")
    add(checks, "candidate_sequence_order", sequences == expected_sequences,
        f"sequence bytes {sequences}")
    add(checks, "candidate_chunk_bounds",
        all(0 < len(payload) <= CHUNK_SIZE for _sequence, payload in parsed),
        f"chunk lengths {[len(payload) for _sequence, payload in parsed]}")
    candidate_end = APP_BASE + len(candidate)
    add(checks, "candidate_flash_partition", candidate_end <= APP_END,
        f"write range 0x{APP_BASE:08x}..0x{candidate_end:08x}; limit 0x{APP_END:08x}")

    manifest_ok = (
        manifest.get("plaintext_size") == len(candidate)
        and manifest.get("plaintext_sha256") == digest(candidate)
        and manifest.get("ciphertext_sha256") == digest(encrypted)
        and manifest.get("chunk_size") == CHUNK_SIZE
        and manifest.get("chunk_count") == len(records)
        and manifest.get("profile_provenance_bootloader_sha256") == BOOTLOADER_SHA256
        and manifest.get("uart_payload_artifact") == args.encrypted.name
    )
    add(checks, "candidate_manifest", manifest_ok,
        "size, hashes, chunking, UART artifact and bootloader provenance are self-consistent")

    verify_source_startup(checks, args.elf, args.objdump, args.nm, stack, reset)

    reference_records = build_update_records(reference_cipher)
    report: dict[str, Any] = {
        "schema": 1,
        "generated_date": date.today().isoformat(),
        "offline_uart_package_ready": all(item.passed for item in checks),
        "target_uart_handoff_verified": False,
        "whole_app_behavior_verified": False,
        "corrected_defects": [
            {
                "id": "flash_fwmc_unlock_register",
                "previous": "first key to KEY1, second key to OTP KEY2",
                "corrected": "both FWMC unlock words written to KEY1",
                "observed_impact": "APP confirmation could not persist (0,1), causing the loader's Upgrade failed loop on the next reset",
            },
            {
                "id": "startup_adc_and_power_test_phase",
                "previous": "runtime ADC trigger/IRQ and GPIO output enable were active before the six-output startup test",
                "corrected": "software-trigger ADC test phase and non-driving GPIO latch sequence now precede runtime ADC trigger/IRQ enable",
                "observed_impact": "target reached APP but reported WH/WL/VH/VL/UH/UL all failed",
            },
        ],
        "reference": {
            "size": len(reference),
            "chunk_count": len(reference_records),
            "flash_end": APP_BASE + len(reference),
            "plaintext_sha256": digest(reference),
            "ciphertext_sha256": digest(reference_cipher),
        },
        "candidate": {
            "size": len(candidate),
            "chunk_count": len(records),
            "sequences": sequences,
            "flash_end": candidate_end,
            "initial_msp": f"0x{stack:08x}",
            "reset_vector": f"0x{reset:08x}",
            "plaintext_sha256": digest(candidate),
            "ciphertext_sha256": digest(encrypted),
        },
        "boot_record_state_machine": [
            "loader finishes update: (1,0)",
            "loader clears before first APP jump: (0,0)",
            "APP confirms after clock setup: (0,1)",
            "next reset: loader jumps directly to APP",
        ],
        "offline_checks": [asdict(item) for item in checks],
        "target_required": [
            {
                "id": "uart_handoff",
                "reason": "confirm the accepted image actually starts and Upgrade failed stops",
            },
            {
                "id": "persistent_confirmation",
                "reason": "warm-reset once more and confirm the loader still jumps directly to APP",
            },
            {
                "id": "app_uart_identity",
                "reason": "capture the reconstructed APP banner/status at 921600 8N1",
            },
            {
                "id": "communication_behavior",
                "reason": "exercise Disable/Enable/read commands and compare replies/state without a probe",
            },
            {
                "id": "motor_and_power_equivalence",
                "reason": "cannot be proven from a binary, package ACK, or UART text alone",
            },
        ],
    }

    args.output_json.parent.mkdir(parents=True, exist_ok=True)
    args.output_markdown.parent.mkdir(parents=True, exist_ok=True)
    args.output_json.write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    args.output_markdown.write_text(markdown_report(report), encoding="utf-8")

    failed = [item for item in checks if not item.passed]
    print(
        f"UART package audit: {len(checks) - len(failed)}/{len(checks)} "
        f"offline checks passed; candidate has {len(records)} packs"
    )
    if failed:
        for item in failed:
            print(f"FAIL {item.id}: {item.detail}")
        return 1
    print(f"report: {args.output_markdown}")
    print("target UART handoff remains explicitly unverified until device observation")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
