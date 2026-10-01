# Firmware Recovery Progress

最後更新：2026-10-02

## 型號進度

| 型號 | Factory 全域掃描 | Source 復原 | 全量差分 | 核心/映像 | plain/enc | 狀態 |
|---|---:|---:|---:|---:|---:|---|
| DM4310 V3 V5017.04 | 完成 | 完成 | 65/65 | 15/15 + PASS | 完成 | **5/5** |
| DM4340 V3 V5117.04 | 完成 | 完成 | 65/65 | 15/15 + PASS | 完成 | **5/5** |
| DM8009 V3 V6417.04 | 完成（185 functions） | 完成 | 65/65 | 15/15 + PASS | 完成 | **5/5** |

整體型號閘門：`[████████████████████] 15/15`。

## 已完成產物

| 產物 | 大小 | SHA-256 |
|---|---:|---|
| `dist/development/dm4310_plain.bin` | 62,360 | `26a0385a87bbd06ef1e1d3c4e25d1d1bd87cd88460fa3111f08db6f633ea25d1` |
| `dist/development/dm4310_enc.bin` | 62,360 | `f290ffbb4caccb60ae42c557649df113f18fc78e0746357d4a5930a8d8c2f089` |
| `dist/development/dm4340_plain.bin` | 62,360 | `463462b8c3ead2384d09993b0e52e2fef00fa853cd0b37d5edbe5dd675c6f78f` |
| `dist/development/dm4340_enc.bin` | 62,360 | `9c441067e4448e0743d35ba28c71db12067184785f203d15dc8a58cacba4910e` |
| `dist/development/dm8009_plain.bin` | 62,372 | `2e72f182355fda9143f41deb8f78cc7a1c52dead6e4ad0162c5d4d9d16dc8f74` |
| `dist/development/dm8009_enc.bin` | 62,372 | `a455f712dce8cf15aca2620a46485ed8bf7458ec3d9645958e87bd78cbb1e195` |

DM4310/DM4340 距 64 KiB APP 上限尚有 3,176 bytes；DM8009 尚有 3,164 bytes。
每份 plain 均逐 byte 等於對應 ELF 的 objcopy，encrypted 均已通過 AES-256-CTR
解密 round-trip，manifest 的大小與 SHA-256 也已核對。

## 完成證據

- 三型號 factory/source 全量差分各 65/65；不是只比較函式 hash，而是執行工廠 binary
  與 source ELF，比對暫存器、固定 SRAM、MMIO、呼叫 ABI、FPSCR 與存取順序。
- 三型號核心組合各 15/15，涵蓋 Reset/SystemInit/scatter/runtime/main、五個 IRQ、
  deferred event、commissioning、motor identification 與 output calibration outer flow。
- 三份映像均通過 64 KiB、vector、VMA/LMA、copy-down、zero-fill、RAMB、heap/stack、
  fixed helper 與所有 recovered state section 的直接地址檢查。
- DM8009 使用正確 V3/V6417 reference；262 筆 DM4310→DM8009 SRAM 映射與七個不同順序的
  fixed-state section 已落入共用 source/linker。錯誤 V4/V7318 死碼與命名已刪除。
- 產品建置只使用 C/ASM、生成常數與 linker script；沒有把 factory binary 或 rebuilt
  `dm4310_plain.bin`/`dm8009_plain.bin` 連入韌體。
- 65 支持久化驗證器、共用 Unicorn loader、Ghidra 匯出器與映射工具均保存在
  `tools/recovery/` 或 `recovered/*/tables/`；`/tmp` 不承載唯一分析成果。
- recovery regressions 不屬於一般 CMake/make 編譯圖；正常 `make firmwares` 只做產品建置、
  封裝，以及來源/大小/AES/manifest 的產物完整性驗證。

## 最終交付閘門

- [x] 一般工作目錄完成三型號 build、六份封裝與全部差分/組合/映像驗證。
- [x] 移除重複舊 verifier、V4/V7318 遺留與過時 target 註解。
- [x] 從獨立全新 build 目錄重建並重驗六份產物；結果與一般 build 逐 byte 相同。
- [x] `git diff --check`、最終狀態稽核、commit 與 push。

此表的「完成」指 disassembly 與 factory/source differential 所能驗證的 source-recovery
範圍；實機功率級與馬達負載測試仍應依硬體驗收流程另行執行。
