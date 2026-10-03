# Firmware Recovery Progress

最後更新：2026-10-03

本表是目前權威的 source/behavior recovery 狀態。DM4310 factory byte-exact 目標已停止，
歷史分析保存在 `docs/DM4310_BYTE_EXACT_PROGRESS.md`。正式 source-recovery 範圍是
`reference/V3/` 內具備 original/decrypted 配對的九款 V3 sub-version 04 韌體；DM1h10L
與 DM6006 不在目標內。`reference/V4/` 另保存三份用於版本與算法研究的 V4 配對，目前不代表
V4 已納入 source-recovery 交付範圍；差異見 `docs/FIRMWARE_VERSION_ANALYSIS.md`。
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

表中 factory 檔案位於 `dist/development/factory/`。Plain 大小比對應 factory image 大 5.44–5.53%，
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

### Raw-feedback 產物

raw 版位於 `dist/development/raw/`，比同型號 factory 版增加 72–80 bytes，64 KiB APP 仍保留
11,316–11,344 bytes。它維持位置、方向、單位、量化、溫度與 fault 行為，只讓標準 feedback 與
`0x7FF` LIVE 的速度／扭矩回報繞過既有低通；UART 狀態 banner 為
`DMBOT Motor Driver(with raw result)`。

| 型號 | Plain/enc 大小 | 剩餘 APP | Plain SHA-256 | Encrypted SHA-256 |
|---|---:|---:|---|---|
| `dm10010` | 54,192 | 11,344 | `ed192f25cdd86d497fc9d731468ce60f7251c989830b56b2d073222eb524aef7` | `3cd8722d28916d8999363f6d7c51dc0ac2fa69506c8290ae9c3f13cb79d7fbbe` |
| `dm3507` | 54,192 | 11,344 | `9227f29f72847b41706f1aa45e874539d82251b70f4ca34c41331bdef82e144c` | `2094892b6f86aac7a5b2c98c1b4e9845f89bba0bb30f806f7836b1aaad5335fb` |
| `dm3507_48v` | 54,200 | 11,336 | `d6d646e769d99bb505688e92c4627be0f54da60819c486e20562f01a0fbe77c4` | `caf8a43820ceff95f0b87386d0d12e3d38b5931b568fc36b4c16fe9fe3a9e770` |
| `dm4310` | 54,192 | 11,344 | `378528c3e956f627ed166130a051ec37951d736e8469af99a228d6ad7f8c7175` | `2623ebfcdd765d607cb0a3c524b9af50ec491a4c0988fd2575c474d8cb2f6f16` |
| `dm4310_48v` | 54,208 | 11,328 | `75c8539b53be3d1a9193dc40231b5ab928964811988e703cb6f5ec68542d8792` | `71dc1cf20c72fbd762e6e8e00f6b364ef06e93d83d9ba23d9611950203bbdf43` |
| `dm4340` | 54,192 | 11,344 | `4238beee4cc5e4d1841b67360652757ccd3d12aa199cd14c8b422d15514d64eb` | `f8da4991180d2581cf821fdcfd676aad0ca8f007df7a2e84eb814e3f194c4342` |
| `dm4340_48v` | 54,208 | 11,328 | `ae16cf5b6ec484148a4a0a9d3c991bae67b03d7b16ea7d7aff03ab776512fca3` | `6dfbe0ed16c76112495ac623ca4603af0470ae22253d9ee52d2d58b1d7079720` |
| `dm8006` | 54,220 | 11,316 | `34591c4e0c8b7198168319234d81d627ca7cea85643e6f12deef64701029065e` | `65a5675985ef80ca8565b735c9bed50d07a564b618ca2910d5161952289c1962` |
| `dm8009` | 54,212 | 11,324 | `f53bc83b718613a78597d0201ba41e91f2c6294235fc16073a387996ce4a3c5e` | `9aafc0d8e1e0eb8ba40aaa34872790f0d1cf3068f92b978afa64c4b001e92553` |

### No-response 產物

no-response 版位於 `dist/development/no_response/`。它照常接收並執行 MIT／節點控制命令，但略過
命令尾端的自動 motor feedback；`0x7FF` READ/WRITE/LIVE/STORE、discovery、bootloader request 與
必要 ACK 均保留。UART 狀態 banner 為 `DMBOT Motor Driver(no response)`。

