# DM4310 V3 V5017.04 byte-exact 復原進度（已停止）

最後更新：2026-10-03

本頁保存停止 byte-exact 工作時的歷史證據，不再是完成閘門。專案已改採可持續開發的
連續 Flash 配置；不固定一般函式位址、不保留 factory padding，也不要求 whole-file SHA。
現行行為、SRAM/MMIO、ABI、容量與流程完成度以
`docs/DM4310_RECOVERY_PROGRESS.md` 為準。下列數字均是停止前的歷史 checkpoint。

## 完成定義

- Plain image 長度為 51,284 bytes。
- Plain SHA-256 為
  `65aab219268e9159b196d4578d3cd530e6fa90a731a82670a0d3d3be609b59d4`。
- 144 個 vector words、191 個 factory Ghidra function bodies、固定 SRAM source、
  scatter-compressed initialized state、literal pools、padding 與尾端 metadata 全部相同。
- 產物只由 C、具名 ASM、生成資料與 linker/scatter 描述建立；禁止連結 factory binary、
  `.incbin` 或以 `.byte` 大量包裝原映像。
- Byte-exact 分析與驗證保持在 `tools/recovery/`，不加入一般 make 的測試流程。

## 2026-10-03 進度表

| 指標 | 初始 | 目前 | 目標 |
|---|---:|---:|---:|
| Plain image size | 62,360 | **59,472** | 51,284 |
| 超出 factory | +11,076 | **+8,188** | 0 |
| 64 KiB 剩餘 | 3,176 | **6,064** | 14,252 |
| 同位址相同 bytes | 2,870（5.60%） | **6,904（13.46%）** | 51,284（100%） |
| Common prefix | 4 bytes | 4 bytes | 51,284 bytes |
| Vector words | 13 / 144 | 13 / 144 | 144 / 144 |
| Factory function bodies | 12 / 191 | **30 / 191** | 191 / 191 |
| Flash function bodies | 0 / 161 | **18 / 161** | 161 / 161 |
| Fixed-SRAM function bodies | 12 / 30 | 12 / 30 | 30 / 30 |
| Fixed-SRAM source sections | 24 / 44 | 24 / 44 | 44 / 44 |
| Whole-file SHA | 不同 | 不同 | 相同 |

目前 function-body 進度：`[███░░░░░░░░░░░░░░░░░] 30/191（15.71%）`。
目前同位址 byte 進度：`[███░░░░░░░░░░░░░░░░░] 6,904/51,284（13.46%）`。
這兩條只表示嚴格 byte 證據，不拿行為已完成的函式灌入分子。

Function denominator 使用 Ghidra 真實 body ownership，其中 27 個 body 含非連續 ranges；
不能以 entry 到最大位址的整段範圍替代。Function-owned byte 總數 38,444，因 shared-tail
ownership 會重疊，只作 function 級進度，不作映像大小計算。

## Load image 差額

| 組成 | Factory | Source build | 差值 |
|---|---:|---:|---:|
| 一般 Flash code/data | 34,432 | 52,844 | +18,412 |
| Fixed-SRAM load payload | 9,488 | 6,372 | -3,116 |
| Compressed/generated state tail | 7,364 | 256 | -7,108 |
| 最終映像 | 51,284 | 59,472 | +8,188 |

目前的 8,188-byte 淨差掩蓋了一般 Flash 實際膨脹 18,412 bytes。Source build 以 sparse
fixed-SRAM LMA 與啟動期 table reconstruction 省下約 10 KiB，而 factory 使用 Arm scatter
compression。因此不能先把 factory scatter payload 加回來再處理 code size。

目前 52,844-byte 一般 Flash 可再分成 47,958 bytes linked inputs、3,572 bytes
factory-address placement fill，以及 1,314 bytes linker/manual tables。已將原本放在映像後方的
extended/soft-float runtime 搬回 factory ranges，並刪除四個重複 C helper，因此相較初始基準
縮小 2,888 bytes。剩餘洞仍不是單純可刪除 padding：factory 原本在相同地址範圍內有實際
函式。收斂方式仍是把對應實作移回 factory ranges 並移除後方重複實作。

本 checkpoint 已完成：

- 將 extended divide/round/multiply、softdouble add/subtract/multiply/divide、exception selector、
  float/double conversions 與 literal/seed bank 放回原廠位址。
