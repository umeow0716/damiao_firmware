# 達妙 V3 馬達韌體

本專案是 HC32F448 平台的達妙 V3 單馬達控制器 APP 原始碼。九個型號共用同一套可維護的
C／ASM 實作；型號 profile 只負責產品識別、電氣與控制常數，以及三種固定 SRAM 配置。

支援的 target：

- `dm10010`
- `dm3507`、`dm3507_48v`
- `dm4310`、`dm4310_48v`
- `dm4340`、`dm4340_48v`
- `dm8006`、`dm8009`

以上皆為 V3、sub-version 04。這是 **APP-only** 工作區：APP 從 Flash `0x00020000`
執行，既有 bootloader 不在本專案內建置。

## 快速開始

建置全部型號並產生明文與加密映像：

```sh
make
```

只建置一個型號：

```sh
make dm4310
```

產物位於 `dist/development/`：

```text
<model>_plain.bin   debugger／programmer 直接寫入 0x00020000
<model>_enc.bin     交給裝置既有 bootloader 的加密更新 payload
```

請勿把 `_enc.bin` 直接寫進 APP Flash，也不要把某一型號的映像刷到另一型號。

## 文件

- [文件總覽](docs/README.md)：依「第一次閱讀、修改功能、上板驗收、追查復原證據」分流。
- [完整韌體手冊](docs/book/README.md)：架構、開機、控制、通訊、校正、限制與修改實例。
- [原始碼地圖](docs/SOURCE_MAP.md)：目錄、檔案、主要入口函式與功能對照。
- [開發與建置](docs/DEVELOPMENT.md)：日常流程、產物、驗證層級與提交前檢查。
- [上板驗收清單](docs/PORTING_CHECKLIST.md)：由無功率到低壓限流的分級驗收。
- [更新格式](docs/UPDATE_FORMAT.md)：plain／encrypted 映像與既有 loader 的邊界。
- [復原完成度](docs/FIRMWARE_RECOVERY_PROGRESS.md)：九型號差分、映像與大小證據。

## 專案分層

```text
CAN／UART 命令與設定
        │
        ▼
app/    產品語意、控制器、安全、校正、commissioning
        │
        ▼
platform.c    APP 與硬體之間的協調層
        │
        ▼
board/  HC32F448 clock、ADC、SPI/DMA、TMR4、MCAN、UART、Flash
        │
        ▼
HC32F448 + inverter + sensors
```

其他重要目錄：

- `common/`：控制數學、CRC、AES、boot record 等共用元件。
- `config/targets/`：九個 target profile；不要在共用程式散落型號判斷。
- `startup/`、`linker/`：vector、copy-down、固定 SRAM ABI 與 64 KiB APP 邊界。
- `tools/`：封裝、傳輸、校正 provisioning 與開發工具。
- `reference/`、`recovered/`、`tools/recovery/`：復原證據與獨立差分工具，**不會連結進產品映像**。

## 最重要的開發原則

1. 功能差異放在 `app_profile.h` 或 target profile；共用模組依能力或 layout 選擇，不依另一個
   型號的名稱猜行為。
2. 一般函式可以自然增長與重排，不應加入 factory 位址 padding 或固定函式順序。
3. 固定 SRAM IRQ、helper、狀態結構與 copy-down 表是 ABI；修改前先讀
   [startup 與記憶體章](docs/book/02-startup-memory.md)。
4. 20 kHz IRQ 內禁止阻塞、配置記憶體、格式化輸出或擦寫 Flash；慢工作透過 deferred event
   交回 `main()`。
5. 正常 `make` 不編入測試碼。復原差分與測試留在 `tools/recovery/`，另行執行。
6. `-Os`、hard-float 與 `-ffp-contract=off` 是目前驗證過的基線；改最佳化或浮點順序後必須重做
   差分與實機 timing 驗收。

目前各 target 的 plain 映像約 53.8 KiB，64 KiB APP partition 尚餘約 11.7 KiB；此餘裕不是
跳過 map、stack、IRQ latency 與硬體安全驗證的理由。
