# 1. 架構與執行流程

## 系統邊界

這個 repository 只產生馬達控制 APP。裝置上的完整系統仍包含既有 bootloader、APP、OTP 裝置
金鑰與多個持久化 Flash sector：

```text
0x00000000  ┌─────────────────────────────────┐
            │ 既有 bootloader（不在本專案建置）│
0x0001E000  ├─────────────────────────────────┤ boot record / loader 狀態
0x00020000  ├─────────────────────────────────┤
            │ V3 APP，最多 64 KiB              │
0x00030000  ├─────────────────────────────────┤ APP partition 結束
            │ 其他保留／產品資料 sector        │
0x00036000  ├─────────────────────────────────┤ output / motor zero offsets
0x00038000  ├─────────────────────────────────┤ output sensor 4-float calibration
0x0003A000  ├─────────────────────────────────┤ 4096-entry output correction table
0x0003C000  ├─────────────────────────────────┤ 259-word motor encoder record
0x0003E000  ├─────────────────────────────────┤ 37-word產品設定
            └─────────────────────────────────┘
```

`_plain.bin` 是從 APP ELF objcopy 出來、以 `0x20000` 為基準的明文；`_enc.bin` 是既有 loader
接受的加密 payload。二者內容用途不同，詳見 [第 7 章](07-bootloader-update.md)。

## 軟體分層

```text
外部主機
├── motor CAN command / feedback
├── 0x7FF parameter protocol
└── UART setup / calibration / firmware-control
                │
                ▼
產品層 app/src/
├── protocol decode 與命令語意
├── configuration / safety / calibration
├── position、speed、current control
└── commissioning
                │
                ▼
協調層 app/src/platform.c
├── 定義開機與周邊呼叫順序
├── 串接固定 SRAM 狀態
└── 將慢速 Flash 工作留給主迴圈
                │
                ▼
板級層 board/src/
└── HC32F448 register、DDL、GPIO、ADC、SPI/DMA、TMR4、MCAN、UART、Flash
```

分層的判斷方法很簡單：命令「代表什麼」屬於 APP；某個 pin／register「怎麼做」屬於 board；
兩者「以什麼順序合作」通常在 platform。不要為了改 pin 而把 register 寫進控制器，也不要把
產品 fault policy 塞進 board driver。

## 開機生命週期

APP 的主要順序由 `main.c` 明確表達：

1. `Reset_Handler` 完成 FPU、copy-down、BSS 與 scatter defaults。
2. `main()` 將 VTOR 設為 `0x00020000`，初始化 clock。
3. 確認 APP boot record，更新 target-specific application identity。
4. 初始化 `g_app`、UART、ADC、power-stage self-test、SPI/DMA position sensor。
5. 載入 motor/output calibration、startup ADC average 與 37-word 產品設定。
6. 建立 runtime controller、sensor 與 MCAN 狀態。
7. 檢查 startup bus voltage，啟動 TMR4 與 ADC IRQ。
8. 主迴圈只服務 deferred event；即時控制由 IRQ 執行。

boot confirmation 只代表 loader 下次可啟動此 APP，不代表馬達已 armed。真正的 motor state、startup
fault、FC 命令與控制 IRQ 是另一條安全路徑。

## 即時與非即時世界

即時世界包含 IRQ0..4，其中 IRQ2 是 20 kHz 控制核心。它們遵守固定 acknowledge 與資料發佈
順序，部分程式與狀態具有固定 SRAM ABI。

非即時世界是 `main()` 的 `service_deferred_events()`：

- 儲存參數與校正資料；
- direction/alignment、output calibration、motor identification；
- firmware-control 匯入／匯出；
- motor state 與 fault indicator 訊息。

新增慢功能時，IRQ 只記錄請求與必要 payload，主迴圈再執行。不要在 ADC／MCAN／UART IRQ
裡等待 Flash、跑長迴圈或大量 `printf`。

## 九型號共用策略

共用 source 不代表九個映像完全相同。差異分成兩個正交維度：

1. **產品 profile**：版本、額定值、pole pairs、gear ratio、控制預設值等，集中在
   `app/include/app_profile.h`。
2. **固定 SRAM layout**：standard、shifted、relocated，由 `config/targets/*.cmake` 同時通知
   compiler 與 linker。

不要在任意 `.c` 中新增 `#if DAMIAO_MODEL_DM4310`。若差異只是數值，新增 profile constant；若是
一項真實能力差異，先定義有產品語意的 capability；只有完整演算法確實分歧時才分成獨立函式。

## Source 與復原證據的關係

```text
factory/reference binary ──► disassembly / differential verifier ──► 行為證據
                                                                  │
maintainable C/ASM source ──► GCC build / ELF / plain / encrypted ─┘
```

產品 build 只依賴 C、ASM、生成表、DDL、profile 與 linker script。任何 `reference/*.bin`、Ghidra
輸出或 factory code byte 都不能被 `.incbin` 或 linker 方式偷偷放回產品映像。普通 Flash 函式也
不要求 byte-to-byte 或位址相同；要求的是外部行為、固定 ABI 與安全時序可驗證。

下一章會說明哪些東西真的必須固定，以及哪些程式可放心擴充。

[下一章：startup 與記憶體](02-startup-memory.md) · [回到目錄](README.md)
