# DM4310 V3 V5017 復原進度

最後更新：2026-10-03

九型號總進度與最終產物見 `docs/FIRMWARE_RECOVERY_PROGRESS.md`；日常開發入口與限制見
`docs/README.md` 與 `docs/book/README.md`。
Factory byte-exact 目標已停止；`docs/DM4310_BYTE_EXACT_PROGRESS.md` 僅保存歷史分析。
目前完成標準是 factory 行為、固定 SRAM/MMIO ABI、64 KiB 映像約束及可持續開發的
source build，不要求 Flash 函式位址或 whole-file SHA 相同。

## 目前基準

- Factory APP：`recovered/binaries/dm4310/dm4310_v3_v5017_app_flash_00020000_memory.bin`
- Factory SHA-256：`65aab219268e9159b196d4578d3cd530e6fa90a731a82670a0d3d3be609b59d4`
- Factory image：51,284 bytes
- Source-built APP image：54,120 bytes（factory +2,836 bytes，+5.53%）
- 64 KiB APP 剩餘空間：11,416 bytes
- Source-built plain SHA-256：`6aae35703a2a4119c83af8dc63d854bd7009ca706cbaa203dcccedfe3afee069`
- Source-built encrypted SHA-256：`6ab4945dd9f14c945785e32af187673bf6afd2c37be87c89f09200701cb36348`
- 行為叢集差分：65 / 65 通過（100%）
- Factory function closure：195 / 195 已建立 source/fixed-runtime owner
- Ghidra unowned instruction ranges：6 / 6 已分類（3 callable、3 data）

`65 / 65` 只表示目前已建立的 factory/source 行為差分叢集，不代表整個韌體完成度。
完成必須同時通過下列所有閘門。

## 完成閘門

### 1. 既有全域回歸：65 / 65

- [x] UART IRQ 完整命令矩陣
- [x] ADC IRQ retained-pointer 完整路徑
- [x] MCAN feedback IRQ 路徑
- [x] direction/pole-pair、current-sensor、debug-status、firmware-control、DMA pool
- [x] alignment cluster
- [x] main-loop cluster
- [x] observer slice
- [x] offset-sweep cluster
- [x] motor-identification outer flow
- [x] output-calibration outer flow

剩餘叢集必須比對可觀察的記憶體/MMIO 存取順序、狀態、呼叫 ABI 與浮點旗標；不能只更新
測試位址讓它略過實際差異。

### 2. 全域 factory 覆蓋掃描

- [x] 重新匯出 factory Flash 與 fixed-SRAM 的完整函式 inventory、call graph 與區段空洞。
- [x] 對 195 個 closure entry 建立 source/fixed-runtime owner；零個僅 role/signature identified 項目。
- [x] 以實際 Ghidra function body 掃描 instruction ownership；6 個 gap 均已分類。
- [x] 批次掃描分支、IRQ 路徑、固定 SRAM/MMIO address 與存取寬度，差異回到 Thumb-2
      disassembly 修復。

### 3. 組合流程

- [x] 從 reset/scatter-copy 經 board/peripheral 初始化進入 main loop 的完整組合驗證。
  - [x] Reset/SystemInit/scatter/runtime 到 main entry，8 種 clock-source/PRIMASK 組合。
  - [x] board/peripheral 初始化各自的完整 MMIO、固定 SRAM 與 FPSCR 矩陣。
  - [x] 同一台模擬機從 Reset 連續進入第一輪 deferred loop；8 種 clock/boot-confirm/
        hardware-variant 組合驗證 parent call order/ABI、固定狀態、VTOR 與 FPSCR。
- [x] 五個有效 IRQ 與 main-loop/deferred-event 交互驗證。
  - [x] IRQ000/IRQ001 共 512 案例，包含無 source-only 全域 SRAM 寫入檢查。
  - [x] IRQ002、IRQ003、IRQ004 個別完整路徑已有持久化差分驗證。
  - [x] IRQ003 發布 CAN error 後直接銜接 main 消費、列印及清除，64 案例。
  - [x] IRQ002 outer-loop tick/fault-indicator 銜接 main，16 案例。
  - [x] IRQ004 發布 9 種 calibration/commissioning/motor-ID/firmware 工作後銜接 main。
- [x] calibration、commissioning、motor identification、parameter/CAN/UART protocol 的 caller
      sequencing 與非正常分支驗證。
  - [x] main 17 案例涵蓋所有單一 deferred event、同時事件優先序、三種 calibration request。
  - [x] direction/alignment、output calibration 與 motor-ID outer flow 涵蓋完整 epilogue；motor-ID
        額外涵蓋 negative-R、negative-L 兩條不恢復 IRQ 的 factory failure path。
  - [x] UART 完整命令/負長度/retained-byte/reset matrix，加上 9 條 IRQ→main 發布路徑。
  - [x] MCAN RX/tail、accepted/rejected WRITE、armed/malformed upgrade、legacy persist 與
        IRQ→main CAN error 路徑均由持久化差分驗證覆蓋。

