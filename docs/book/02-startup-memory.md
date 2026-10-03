# 2. startup、記憶體與固定 SRAM ABI

本章是修改 linker、IRQ、固定狀態或 runtime helper 前的必讀章節。最大的原則是：**普通 C 程式可
自然增長，只有明確標示的固定 SRAM 介面需要保留位址、大小或欄位 offset。**

## Reset_Handler 做什麼

`startup/startup_hc32f448.S` 的 vector table 位於 APP `0x00020000`。Reset 後依序：

1. 保留 loader handoff／reset 的中斷狀態，啟用 Cortex-M4F 所需環境。
2. 呼叫 `SystemInit()`。
3. 複製固定 SRAM IRQ、helper、常數、一般 `.data` 與 RAMB data。
4. 清除固定 BSS、`.app_state` 與一般 `.bss`。
5. 建立 37-word scatter defaults、commissioning defaults 與 sine table。
6. 執行 init array，進入 `main()`。

目前 image-layout 驗證會確認 45 個 initialized SRAM section 都有正確 load image 與 startup copy。
新增帶初值的 SRAM section 時，不能只改 linker；必須同步加入 startup copy，否則 VMA 看似存在、
上電後卻仍是垃圾資料。

## 記憶體分區

| 區域 | 範圍／位置 | 用途 |
|---|---|---|
| APP Flash | `0x00020000..0x0002FFFF` | vector、普通 `.text/.rodata`、SRAM load image；64 KiB 硬上限 |
| 主 SRAM alias | `0x1FFF8000..0x20007FFF` | 固定 code/data/state、stack、`g_app`、一般 BSS |
| RAMB | `0x200F0000..0x200F0FFF` | linker 支援的獨立資料區 |
| `g_app` | `0x20001600` 起 | 可維護、可擴充的應用狀態 `.app_state` |
| 一般 BSS | `0x20002000` 起 | 不需要 factory 固定位址的新狀態 |

標準 layout 的目前開發餘裕為：`.app_state` 1,760 bytes、一般 BSS 到 SRAM 末端 24,396 bytes；
九個 target 的 Flash 餘裕為 11,664–11,688 bytes。這些數字可由 ELF linker symbol 重新查詢：

```sh
tools/arm-gnu-toolchain/bin/arm-none-eabi-nm -n build/dm4310.elf \
  | rg '__flash_free_bytes__|__app_state_free_bytes__|__source_bss_free_bytes__'
```

不要只看 `.bin` 大小判斷能否新增功能；同時檢查 map、BSS、stack 使用量與 IRQ latency。

## 三種固定 SRAM layout

| layout | target | 特性 |
|---|---|---|
| standard | `dm3507`、`dm4310`、`dm4340` | 基準固定 SRAM 位址 |
| shifted | `dm10010`、三個 `_48v` target | `0x1FFF9878` 之後的指定 runtime 物件平移 4 bytes |
| relocated | `dm8006`、`dm8009` | IRQ/helper 與 runtime state 使用另一組地址 |

選擇器在 `config/targets/*.cmake`，由 `memory_layout.h` 的 `MEMORY_LAYOUT_ADDRESS()`／
`MEMORY_LAYOUT_SECTION()` 傳到 C，並由 `linker/hc32f448_v3_app.ld` 在 link time 套用。compiler 與
linker 必須選到同一 layout；不能只改其中一邊。

## 哪些東西是固定 ABI？

V3 linker 明確配置：

- IRQ000..IRQ003 的 SRAM code（IRQ004 仍在 Flash text 區）；
- fault monitor、output sensor、MCAN send、SVPWM、controller、observer、commissioning math 等
  helper／kernel；
- current/sample/runtime status、protocol scratch、correction table、UART buffer 等固定狀態；
- Flash erase/program primitive，因 internal Flash 操作期間不能安全地從同 bank 取指令；
- fixed allocator／runtime 與原韌體 callback 所使用的地址。

完整地址由 linker script 與 `verify_dm4310_image_layout.py` 管理，不要在一般模組另抄一張常數表。
實際 ELF section 可查：

```sh
tools/arm-gnu-toolchain/bin/arm-none-eabi-objdump -h build/dm4310.elf
```

## veneer 與完整 SRAM kernel

`app/src/sram_runtime.c` 中多數 entry 是很小的固定 veneer：固定地址只負責維持呼叫 ABI，真正
實作仍位於普通 C 模組。這類功能要增加邏輯時，擴充 veneer 呼叫的 C 函式，不要把不相關程式
塞入固定 slot。

少數 IRQ／kernel 本身必須完整留在 SRAM，linker 會檢查其上限或 exact size。若真的需要修改：

1. 先辨識它是 fixed veneer 還是完整 kernel。
2. 查看 linker 中該 section 的下一個邊界與 ASSERT。
3. 保留呼叫 ABI、寄存器／FPSCR／MMIO 順序。
4. 建置所有三種 layout，而不是只建 `dm4310`。
5. 執行 image-layout、startup-copy 與對應 factory/source 差分。

## 結構 layout assertion

`SRAM_ABI_ASSERT_SIZE/OFFSET` 只用於會被固定 SRAM code、ASM、IRQ 或 persistent storage 以 offset
存取的型別。它們是 compile-time check，不占 Flash 或 SRAM。

一般 C-only private struct 不應為了「看起來更安全」而鎖死 layout。新功能狀態優先加入 `g_app`、
`.app_state` 或一般 `.bss`；不要隨意擴充 `RuntimeStatus`、`MotorRuntimeState`、
`SampleRuntimeState` 等固定物件。

## stack、heap 與 allocator

newlib `_sbrk()` 明確失敗，因此沒有一般 C heap，也不應在控制路徑使用 `malloc`。原 runtime 相容
層另有固定 1 KiB allocator，主要服務既有 formatter／協定語意；它不是給新功能任意配置的 heap。

新增 buffer 時用有上限的靜態儲存，並評估：

- 是否需要跨 IRQ／main 共用，是否要 `volatile` 或 critical section；
- 是否屬於固定 ABI，若不是就放 `g_app`／一般 BSS；
- 最壞 stack frame 是否觸發 `-Wstack-usage=1024`／`-Wframe-larger-than=1024`；
- DMA 所需 alignment、生命週期與 cache／bus 可見性。

## 修改檢查表

- 不手動固定普通 Flash 函式地址或加入 padding。
- 不因新增一個 state 就挪動固定 SRAM section。
- 不移除 linker ASSERT 來「讓它先編過」。
- 不把 Flash writer 拆到普通 `.text`。
- 不更改 hard-float ABI 或 `-ffp-contract=off` 而不重做差分。
- 至少建置 standard、shifted、relocated 各一個 target，再完整建九個 target。

[上一章：架構](01-architecture.md) · [下一章：板級周邊](03-board-peripherals.md)
