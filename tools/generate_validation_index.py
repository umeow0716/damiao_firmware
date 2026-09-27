#!/usr/bin/env python3
"""Validate and summarize the current DM4310 evidence bundle."""

from __future__ import annotations

import argparse
import hashlib
import json
from datetime import date
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
VALIDATION = ROOT / "dist" / "validation"


def load_json(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"{path} is not a JSON object")
    return value


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(65536), b""):
            digest.update(block)
    return digest.hexdigest()


def verify_package(package_name: str) -> dict[str, Any]:
    package_dir = ROOT / "dist" / package_name
    manifest = load_json(package_dir / "manifest.json")
    plain = package_dir / str(manifest["direct_flash_artifact"])
    encrypted = package_dir / str(manifest["bootloader_update_artifact"])
    frames = package_dir / "app_update.frames.jsonl"
    if plain.stat().st_size != int(manifest["plaintext_size"]):
        raise ValueError(
            f"{package_name}: plaintext size does not match manifest"
        )
    if encrypted.stat().st_size != int(manifest["plaintext_size"]):
        raise ValueError(
            f"{package_name}: ciphertext size does not match manifest"
        )
    if sha256(plain) != manifest["plaintext_sha256"]:
        raise ValueError(
            f"{package_name}: plaintext SHA-256 does not match manifest"
        )
    if sha256(encrypted) != manifest["ciphertext_sha256"]:
        raise ValueError(
            f"{package_name}: ciphertext SHA-256 does not match manifest"
        )
    frame_count = sum(1 for line in frames.read_text(encoding="utf-8").splitlines()
                      if line.strip())
    if frame_count != int(manifest["can_frame_count"]):
        raise ValueError(f"{package_name}: frame count does not match manifest")
    return manifest


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    source = load_json(VALIDATION / "source_image.json")
    reference = load_json(VALIDATION / "reference_image.json")
    functions = load_json(VALIDATION / "function_code.json")
    semantic = load_json(VALIDATION / "semantic_coverage.json")
    full_audit = load_json(VALIDATION / "full_app_audit.json")
    accountability = load_json(
        VALIDATION / "original_function_accountability.json"
    )
    discovery = load_json(VALIDATION / "original_function_discovery.json")
    differential = (
        VALIDATION / "original_source_differential.txt"
    ).read_text(encoding="utf-8").strip()
    uart_audit = load_json(VALIDATION / "uart_package_audit.json")
    static_analysis = load_json(VALIDATION / "static_analysis.json")
    supplemental_static = load_json(
        VALIDATION / "supplemental_static_analysis.json"
    )

    if reference.get("byte_exact") is not True:
        raise ValueError("historical reference target is no longer byte exact")
    if not differential.startswith("PASS original/source differential"):
        raise ValueError("original/source differential did not record PASS")
    if uart_audit.get("offline_uart_package_ready") is not True:
        raise ValueError("offline UART package audit did not pass")
    if semantic.get("known_gaps") != []:
        raise ValueError("semantic coverage contains missing/partial source gaps")
    for image_name in ("app", "bootloader"):
        image = static_analysis[image_name]
        if image["direct_recursion_cycles"] != 0:
            raise ValueError(f"{image_name}: static analysis found recursion")
        if image["stack_margin_bytes"] <= 0:
            raise ValueError(f"{image_name}: no positive static stack margin")
        if image["forbidden_runtime_symbols"] != []:
            raise ValueError(f"{image_name}: forbidden runtime API linked")
    if supplemental_static.get("result") != "PASS":
        raise ValueError("supplemental static analysis did not pass")
    for analyzer_name in ("cppcheck", "clang_tidy"):
        if supplemental_static[analyzer_name]["diagnostics"] != 0:
            raise ValueError(f"{analyzer_name}: static diagnostics remain")
    function_total = sum(
        int(value) for value in accountability["by_image"].values()
    )
    if function_total != len(accountability["functions"]):
        raise ValueError("original-function totals do not agree")
    if discovery.get("result") != "PASS":
        raise ValueError("original-function discovery audit did not pass")
    if discovery.get("total_functions") != function_total:
        raise ValueError("function discovery/accountability totals do not agree")
    for image_name in ("app", "bootloader"):
        if discovery["images"][image_name]["calls"]["missing_direct_targets"] != 0:
            raise ValueError(f"{image_name}: direct call target is missing")

    packages = {"development": verify_package("development")}

    source_candidate = source["candidate"]
    reference_candidate = reference["candidate"]
    status_counts = ", ".join(
        f"`{key}` {value}" for key, value in semantic["status"].items()
    )
    gate_counts = ", ".join(
        f"`{key}` {value}" for key, value in semantic["target_gates"].items()
    )

    lines = [
        "# DM4310 驗證證據包",
        "",
        f"生成日期：{date.today().isoformat()}",
        "",
        "## 結論先讀",
        "",
        "目前主線 source APP 已通過建置、記憶體配置、ABI、host tests、語意歸責、有限範圍的"
        "原始 machine-code 差分，以及 UART 套件的離線硬性稽核。下方加密更新包的實際檔案"
        "也與 manifest 一致。",
        "",
        "先前純 UART 更新後反覆 `Upgrade failed` 的直接原因已找到：Flash writer 曾把第二個 "
        "FWMC key 寫到 OTP `KEY2`；目前已修成原 APP／DDL 的 KEY1/KEY1 sequence，並加入"
        "編譯後指令 gate。",
        "",
        "第一次修正版已由實機證明 loader 能自動 reset 並進 APP，但隨即六路 "
        "`MOSFET ERROR`。重新反組譯 startup 後又修正兩項差異：ADC runtime trigger／IRQ "
        "不得在輪詢自檢前啟用，自檢也不得額外開啟 GPIO POERA／POERB。這兩項現在同樣由"
        "最終 ELF gate 檢查。",
        "",
        "該舊版又暴露 recovery 缺陷：startup fault loop 不返回 main loop，所以 UART `X` 只設"
        "deferred flag 而不能進 loader。主線三條 blocking fault 路徑現在都會重新開 IRQ 並服務"
        "boot request，post-link gate 會檢查其 call sites。已卡住的舊版仍需依 "
        "[緊急復原說明](../../docs/EMERGENCY_RECOVERY.md)優先用已實機證實的 loader 開機 UART `X` 窗；ROM ISP/SWD 為備援。",
        "",
        "這不是實機電氣等價的宣告。周邊時序、功率級、馬達 trace、真實 Flash 失敗／"
        "復原，以及由已安裝 bootloader 更新後真正啟動，都仍須完成 "
        "[`PORTING_CHECKLIST.md`](../../docs/PORTING_CHECKLIST.md) 的分階段硬體 gate。",
        "",
        f"Fail-closed 全面稽核目前仍有 "
        f"**{full_audit['unresolved_release_blocker_count']}/"
        f"{full_audit['release_blocker_count']}** 個 release blocker 未解；"
        "因此本頁列出的 package 只能稱為 development candidate。",
        "",
        "## 目前的 source artifacts",
        "",
        "| Artifact | Bytes | SHA-256 | Vector／reset |",
        "|---|---:|---|---|",
        f"| Source APP | {source_candidate['size']:,} | "
        f"`{source_candidate['sha256']}` | MSP "
        f"`{source_candidate['initial_msp']}`, reset "
        f"`{source_candidate['reset_vector']}` |",
        f"| 歷史 exact 稽核 target | {reference_candidate['size']:,} | "
        f"`{reference_candidate['sha256']}` | byte exact：是 |",
        "",
        "exact 稽核 target 是隔離的歷史證據，不是 source firmware 產品。",
        "",
        "## 加密更新包",
        "",
        "| Package | 明文 bytes | 明文 SHA-256 | 密文 SHA-256 | Chunks | Frames |",
        "|---|---:|---|---|---:|---:|",
    ]
    for package_name, manifest in packages.items():
        lines.append(
            f"| `{package_name}` | {int(manifest['plaintext_size']):,} | "
            f"`{manifest['plaintext_sha256']}` | "
            f"`{manifest['ciphertext_sha256']}` | "
            f"{manifest['chunk_count']} | {manifest['can_frame_count']:,} |"
        )

    lines += [
        "",
        "每個 package 都已核對 manifest 中的檔案大小、SHA-256 與 JSONL frame 數。"
        "`app_plain.bin` 是給 programmer 從 `0x00020000` 寫入的明文；"
        "`app_update.enc.bin` 是給 bootloader transport 的密文，不能當成一般 BIN 直接燒錄。",
        "",
        "## 證據 gate",
        "",
        f"- 舊／新程式直接執行：`{differential}`",
        f"- UART package 離線稽核："
        f"{sum(1 for item in uart_audit['offline_checks'] if item['passed'])}/"
        f"{len(uart_audit['offline_checks'])} PASS；前一候選已觀察到 loader 自動 reset／APP "
        "handoff，新 ADC／GPIO 候選仍待實機驗收。",
        f"- 全程式靜態分析：APP 保守巢狀 stack "
        f"{static_analysis['app']['conservative_nested_stack_bytes']}/"
        f"{static_analysis['app']['stack_budget_bytes']} bytes；bootloader "
        f"{static_analysis['bootloader']['conservative_nested_stack_bytes']}/"
        f"{static_analysis['bootloader']['stack_budget_bytes']} bytes；無直接遞迴、"
        "無非預期間接呼叫、無禁用 runtime API。",
        f"- 補充靜態分析：cppcheck `{supplemental_static['cppcheck']['version']}`、"
        f"clang-tidy `{supplemental_static['clang_tidy']['version']}`；"
        f"{supplemental_static['unique_c_files']} 個自有 C 檔／"
        f"{supplemental_static['build_configurations']} 個編譯配置，未抑制診斷 0。",
        f"- 語意追蹤矩陣：{semantic['total_subsystems']} 個 subsystem；"
        f"{status_counts}. 這只表示 source/test/evidence 路徑有歸責，不是行為等價 PASS。",
        f"- 尚待裝置驗收：{semantic['target_pending_count']} 個 subsystem；"
        f"最終 gate 分布為 {gate_counts}.",
        f"- 原始函式逐項歸責：{function_total} 個入口"
        f"（APP {accountability['by_image']['app']}、bootloader "
        f"{accountability['by_image']['bootloader']}）；raw call/pointer discovery audit PASS。",
        f"- 逐 byte machine-code 比對："
        f"{functions['reference_functions_found_verbatim']}/"
        f"{functions['reference_functions_audited']} 個歷史函式。GCC source 重寫出現這個結果"
        "是預期行為，因此不拿它作為語意等價證明。",
        f"- 歷史 byte 基線：exact 稽核 target 有 "
        f"{reference['differing_or_missing_bytes']} 個不同／缺少 byte。",
        f"- Source 與歷史映像 byte 稽核：{source['differing_or_missing_bytes']:,} 個不同／"
        "缺少 byte；僅透明揭露，不當作可持續開發版的合格 gate。",
        "",
        "## 詳細檔案",
        "",
        "- [`full_app_audit.md`](full_app_audit.md)：fail-closed 全面掃描與未解 release blockers。",
        "- [`semantic_coverage.md`](semantic_coverage.md)：subsystem 到 source、test 與最終 gate 的矩陣。",
        "- [`original_function_accountability.md`](original_function_accountability.md)：所有原始 Ghidra 入口的歸責。",
        "- [`original_function_discovery.md`](original_function_discovery.md)：直接呼叫、IRQ/callback 函式指標與漏檢防護。",
        "- [`hardware_capture.md`](hardware_capture.md)：已解碼的 target register 與 RAM capture。",
        "- `source_image.json`、`reference_image.json`、`function_code.json`：可供工具讀取的映像／程式碼比較。",
        "- [`original_source_differential.txt`](original_source_differential.txt)：保存的 QEMU 差分結果。",
        "- [`uart_package_audit.md`](uart_package_audit.md)：原 loader、加密分包、vector 與早期 boot confirmation 稽核。",
        "- [`static_analysis.md`](static_analysis.md)：GCC analyzer、stack、call graph、間接呼叫與 link-input 稽核。",
        "- [`supplemental_static_analysis.md`](supplemental_static_analysis.md)：cppcheck CTU 與 clang-tidy/Clang Static Analyzer 稽核。",
        "- [全面重新驗收報告](../../docs/REVALIDATION_AUDIT.md)：本次 UART failure 根因、修正與開放差異。",
        "- [開發電子書](../../docs/book/README.md)：從硬體底層、控制流程到修改食譜的閱讀入口。",
        "",
    ]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines), encoding="utf-8")
    print(f"validation index written to {args.output}")


if __name__ == "__main__":
    main()
