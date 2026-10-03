# 12. 證據、完成度與已知邊界

## 已完成的 source recovery

目前九個 target 均已具備：

- 獨立 model identity、profile與正確固定 SRAM layout；
- 全 C／ASM source build，沒有連結 factory binary；
- 65/65 持久化 factory/source differential；
- 核心組合、五個 IRQ、startup/main、commissioning與周邊驗證；
- vector、64 KiB、VMA/LMA、copy-down、BSS、RAMB、heap/stack與 section overlap 驗證；
- plain/enc 封裝、manifest與 AES round-trip；
- 一般 Flash 函式沒有 factory address anchor或 padding trick。

完整逐 target hash、大小與 gate 表以 [FIRMWARE_RECOVERY_PROGRESS.md](../FIRMWARE_RECOVERY_PROGRESS.md)
為準，避免本章複製一份會過時的數字。

## 「完成」的精確意思

source recovery 的完成表示：disassembly 可見的功能已歸責到可維護 source，指定 factory/source
差分通過，固定 ABI與映像契約可重現，九型號能建置與封裝。

它不自動表示：

- 所有未探索輸入都已形成形式證明；
- 每個 board revision、sensor lot、gate driver都已實測；
- 高壓、最大速度、最大扭矩、堵轉與熱穩態皆已安全驗收；
- 新增功能後仍可沿用舊差分結論而不重測。

## 為什麼不追求整檔 byte equality

factory 使用不同 compiler/runtime與函式配置。要求整檔 byte equality會迫使一般 C 函式固定地址、
塞 padding或重建舊 toolchain副作用，導致後續改一個函式就 overflow。現行策略是：

- 普通 Flash code只要求行為、協定與安全語意；
- 固定 SRAM code/state、persistent format、IRQ/MMIO順序保留精確契約；
- 以 differential、map、hardware trace分別驗證它們。

目前 source plain 比對應 factory 約大 5%，但仍保留約 11.7 KiB APP空間，且一般函式可自由成長。

## Recovery 工具的角色

`tools/recovery/` 與 `recovered/*/tables/` 保存可重現分析成果。它們：

- 可以讀 reference/factory與 source ELF做比較；
- 不會被 CMake/Makefile產品 target編譯或連結；
- 不應把 absolute Flash function address變成日常開發 gate；
- 應保留真正的 ABI、SRAM、MMIO、FPSCR與行為檢查。

檔名仍可能帶 `dm4310_`，因為差分框架最早由 DM4310 建立；是否支援其他型號應看工具的
`--model` 選項，不要僅憑歷史檔名判斷範圍。

## 已知高風險邊界

- power-stage、ADC scale、相序與 sensor硬體一定要上板證明。
- motor identification雖有數學與差分證據，仍是主動驅動流程。
- 48 V target必須使用對應硬體與限壓流程；不可由 24 V結果外推。
- bootloader本身不在此工作區重建；更新相容性依賴既有 loader與版本化 profile。
- profile default可能被裝置既有 persistent config覆蓋；刷 APP不等於恢復 factory設定。
- malformed legacy協定中有為相容性保留的特殊存取語意，擴充 parser時需避免「順手修正」造成
  不相容或記憶體風險。

## 後續開發的 Definition of Done

一項功能可交付至少要有：

1. ownership 清楚，沒有將 target特例散落共用模組。
2. 九型號或明確受影響 target可建置，容量與 SRAM餘裕可接受。
3. 對應 unit/differential/image-layout驗證；測試碼不進產品映像。
4. IRQ與 persistent format若有變動，完成順序、斷電與 reset測試。
5. 硬體相關變更依 Gate 0–6完成相稱層級的實機驗收。
6. source map、修改章節、對外 protocol與驗收紀錄同步更新。
7. 產物與 source commit、target、toolchain、hash可追溯。

做到這些，這個工作區才是一個可持續開發的韌體專案，而不是只能重現一次的逆向快照。

[上一章：上板驗收](11-validation-recovery.md) · [回到目錄](README.md)
