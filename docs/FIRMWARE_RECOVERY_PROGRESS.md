# Firmware Recovery Progress

最後更新：2026-10-03

本表是目前權威的 source/behavior recovery 狀態。DM4310 factory byte-exact 目標已停止，
歷史分析保存在 `docs/DM4310_BYTE_EXACT_PROGRESS.md`。正式範圍是 `reference/` 內具備
original/decrypted 配對的九款 V3 sub-version 04 韌體；DM1h10L 與 DM6006 不在目標內。
所有產品映像均由可維護的 C 與必要 ASM 重建，沒有連結或嵌入 factory image。
工作區架構、功能地圖、修改方法與實機限制請由 `docs/README.md` 進入。

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

所有檔案位於 `dist/development/`。Plain 大小比對應 factory image 大 4.91–5.03%，
仍保留 11,664–11,688 bytes 的 64 KiB APP 空間；一般 Flash 函式沒有固定地址或長度。

| 型號 | Plain/enc 大小 | 剩餘 APP | Plain SHA-256 | Encrypted SHA-256 |
|---|---:|---:|---|---|
| `dm10010` | 53,856 | 11,680 | `6f20a3704f2c1e1f7532b73f492d2a276cdf77c79578a546d430e8342b46148e` | `bc896c287dd539ab63351ff4aca7fc7d18cd9d733db6ad8c1f6d5c6db8b7069b` |
| `dm3507` | 53,848 | 11,688 | `0d6d9f4ec8bfde570431c9cd9213c71fa83e6a6dd1363da591c9f3ccc3e919f4` | `2194f868f82ce1191d9f3c415a8a13238f8a5d5e7b3e9ad748f08f7f20718fd8` |
| `dm3507_48v` | 53,864 | 11,672 | `9c91ed14847d46d9025f7e20b8fe8dec0984a971b1937ce89db86c205d406952` | `af1adc352617fbaed1a4fe2cb244c9d5664302f3e7895cb9d5559d51745855f3` |
| `dm4310` | 53,856 | 11,680 | `ce923b242bf9c055aef81c76078bd676cef3b55779f11c403715238384451707` | `06a519f26e6352e3ebe4d38c6c076845672026ebf67c5d2ed21bdf2fbcbce550` |
| `dm4310_48v` | 53,872 | 11,664 | `e2300cbac596a81bc9000249460e637b225657f4e03b70c75964f51df89475e0` | `24cc72578a081bac64af5e6aebb041d1f93d47911f9be4e00d9fd337278d4cf2` |
| `dm4340` | 53,856 | 11,680 | `d379fddde892b002cd49560b251c02d552f8e749abaaf77f6fd0f7ee3cf4df3d` | `e589ce8606db692b8e5c65a232279f1cc53b070c7637cfded4dadc5386965d63` |
| `dm4340_48v` | 53,872 | 11,664 | `beb7549ff34289ad8db905ad3312830fb97e8d1f854a5987ec3c0293e96575be` | `ec1288219856a94161a67c02b1913cc779c74699a7dd88bde17f8a2887762da2` |
| `dm8006` | 53,868 | 11,668 | `6114d7da6b04e02f103d98de0ef8bfc45a7154cdfb6a1bc70d42becbeb2e3963` | `9361a5f8180874c602b31b03868542b493abab5169b47a5d58a7a8fce0624b22` |
| `dm8009` | 53,868 | 11,668 | `1ece8d7d0a47fc347588a29337bf26c950926f66af31f81456bc38b47dda4dbc` | `0d77352bca3ac51e961cdb88add976d6eb56aaac719277ed484b8b6550e9d42b` |

每個 plain 都逐 byte 等於對應 ELF 的 objcopy 結果。每個 encrypted 產物都通過
AES-256-CTR 解密 round-trip，manifest 的大小與 SHA-256 也已核對。

## 完成證據

- 九個 target 現在各自只定義一個 `DAMIAO_MODEL_*`；DM800x 與 DM43 48V 的差異由
  `DAMIAO_LAYOUT_*` 明確描述，不再以 `DAMIAO_DM4310` 假裝成同一型號。
- 產品碼中的舊 DM4310 compatibility 條件已展開成唯一有效路徑，未使用替代實作已刪除。
  共用 formatter/arithmetic、memory layout、linker 檔名與公開資料型別都使用產品語意；
  `FUN_*`、`Vector_*`、model-named shared section 等逆向式識別字不留在產品 source tree。
- 一般 C/H 已套用一致格式；固定 SRAM 與算術 ABI 仍由語意化 linker section、型別與必要
  ASM 明確表達。最終 build 後重新執行 585/585 差分與九份 image-layout 驗收。
- `tools/recovery/verify_source_architecture.py` 獨立檢查 model/profile/layout 分離、共用 V3
  檔案命名與 recovery 工具隔離，不加入一般 CMake/make 流程。
- 九型號 factory/source 全量差分合計 585/585。驗證器實際執行 factory binary 與 source
  ELF，比對暫存器、固定 SRAM、MMIO、呼叫 ABI、FPSCR 與存取順序，不以函式 hash 代替行為。
- 九型號核心組合合計 135/135，涵蓋 Reset/SystemInit/scatter/runtime/main、五個 IRQ、
  deferred event、周邊初始化、commissioning、motor identification 與 output calibration。
- 九型號映像都通過 64 KiB、vector、VMA/LMA、copy-down、zero-fill、RAMB、heap/stack、
  fixed helper 與 recovered state section 的地址檢查，無區段重疊。
- `make firmwares` 與獨立的多目標 CMake build 均成功；18 份輸出已重新產生並驗證。
- 產品建置只使用 C/ASM、生成常數與 linker script；factory/decrypted binary 只供獨立差分驗證，
  沒有進入 CMake/make 產品依賴圖。
- 65 支持久化差分驗證器、15 支核心 runner、共用 Unicorn loader、Ghidra 匯出器及地址映射
  工具均保存在 `tools/recovery/` 或 `recovered/*/tables/`；`/tmp` 不承載唯一分析成果。
- 固定配置只存在於硬體需要的 SRAM ABI 與 linker memory layout。Recovery regressions
  不屬於一般 CMake/make 編譯流程；一般 Flash 函式沒有 padding 或 address anchor。
- 私有 C 型別已撤除無必要的固定 offset assertion；真正由 SRAM/IRQ/ASM 取用的型別改用
  `SRAM_ABI_ASSERT_*` 明確標示。這些 compile-time 檢查不占映像空間。
- linker 直接匯出可開發餘裕：九型號 Flash 尚餘 11,664–11,688 bytes，`.app_state`
  尚餘 1,760 bytes，一般 BSS 尚餘 24,396 bytes；新增功能不必修改固定 SRAM 物件。

## 最終交付閘門

- [x] 九型號完成 factory 掃描、source recovery 與個別 profile/fixed-SRAM layout。
- [x] 九型號完成 65/65 全量差分、15/15 核心組合與映像驗證。
- [x] 九型號完成 plain/enc 封裝、AES round-trip、manifest 與 source identity 驗證。
- [x] 從全新 build directory 重建 18 份最終產物。
- [x] 一般 Flash factory anchors 已移除；測試未加入正常產品編譯流程。
- [x] 每個 target 只有自己的 model identity；共用 source 與 SRAM layout 選擇已解耦。

此表的「完成」指 disassembly 與 factory/source differential 所能驗證的 source-recovery
範圍；實機功率級、馬達負載與各硬體 revision 的安全測試仍需依硬體驗收流程執行。
