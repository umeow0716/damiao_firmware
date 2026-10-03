# 原始碼與功能地圖

本頁回答兩個問題：「某項功能在哪裡？」以及「從一個入口會走到哪些模組？」若要理解設計
理由，請搭配 [完整韌體手冊](book/README.md)；若要直接修改，請再看
[常見修改實例](book/10-change-recipes.md)。

## 目錄樹

```text
DM4310_firmware/
├── app/
│   ├── include/                 APP 公開型別、profile、模組介面
│   ├── src/                     產品邏輯、控制、協定、校正與 runtime
│   └── generated/               溫度查表（生成檔）
├── board/
│   ├── include/                 板級抽象介面與 register image
│   └── src/                     HC32F448 周邊實作
├── common/
│   ├── include/、src/           共用數學、CRC、AES、boot record
│   └── generated/               2049 點 sine table
├── config/
│   ├── targets/                 九個 target 與 V3 共用 link 選項
│   └── update_profile.json      既有 loader 的加密更新設定
├── startup/                     Cortex-M vector 與 Reset_Handler
├── linker/                      一般 APP 與 V3 固定 SRAM linker script
├── tools/                       封裝、傳送、provisioning、輸出驗證
│   └── recovery/                factory/source 獨立差分；不進產品 build
├── recovered/                   disassembly-derived tables 與復原證據
├── reference/                   原始／解密參考映像；不進產品 build
├── third_party/                 HC32F448 DDL 1.3.0
├── CMakeLists.txt               真正的 source／flags／target 定義
└── Makefile                     日常建置與封裝入口
```

## 由 Reset 到控制迴圈

```text
Reset_Handler                         startup/startup_hc32f448.S
├── SystemInit                        vendor system_hc32f448.c
├── 複製固定 SRAM code/data、清 BSS
├── app_config_initialize_scatter_defaults
├── commissioning_initialize_scatter_defaults
└── main                              app/src/main.c
    ├── platform_early_init           clock
    ├── confirm boot + APP identity   boot record
    ├── app_state_init                g_app / defaults
    ├── platform_prepare_board_startup
    ├── platform_initialize_peripherals
    │   ├── UART、ADC、power-stage self-test
    │   ├── SPI/DMA position sensor
    │   └── calibration + startup ADC averages
    ├── platform_load_parameters
    ├── platform_prepare_runtime_configuration
    ├── platform_initialize_runtime  output sensor、MCAN、startup fault
    ├── platform_start_control_loop  TMR4 + ADC IRQ
    └── service_deferred_events      Flash、commissioning、firmware control
```

## 五個即時 IRQ

```text
IRQ000  TMR4 position event ───────► 啟動 SPI3 位置取樣
IRQ001  DMA2 transfer complete ────► rotor raw/LUT/unwrap
IRQ002  ADC1 EOCA，20 kHz ─────────► ADC → 感測 → fault → FOC → SVPWM
IRQ003  MCAN1 line 1 ──────────────► 命令／參數 → feedback → ack/bus-off
IRQ004  USART1 receive timeout ────► 一個 UART DMA frame → setup console
```

wrapper 與語意 handler 在 `app/src/interrupts.c`；周邊 acknowledge、DMA、FIFO 與 register 操作在
`board/src/`。IRQ 中不可做 Flash erase、長時間 UART 輸出或 commissioning；這些工作只設定
`runtime_status` event，再由 `main()` 執行。

## APP 檔案地圖

| 檔案 | 責任與主要入口 |
|---|---|
| `main.c` | 開機順序、主迴圈、deferred event 消費者 |
| `app_state.c/.h` | `g_app` 的可擴充應用狀態；一般新功能優先放這一層 |
| `app_profile.h` | 九型號 identity、電氣、機械與控制預設值；唯一可出現 `DAMIAO_MODEL_*` 的共用 header |
| `firmware_variant.h` | raw measurement／command response 能力與 UART 狀態 banner |
| `feedback_measurements.h` | 速度／扭矩 reporting source；供 motor feedback 與 0x7FF LIVE 共用 |
| `memory_layout.h` | standard／shifted／relocated SRAM 位址選擇與 ABI assertion |
| `app_config.c` | 37-word persistent 設定的 defaults、encode、decode、staging |
| `motor_control.c` | 四種模式、外迴路、current controller、motion observer、20 kHz fast path |
| `app_commands.c` | FC／FD／FE／FB 與 motor state change 的產品語意 |
| `safety.c` | CAN timeout、過流、欠壓、過壓、MOS／motor 過溫 debounce 與 fault latch |
| `can_protocol.c` | motor command 解碼與 8-byte feedback 編碼 |
| `parameter_protocol.c` | standard ID `0x7FF` 的 read/write/live/store protocol |
| `interrupts.c` | 五個 IRQ 的跨模組編排與必要執行順序 |
| `platform.c` | APP 與 board 的協調層；startup、持久化、感測、PWM、MCAN、UART |
| `position_sensor.c` | motor-side 14-bit SPI encoder、256 點 correction、unwrap、速度 |
| `output_sensor.c` | 類比 sin/cos output encoder、4 參數校正、4096 點 correction |
| `sensor_calibration.c` | record decode、NaN 規則、output table 與 sensor 合法性 |
| `calibration_store.c` | FE 的兩個 zero-offset word encode/decode |
| `calibration_upload.c` | UART motor/output calibration chunk 接收與固定 SRAM staging |
| `commissioning.c` | direction/pole-pair、alignment、output calibration、motor identification 流程 |
| `commissioning_math.c` | RLS、flux observer、sine regression 與特定浮點順序 |
| `firmware_control.c` | UART 匯出／匯入 128-byte configuration block |
| `debug_console.c` | USART DMA frame parser、選單、狀態輸出與內建 formatter |
| `device_auth.c` | MCU UID token 與 OTP slot 比對 |
| `runtime_compat.c` | 原 runtime 相容層、固定 allocator、memory/string helper、AEABI wrapper |
| `sram_runtime.c` | 固定 SRAM IRQ/helper veneer 與少數完整 timing/ABI kernel |
| `softfloat_binary64.c` | formatter 所需 binary64 分類與 extended conversion |
| `extended_arithmetic.c`、`extended_arithmetic_core.S` | 非標準三 word 算術 ABI 的 typed adapter 與核心 |
| `decimal_power.c`、`fixed_decimal.c` | `%f` 十進位縮放與 digit 生成 |
| `motor_lookup_storage.c` | 固定 SRAM 的 2049 點 sine table 儲存擁有者 |