- `0x27074..0x27c9f` 形成最長 1,786-byte 連續相同區段；另有
  `0x212f0` 起 1,708 bytes 完全相同。
- 由後方移除 `double_to_uint32`、`uint32_to_double`、`uint64_to_float`、`double_to_float`
  的重複 C machine code；保留具名原廠 ASM，不連結 factory binary。
- 三型號 build 通過；DM4310 65/65 全量差分、15/15 核心組合與 image-layout 全部通過。

## 工具鏈 fingerprint

本機已找到 `/home/umeow/ARM_Compiler_5.06u7/`，版本為 ARM Compiler 5.06 update 7 for
Certification build 960；但 `armcc`、`armlink`、`fromelf` 都因未設定合法
`ARMLMD_LICENSE_FILE` 而拒絕執行。目前不能用它做 codegen probe。現有證據仍將原廠候選
縮到 ARMCC 5.06 家族：

- HC32F448 DDL Rev1.3.0 內 179 個 `.uvprojx`、358 個 targets 全部指定
  `5060020::V5.06 (build 20)::ARMCC` 且 `uAC6=0`。
- 179 個 Release targets 全部使用 `Optim=4`、`oTime=1`；Debug targets 使用
  `Optim=1`、`oTime=0`。
- DDL 附帶的 M4 safety library `.comment` 顯示 ARM Compiler 5.06 update 6
  build 750。它不是 DM4310 factory 物件，只證明這套 DDL 的 ARMCC 5 工具鏈脈絡。
- Factory 使用 Arm scatter descriptors/compression；`SystemInit` 與
  `SystemCoreClockUpdate` 已對應到同一份 DDL source。

候選驗證順序：

1. ARMCC 5.06 build 20，Release 最佳化與 Cortex-M4/FPv4-SP 設定。
2. ARMCC 5.06 update 6 build 750，相同設定。
3. 已存在但待授權的 ARMCC 5.06 update 7 build 960。
4. 其他 5.06 maintenance updates；只有前三者不匹配才擴張搜尋。

取得合法可用 compiler 後，先以相同 DDL source 的 `SystemInit`、
`SystemCoreClockUpdate` 及數個短 leaf driver functions 比較 instruction、literal pool、
section naming 與 relocation。若短函式無法收斂，停止全面移植，改走只對必要函式使用
具名 ASM 的 hybrid 路線。

GCC IPO/LTO 已在兩個獨立 build directory 做過不修改正常 CMake 的探測。Release LTO
在 fixed division-seed anchor `0x27450` 前增長到 `0x28efc`；MinSizeRel `-Os` LTO 仍增長到
`0x28b78`。兩者都被 linker 的 backward-location guard 正確拒絕。LTO 會改變 section
identity/placement 並破壞目前 factory anchors，不能當作容量或 byte-exact 捷徑。

## 執行順序

1. **Evidence/dashboard：完成。** 保存 191 個 function body ranges，建立可重複 parity
   報告，固定 factory SHA 與初始分母。
2. **Compiler go/no-go：等待合法 license。** ARMCC 5.06u7 build 960 已找到；取得 license
   後驗證 build/update、Release flags、ABI、microlib/full-library 與 scatter compression。
3. **Factory layout：進行中。** 已鎖定 arithmetic runtime、division seeds、temperature table；
   下一批鎖定 vector `0x20000`、startup `0x20250`、fixed source `0x28680`、image end
   `0x2c854`。
4. **Function convergence：30/191。** 每個 entry 同時鎖定位址、body ranges、長度、SHA 與
   literal pool；先 startup/runtime/leaf，再 protocol/control/large IRQ。
5. **Scatter/state convergence。** 從 source initializer 生成與 factory 相同的 compressed
   state，不複製 factory bytes。
6. **Final gate。** Whole-file `cmp`、SHA-256、AES-CTR encrypted image 與既有 65/65、
   15/15 行為回歸全部通過。

## 重跑 dashboard

```sh
.venv/bin/python tools/recovery/report_dm4310_byte_parity.py \
    --details-output build/recovery/dm4310_byte_parity_functions.tsv
```

詳細 TSV 是可重建的 build evidence；唯一分析邏輯與 factory body ranges 均保存在
repository，不依賴 `/tmp`。