| 型號 | Plain/enc 大小 | 剩餘 APP | Plain SHA-256 | Encrypted SHA-256 |
|---|---:|---:|---|---|
| `dm10010` | 54,096 | 11,440 | `b8d1e26a586a53d1dde398d01a74514d12c41a1a40b54a62fcc881dc2d0e70f2` | `11be43b90844ad970a15f707855d0f37e18561929b85eef97458eabbee45c63b` |
| `dm3507` | 54,096 | 11,440 | `5dbd76ee11ee8c76067f6d7b002a3bf6deebef7ad70fa1aedff754548e41d7f3` | `6cd2cb15bd15d1fdb215bbc8c188a7313e9771804cd446192b53c392244da967` |
| `dm3507_48v` | 54,104 | 11,432 | `e2e8cb891d0bb89a886d0d7b2e3fa715a909c6f6a649f62ef53f7621d296f70a` | `4b998f791a46087f6e3ae97eb4240e85f03b164a13b1b608d515f759313c33e8` |
| `dm4310` | 54,096 | 11,440 | `ffebfa555a122a899a5d9eb531d861b76bd65a0b1aa40752c807ce3164acf15e` | `c29968358540a16162287e5029da2695106fb9bcfa5225e9203c6c591eea9562` |
| `dm4310_48v` | 54,112 | 11,424 | `be9ef71a6c1f3988f2c7316523481e6061634309b25c9c4b7ea6ad235a8ad3cb` | `e8093c50c59a901b21110006a75f6d2882322f317ed9a386e69532b6775b0ca6` |
| `dm4340` | 54,096 | 11,440 | `658185d779fb48a2d2a639026fb0452d81523991d743306686e0950a35d4a68a` | `f0fc69b23da4d093a215641286bf8e1f4f8e684f43cd03cc9db3d12271722525` |
| `dm4340_48v` | 54,112 | 11,424 | `78d713e994a6e332d3450008d05e6d257a3ae0a44132cc170de6dfc928409e7b` | `4ee6f15b9b7001f8fad642ddf2f3b2ea516d0987eb1185b7b43e9ec955d37f87` |
| `dm8006` | 54,116 | 11,420 | `09faaae796ff5b9d95b69984b32d99874dd8313825b86544ce51f377bc6a4aec` | `dae4b77db95b9bf6419885b33b694c04800d280cb8d92ce02f5a4a756b1f850a` |
| `dm8009` | 54,108 | 11,428 | `d8f4e87aade117d27184d5a508ebb56918dfc98645446cf7b9a7b0604b50b86a` | `f9e26ba4d23404c8d5d96cb5fa122200f028eb088b453b4bfc14bee00ff69c66` |

### Raw + no-response 產物

組合版位於 `dist/development/raw_no_response/`，同時使用 raw 速度／扭矩並取消控制命令的自動
feedback。UART 狀態 banner 為 `DMBOT Motor Driver(with raw result and no response)`。

| 型號 | Plain/enc 大小 | 剩餘 APP | Plain SHA-256 | Encrypted SHA-256 |
|---|---:|---:|---|---|
| `dm10010` | 54,176 | 11,360 | `1f9b1bba7b4fcb48a8a3c7ba76c7860adc71ce86dcd5fe3e5fe06d61d4e5dd88` | `82aaedbac2c79b739e5ff02d7328ff2c7cdb84ddb45dc2610ca00f2a32fa2952` |
| `dm3507` | 54,168 | 11,368 | `2226e79e49f7470aea6823ecfe14d871bea4671635cbe4599ce11895e7231284` | `3f3a256a274dcc3bb5b65847728f23f658ae35f4956969bef593ae422d184f11` |
| `dm3507_48v` | 54,184 | 11,352 | `a8d37db8218349e389f824de619a988444445b609e90b27a4e2737ba1a2c2f4d` | `9414bde3c4a6235a95d00bc27d106a09d2d2a9639b7f2ef1152eaeef90110a32` |
| `dm4310` | 54,176 | 11,360 | `d935ad32c2ec726c1660f97aa134a98f8c56144f9dd8d7f04776db5f4a63d9b7` | `5614641bc49050931f4ddb43f94b4dc2c95ef27b913f3f163887ccd8af5dc56f` |
| `dm4310_48v` | 54,184 | 11,352 | `ec80eb153a23ecf930b4100fa4ca7dc4c1f96e4957876ea6ea705ca1e57e27b6` | `815b4683dbe37c71d2ca8ea0721d0db7ef080da286c411d7cdb07f9af0b45793` |
| `dm4340` | 54,176 | 11,360 | `f5d634932eea4f0b3a28e203a21f1909b135895a00b428ecd2c76ffc5751c57d` | `2e408449f805b254cf06b2275c39fdc211ebaa56461916a2f3d497b55a86e039` |
| `dm4340_48v` | 54,184 | 11,352 | `ad3dda9247ba4a7b3544a49c528bdb94364a99c7e4895531e6c8e4ed3dc1d0cf` | `94f856096eba5612dab6361e9891a67de576b52bf6a1d8ca3c46e7aa453d33b0` |
| `dm8006` | 54,188 | 11,348 | `6e0a2f7ad13a00ef75b1026795d924ea58ea91e4879896904ef0b2d7466d701c` | `8b33ab2dfcd4f309b2379998f69bc4bcba1fa429eaca7d413dae9fbbac89505c` |
| `dm8009` | 54,188 | 11,348 | `b91f5c7b3408748a0364a307763de2a871f23e682b1a4363dc8b857207b71879` | `6aefe3e83af8bae3b33cc571b48a5ffc439b9d6448dc04035abca795da1aa561` |

