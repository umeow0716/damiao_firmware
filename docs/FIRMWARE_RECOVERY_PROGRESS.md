# Firmware Recovery Progress

最後更新：2026-10-03

下表是目前權威的 source/behavior recovery 狀態。DM4310 factory byte-exact 目標已停止，
歷史分析保存在 `docs/DM4310_BYTE_EXACT_PROGRESS.md`。現在先完成 DM4310；DM4340 與使用
正確 V3/V6417 reference 的 DM8009 必須分別重新掃描、復原及驗證，不能沿用舊移植版本的
「完成」標記。

## 型號進度

| 型號 | Factory 全域掃描 | Source 復原 | 全量差分 | 核心/映像 | plain/enc | 狀態 |
|---|---:|---:|---:|---:|---:|---|
| DM4310 V3 V5017.04 | 完成 | 完成 | 65/65 | 15/15 + PASS | 完成 | **5/5** |
| DM4340 V3 V5117.04 | 待重新稽核 | 待 DM4310 定版後復原 | 待執行 | 待執行 | 未定版 | **0/5** |
| DM8009 V3 V6417.04 | reference 已補齊，待重新掃描 | 舊移植不可採信 | 待執行 | 待執行 | 未定版 | **0/5** |

整體型號閘門：`[███████░░░░░░░░░░░░░] 5/15`。

## 已完成產物

| 產物 | 大小 | SHA-256 |
|---|---:|---|
| `dist/development/dm4310_plain.bin` | 53,784 | `bf38aaf88f85e3441a2115d8d5839fa4d273dd3a24ae1403a2cc52885e19acc2` |
| `dist/development/dm4310_enc.bin` | 53,784 | `2e18dc7061845367c92958147f19523f0885fdd1b31ef8b9a071b74ac34d74f8` |

DM4310 距 64 KiB APP 上限尚有 11,752 bytes。Plain 逐 byte 等於 ELF objcopy；encrypted
已通過 AES-256-CTR 解密 round-trip，manifest 的大小與 SHA-256 已核對。工作區中若仍存在
先前產生的 DM4340/DM8009 檔案，只能視為過期 development artifact，不能當作復原完成證據。

## 完成證據

- DM4310 factory/source 全量差分 65/65；不是只比較函式 hash，而是執行 factory binary
  與 source ELF，比對暫存器、固定 SRAM、MMIO、呼叫 ABI、FPSCR 與存取順序。
- DM4310 核心組合 15/15，涵蓋 Reset/SystemInit/scatter/runtime/main、五個 IRQ、
  deferred event、commissioning、motor identification 與 output calibration outer flow。
- DM4310 映像通過 64 KiB、vector、VMA/LMA、copy-down、zero-fill、RAMB、heap/stack、
  fixed helper 與所有 recovered state section 的直接地址檢查。
- DM4310 產品建置只使用 C/ASM、生成常數與 linker script；沒有把 factory binary 或 rebuilt
  `dm4310_plain.bin`/`dm8009_plain.bin` 連入韌體。
- 65 支持久化驗證器、共用 Unicorn loader、Ghidra 匯出器與映射工具均保存在
  `tools/recovery/` 或 `recovered/*/tables/`；`/tmp` 不承載唯一分析成果。
- 一般 Flash 函式不鎖 factory address 或長度。固定配置只存在於 recovered SRAM ABI
  compatibility layer；recovery regressions 不屬於一般 CMake/make 編譯圖。

## 最終交付閘門

- [x] DM4310 完成 build、plain/enc 封裝、65/65 全量差分、15/15 核心與映像驗證。
- [x] DM4310 移除一般 Flash factory anchors；後續修改一般函式不會因地址或 padding 失敗。
- [ ] DM4340 依正確 reference 完成全域掃描、復原、差分、映像及封裝。
- [ ] DM8009 依正確 V3/V6417 reference 取代舊移植，完成相同閘門。
- [ ] 從全新 build directory 重建六份最終產物，完成 workspace 清理、最終 commit 與 push。

此表的「完成」指 disassembly 與 factory/source differential 所能驗證的 source-recovery
範圍；實機功率級與馬達負載測試仍應依硬體驗收流程另行執行。