## Board 檔案地圖

`*_config.c` 建立可檢查的 register image；`*_hc32f448.c` 才寫真實暫存器。修改硬體設定時通常
兩邊要一起改。

| 模組 | config／HC32 實作 | 上層用途 |
|---|---|---|
| clock | `board_clock_config.c`／`board_clock_hc32f448.c` | 200 MHz core、Flash wait、PLL、bus clock |
| ADC | `board_adc_config.c`／`board_adc_hc32f448.c` | 三 ADC 同步取樣、startup average、IRQ2 |
| position | `board_position_config.c`／`board_position_hc32f448.c` | SPI3、DMA2、IRQ0/1 |
| PWM/timebase | `board_sampling_timer_config.c`／`board_sampling_timer_hc32f448.c` | TMR4、20 kHz、dead time、SVPWM compare |
| MCAN | `board_mcan_config.c`／`board_mcan_hc32f448.c` | bit timing、message RAM、filter、IRQ3 |
| UART | `board_uart_config.c`／`board_uart_hc32f448.c` | USART1 921600、RX DMA、timeout IRQ4 |
| power stage | `board_power_stage_config.c`／`board_power_stage_hc32f448.c` | 六路 startup self-test |
| Flash | `board_flash_hc32f448.c` | boot/config/calibration sector 擦寫；critical primitive 在 SRAM |
| identity | `board_identity_config.c`／`board_identity_hc32f448.c` | PC14/PC15 hardware variant strap |
| LED | `board_led_hc32f448.c` | PC13 紅、PH2 綠與 fault indication |
| delay／CRC | `board_delay_hc32f448.c`、`board_crc_hc32f448.c` | blocking startup delay、CRC clock |

## Common、生成檔與 third-party

- `common/src/motor_math.c`：Clarke/Park、inverse Park、SVPWM、PI、clamp/wrap、資料量化。
- `common/generated/motor_sine_table.c`：sine table 初值；runtime copy 到固定 SRAM table。
- `common/src/aes256_ctr.c`、`crc8_maxim.c`：更新封裝相容功能。
- `common/src/crc32.c`：一般 CRC32；commissioning 另有其協定指定的 MPEG-2 CRC。
- `common/src/boot_record.c`：APP confirmation、update request 與 identity record 操作。
- `common/src/syscalls.c`：newlib 最小 syscall；`_sbrk()` 失敗，沒有一般 heap。
- `app/generated/temperature_table.c`：ADC temperature conversion table。
- `third_party/HC32F448_DDL_Rev1.3.0/`：vendor DDL/CMSIS。除非確認 vendor bug 或新 peripheral
  需求，不要把產品策略寫進此目錄。

## 功能到檔案的快速索引

| 想修改 | 第一入口 | 通常還要看 |
|---|---|---|
| 型號額定值／預設機械參數 | `app_profile.h` | `config/targets/*.cmake`、`app_config.c` |
| 新增 target | `config/targets/` | `app_profile.h`、CMake、Makefile、layout verifier |
| 控制模式或 gain | `motor_control.c` | `motor_types.h`、`app_config.c`、`parameter_protocol.c` |
| CAN 命令／feedback | `can_protocol.c` | `interrupts.c`、`app_commands.c` |
| measurement／command response 版本 | `firmware_variant.h`、CMake | `can_protocol.c`、`interrupts.c`、Makefile |
| 0x7FF 參數 | `parameter_protocol.c` | `app_config.c`、`platform.c` |
| fault 門檻／debounce | `safety.c` | `motor_types.h`、`app_config.c`、`app_commands.c` |
| PWM 頻率／dead time | `board_sampling_timer_*` | ADC trigger、控制器 `sample_period`、實機驗收 |
| ADC channel／比例 | `board_adc_*` | `platform.c`、`motor_control.c`、temperature table |
| encoder 方向／濾波 | `position_sensor.c`、`output_sensor.c` | calibration records、`platform.c` |
| startup 順序 | `main.c`、`platform.c` | startup ASM、power-stage self-test、boot record |
| UART 選單／命令 | `debug_console.c` | `firmware_control.c`、`calibration_upload.c` |
| Flash record | `app_config.c`／calibration modules | `board_flash_hc32f448.c`、linker/partition map |
| fixed SRAM helper | `sram_runtime.c` | `memory_layout.h`、V3 linker、image-layout verifier |
| 更新加密／檔名 | `tools/pack_update.py` | `config/update_profile.json`、`UPDATE_FORMAT.md` |

詳細改法與驗證步驟見 [第 10 章](book/10-change-recipes.md)。