## 完成證據

- 九個 target 現在各自只定義一個 `DAMIAO_MODEL_*`；DM800x 與 DM43 48V 的差異由
  `DAMIAO_LAYOUT_*` 明確描述，不再以 `DAMIAO_DM4310` 假裝成同一型號。
- 產品碼中的舊 DM4310 compatibility 條件已展開成唯一有效路徑，未使用替代實作已刪除。
  共用 formatter/arithmetic、memory layout、linker 檔名與公開資料型別都使用產品語意；
  `FUN_*`、`Vector_*`、model-named shared section 等逆向式識別字不留在產品 source tree。
- 一般 C/H 已套用一致格式；固定 SRAM 與算術 ABI 仍由語意化 linker section、型別與必要
  ASM 明確表達。最終 build 後重新執行 585/585 差分與 36 份 image-layout 驗收。
- `tools/recovery/verify_source_architecture.py` 獨立檢查 model/profile/layout 分離、共用 V3
  檔案命名與 recovery 工具隔離，不加入一般 CMake/make 流程。
- 九型號 factory/source 全量差分合計 585/585。驗證器實際執行 factory binary 與 source
  ELF，比對暫存器、固定 SRAM、MMIO、呼叫 ABI、FPSCR 與存取順序，不以函式 hash 代替行為。
- 九型號核心組合合計 135/135，涵蓋 Reset/SystemInit/scatter/runtime/main、五個 IRQ、
  deferred event、周邊初始化、commissioning、motor identification 與 output calibration。
- 九型號的四種 variant 共 36 個映像都通過 64 KiB、vector、VMA/LMA、copy-down、zero-fill、RAMB、
  heap/stack、fixed helper 與 recovered state section 的地址檢查，無區段重疊。
- factory recovery 基線由 `make firmwares` 產生於 factory variant；raw measurement 與 no-response
  是兩個正交能力。四種組合都逐一驗證 source identity、AES round-trip 與 manifest。
- 獨立 `verify_feedback_variants.py` 在三種 SRAM layout 執行實際 encoder 與 LIVE selectors 1–4，
  九型號均確認 filtered/raw 資料來源、四個精確 banner、控制回覆開關及保留的 `0x7FF` 參數回覆；
  ID、DLC、位置、溫度與 fault bytes 不變。此工具不掛入一般 Make 流程。
- 產品建置只使用 C/ASM、生成常數與 linker script；factory/decrypted binary 只供獨立差分驗證，
  沒有進入 CMake/make 產品依賴圖。
- 65 支持久化差分驗證器、15 支核心 runner、共用 Unicorn loader、Ghidra 匯出器及地址映射
  工具均保存在 `tools/recovery/` 或 `recovered/*/tables/`；`/tmp` 不承載唯一分析成果。
- 固定配置只存在於硬體需要的 SRAM ABI 與 linker memory layout。Recovery regressions
  不屬於一般 CMake/make 編譯流程；一般 Flash 函式沒有 padding 或 address anchor。
- 私有 C 型別已撤除無必要的固定 offset assertion；真正由 SRAM/IRQ/ASM 取用的型別改用
  `SRAM_ABI_ASSERT_*` 明確標示。這些 compile-time 檢查不占映像空間。
- linker 直接匯出可開發餘裕：factory Flash 尚餘 11,396–11,424 bytes，四種 variant 最低仍有
  11,316 bytes；`.app_state` 尚餘 1,760 bytes，一般 BSS 尚餘 24,396 bytes。新增功能不必修改
  固定 SRAM 物件。

## 最終交付閘門

- [x] 九型號完成 factory 掃描、source recovery 與個別 profile/fixed-SRAM layout。
- [x] 九型號完成 65/65 全量差分、15/15 核心組合與映像驗證。
- [x] 九型號完成 plain/enc 封裝、AES round-trip、manifest 與 source identity 驗證。
- [x] 重建九型號四種行為組合；共 36 個 ELF/app 映像與 72 份最終 plain/enc bin。
- [x] 一般 Flash factory anchors 已移除；測試未加入正常產品編譯流程。
- [x] 每個 target 只有自己的 model identity；共用 source 與 SRAM layout 選擇已解耦。

此表的「完成」指 disassembly 與 factory/source differential 所能驗證的 source-recovery
範圍；實機功率級、馬達負載與各硬體 revision 的安全測試仍需依硬體驗收流程執行。
