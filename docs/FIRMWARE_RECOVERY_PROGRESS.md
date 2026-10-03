# Firmware Recovery Progress

最後更新：2026-10-03

本表是目前權威的 source/behavior recovery 狀態。DM4310 factory byte-exact 目標已停止，
歷史分析保存在 `docs/DM4310_BYTE_EXACT_PROGRESS.md`。正式範圍是 `reference/` 內具備
original/decrypted 配對的九款 V3 sub-version 04 韌體；DM1h10L 與 DM6006 不在目標內。
所有產品映像均由可維護的 C 與必要 ASM 重建，沒有連結或嵌入 factory image。

## 型號進度

| 型號 | Factory 全域掃描 | Source 復原 | 全量差分 | 核心/映像 | plain/enc | 狀態 |
|---|---:|---:|---:|---:|---:|---|
| DM10010 V3 V5617.04 | 完成 | 完成 | 65/65 | 15/15 + PASS | 完成 | **5/5** |
| DM3507 V3 V5717.04 | 完成 | 完成 | 65/65 | 15/15 + PASS | 完成 | **5/5** |
| DM3507 V3 48V V6517.04 | 完成 | 完成 | 65/65 | 15/15 + PASS | 完成 | **5/5** |
| DM4310 V3 V5017.04 | 完成 | 完成 | 65/65 | 15/15 + PASS | 完成 | **5/5** |
| DM4310 48V V6017.04 | 完成 | 完成 | 65/65 | 15/15 + PASS | 完成 | **5/5** |
| DM4340 V3 V5117.04 | 完成 | 完成 | 65/65 | 15/15 + PASS | 完成 | **5/5** |
| DM4340 48V V6117.04 | 完成 | 完成 | 65/65 | 15/15 + PASS | 完成 | **5/5** |
| DM8006 V3 V6317.04 | 完成（185 functions） | 完成 | 65/65 | 15/15 + PASS | 完成 | **5/5** |
| DM8009 V3 V6417.04 | 完成（185 functions） | 完成 | 65/65 | 15/15 + PASS | 完成 | **5/5** |

整體型號閘門：`[████████████████████] 45/45`。

## 最終產物

所有檔案位於 `dist/development/`。Plain 大小比對應 factory image 大 4.97–5.08%，
仍保留 11,636–11,664 bytes 的 64 KiB APP 空間；一般 Flash 函式沒有固定地址或長度。

| 型號 | Plain/enc 大小 | 剩餘 APP | Plain SHA-256 | Encrypted SHA-256 |
|---|---:|---:|---|---|
| `dm10010` | 53,880 | 11,656 | `4f90c2725c04e52ca61acba367fd27db6e5481a80f8f74663fb473f67d03aedf` | `1d9e274374a6cf712d97b59a42a7652f4ed6179a9c69b4fa68c564f4995e866e` |
| `dm3507` | 53,872 | 11,664 | `737cb39fe4a94f309beb9ba017c82daf17cc81b96a074630be339ae952e8baee` | `b20df80f52a2f9859a0e2ef51760aedbfa796f63d508e996ef6c0111fd831683` |
| `dm3507_48v` | 53,888 | 11,648 | `36747dd66f27084a0db3e03e364e268e4b215f1a7c029477dfe610befdbff1a2` | `5aa035326a7cf9504324c8a2c9c51914ca5aeb64035dc4ef32a0964776647230` |
| `dm4310` | 53,880 | 11,656 | `20660d5026ef6c9cac73a6bbe6e95b52ce03b9108f095f733b0c021cabebe443` | `6835c43391fa534f3ef244d5c200749d0eade91c6dd78184056c8dcb6d233fe0` |
| `dm4310_48v` | 53,896 | 11,640 | `e2bcc56d6241330967cd92af9e67b645f2414a49aa98d004f711cb0d07961368` | `54031026defb9584c405a0ea6fa5f06e10429e9a9cba05250e50e67f53d0aef8` |
| `dm4340` | 53,880 | 11,656 | `4c157b52952d580624652c1b2d941684a9695e83ded374f42c18cc722a614867` | `b31f3075e83b9c624d2fbaec358d8648a7fed412c1ce1c87424b047057d3cf7f` |
| `dm4340_48v` | 53,896 | 11,640 | `365358eae7033b03422f48722998af7ce1731412c940d9309460527bec348fde` | `4ca8b6e08d2741f64a3cd6bba7509aa94e92978e759fff69dc62491d41fd9828` |
| `dm8006` | 53,900 | 11,636 | `3079052c5c9954bcb2279415da393d6fa177f54f3779990ccbb8c93ef58f8e66` | `a22099182b716a97bd99e8d99b587663657cd6f64c90d39e7ac66bf22634ffbf` |
| `dm8009` | 53,900 | 11,636 | `d348fa69ba48d9c4163cc25042ce17edc93d14ea550334a2b361e7b7ca5d9a0b` | `32f71f93cdebdf50c194c051f453eaf9140686ced2bb6e34f02cc3452fe39a00` |

每個 plain 都逐 byte 等於對應 ELF 的 objcopy 結果。每個 encrypted 產物都通過
AES-256-CTR 解密 round-trip，manifest 的大小與 SHA-256 也已核對。

## 完成證據

- 九型號 factory/source 全量差分合計 585/585。驗證器實際執行 factory binary 與 source
  ELF，比對暫存器、固定 SRAM、MMIO、呼叫 ABI、FPSCR 與存取順序，不以函式 hash 代替行為。
- 九型號核心組合合計 135/135，涵蓋 Reset/SystemInit/scatter/runtime/main、五個 IRQ、
  deferred event、周邊初始化、commissioning、motor identification 與 output calibration。
- 九型號映像都通過 64 KiB、vector、VMA/LMA、copy-down、zero-fill、RAMB、heap/stack、
  fixed helper 與 recovered state section 的地址檢查，無區段重疊。
- `make firmwares` 與獨立的 `build-acceptance` 全新建置均產生並驗證 18 份輸出。
- 產品建置只使用 C/ASM、生成常數與 linker script；factory/decrypted binary 只供獨立差分驗證，
  沒有進入 CMake/make 產品依賴圖。
- 65 支持久化差分驗證器、15 支核心 runner、共用 Unicorn loader、Ghidra 匯出器及地址映射
  工具均保存在 `tools/recovery/` 或 `recovered/*/tables/`；`/tmp` 不承載唯一分析成果。
- 固定配置只存在於 factory SRAM ABI compatibility layer。recovery regressions 不屬於一般
  CMake/make 編譯流程，後續修改一般 Flash 函式不會因 factory padding 或 address anchor 失敗。

## 最終交付閘門

- [x] 九型號完成 factory 掃描、source recovery 與個別 profile/fixed-SRAM layout。
- [x] 九型號完成 65/65 全量差分、15/15 核心組合與映像驗證。
- [x] 九型號完成 plain/enc 封裝、AES round-trip、manifest 與 source identity 驗證。
- [x] 從全新 build directory 重建 18 份最終產物。
- [x] 一般 Flash factory anchors 已移除；測試未加入正常產品編譯流程。

此表的「完成」指 disassembly 與 factory/source differential 所能驗證的 source-recovery
範圍；實機功率級、馬達負載與各硬體 revision 的安全測試仍需依硬體驗收流程執行。