### 4. 映像與容量

- [x] APP load image 為 54,120 bytes，嚴格小於 65,536 bytes；linker 另對所有 sparse
      `AT(...)` LMA 的最終尾端強制檢查，剩餘 11,416 bytes。
- [x] vector、42 個 file-backed LOAD segment、45 個初始化 RAM section、35 個 zero-fill
      section、RAMB、fixed helper、heap/stack 均無 VMA/LMA 重疊。
- [x] 全域存活函式 body 雜湊掃描沒有任何 >=16-byte 重複實作；僅有四個必要的 8-byte
      fixed-helper veneer。未為縮小映像刪除行為或破壞固定地址契約。
- [x] 一般 Flash code 使用連續配置，不再鎖定 factory function address、插入 factory padding
      或要求函式維持特定長度；一般函式可隨後續開發自由增減，直到真正碰到 64 KiB 上限。
- [x] 全域採 GCC 14 `-Os`；`safety_update()`、`output_atan2f()` 與 `svpwm_result()` 經量測後
      個別採 `-O2`。三個例外使 ELF text 增加 264 bytes，但 96 組 IRQ002 路徑合計由 91,064
      降至 87,032 指令（平均每 tick -42，約 -4.4%）。同一份 source 的全域 `-Oz` 產物逐 byte
      相同；全域 `-O2`/`-O3` 分別增至約 57.8/61.5 KiB，因此不採用。
- [x] 不採用 LTO。探測顯示 LTO 會跨越 hand-audited fixed-SRAM IRQ/literal ABI；這類容量
      捷徑會使 compatibility layer 脆弱，沒有進入正式配置。

### 5. DM4310 收尾

- [x] 將 65 支全域驗證器、核心驗證器與共用 loader 保存在 `tools/recovery/`，不依賴 `/tmp`。
- [x] 驗證器保持獨立執行，不加入一般 CMake/make 編譯與韌體執行路徑。
- [x] 現行程式與驗證器已清除暫時 hook、過期位址及錯誤 V4/V7318 移植；DM8009
      V3/V6417 的差異改由 target profile、固定 SRAM layout 與少量 model 分支表達。
- [x] 全量 build、65/65 全域差分、15/15 核心組合、736/736 formatter、factory/fixed-SRAM
      contract、image layout 與 `git diff --check` 全部通過。
- [x] 已產出 `dist/development/factory/dm4310_plain.bin` 與 `dm4310_enc.bin`，並驗證 plain 等於
      ELF objcopy、AES-256-CTR round-trip、manifest hash、向量及長度。

固定 SRAM code/literal 只存在於 `sram_runtime.c` 與專用 linker compatibility layer；其位址是
原廠 IRQ、PC-relative literal 與 retained SRAM ABI 的一部分，不是一般程式配置技巧。後續新增
產品邏輯不應放入這些 section。DM4340 與正確 V3/V6417 DM8009 reference 的重新復原另列為
後續工作，不以舊的錯誤移植產物宣告完成。

## 已永久保存的核心差分驗證

- `tools/recovery/regressions/`：65 支全域 factory/source 差分驗證器
- `tools/recovery/verify_dm4310_full_regressions.py`：全域驗證 runner

- `tools/recovery/regressions/dm4310_uart_irq_verify.py`
- `tools/recovery/regressions/dm4310_adc_retained_pointer_verify.py`
- `tools/recovery/regressions/dm4310_mcan_feedback_verify.py`
- `tools/recovery/verify_dm4310_position_irqs.py`
- `tools/recovery/verify_dm4310_adc_main_composition.py`
- `tools/recovery/verify_dm4310_irq_main_composition.py`
- `tools/recovery/verify_dm4310_uart_main_composition.py`
- `tools/recovery/verify_dm4310_startup_composition.py`
- `tools/recovery/verify_dm4310_boot_main_composition.py`
- `tools/recovery/verify_dm4310_image_layout.py`
- `tools/recovery/verify_dm4310_peripheral_init.py`
- `tools/recovery/regressions/dm4310_alignment_cluster_verify.py`
- `tools/recovery/regressions/dm4310_main_loop_verify.py`
- `tools/recovery/regressions/dm4310_observer_slice_verify.py`
- `tools/recovery/regressions/dm4310_offset_sweep_cluster_verify.py`
- `tools/recovery/regressions/dm4310_motor_id_outer_verify.py`
- `tools/recovery/regressions/dm4310_output_calibration_outer_verify.py`
- `tools/recovery/verify_dm4310_core_regressions.py`
- `tools/recovery/dm4310_unicorn.py`

這些腳本只讀 factory reference 與 source-built ELF；不會把原始 binary 連結進韌體。
以上是離線 disassembly、差分執行與映像層面的完成；真實馬達、功率級與最差中斷延遲仍需
實機驗收，不能由 Unicorn 模擬結果取代。
