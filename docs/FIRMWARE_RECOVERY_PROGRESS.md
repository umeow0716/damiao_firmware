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

所有檔案位於 `dist/development/`。Plain 大小比對應 factory image 大 5.44–5.53%，
仍保留 11,396–11,424 bytes 的 64 KiB APP 空間；一般 Flash 函式沒有固定地址或長度。

| 型號 | Plain/enc 大小 | 剩餘 APP | Plain SHA-256 | Encrypted SHA-256 |
|---|---:|---:|---|---|
| `dm10010` | 54,120 | 11,416 | `67a48572b90f4f968e14273bb284fd94fbc285dccde5d5a44205d1259a6a7d09` | `768ed754d84465fe6f2112148844667d55ba0ef71079d9e437898608b17b2567` |
| `dm3507` | 54,112 | 11,424 | `e9d354d85b79279e8d976b0d16f97cba81b7a9f81ec3dba645a28e4eef5ef4e6` | `c78ba910d5f42955b1da944b00609a71f63835ea2a9ed10062917576df1f9962` |
| `dm3507_48v` | 54,128 | 11,408 | `61691d82f10da848aa334bbec51f7b25304a53fb310b264954bb66c389e4c6ca` | `1d43ed47d5f92319ae94f5af52cec1187d6921c1643f9761c8393be5c2ccc178` |
| `dm4310` | 54,120 | 11,416 | `6aae35703a2a4119c83af8dc63d854bd7009ca706cbaa203dcccedfe3afee069` | `6ab4945dd9f14c945785e32af187673bf6afd2c37be87c89f09200701cb36348` |
| `dm4310_48v` | 54,128 | 11,408 | `20555e3f138331c6e9f5bd3edecf9dddba027d69cda80101b033c7715c78f322` | `da4a820080e2fee99a905877a5b80bb7e38d3a122ce32a663f55dd916ad3fe9a` |
| `dm4340` | 54,120 | 11,416 | `5e52b398cbfc5d76d577fd5328cf6fa4f5fda0e2f10cee3be2c3db5182d04d8d` | `9a481c5e4b1f39313f48dae1300ffc4ca88ec8734d033e6edb9592e3e4928b04` |
| `dm4340_48v` | 54,128 | 11,408 | `61b3a91f4ea997c2e468d192b9de4517ee1496fd7d6e74fd327a352b48847ad5` | `de28de5310706c206025a95cf918a58f2345db168c437da138812d2b9d11042f` |
| `dm8006` | 54,140 | 11,396 | `cb298dbad0a59158eab9ba234d190a914e9f5545baa70992153a2d1ca9a047c9` | `4b9bfb3100b01ba6fb900dcea4047a4c22f74b2086adb251c6fb6ebc0cba66ec` |
| `dm8009` | 54,140 | 11,396 | `ba491b7de5a2accbfb92b4d54f83d78828ab9a71e9639d448d10b9e61cad35ee` | `8517bff57349e736fb9993e2b05ba3c9871308761100820cae93e8f39ded1e95` |

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
- linker 直接匯出可開發餘裕：九型號 Flash 尚餘 11,396–11,424 bytes，`.app_state`
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
