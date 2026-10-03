# 3. 板級周邊

Board layer 將「register image」與「實際寫硬體」拆開。`board_*_config.c` 描述應有設定，
`board_*_hc32f448.c` 執行 HC32F448 register／DDL 操作。這個切分讓設定可被離線檢查，也避免控制
策略直接依賴 vendor register。

## 周邊總覽

```text
8 MHz XTAL → PLL 800 MHz
             ├─ core/HCLK/PCLK0/PCLK4 200 MHz
             ├─ PCLK1 100 MHz
             ├─ PCLK2/PCLK3 50 MHz
             └─ PLLQ / MCAN 80 MHz

TMR4 20 kHz ─┬─ complementary PWM + dead time → 六路 bridge input
             └─ special compare → ADC1 trigger
                                  └─ ADC1/2/3 synchronous sample → IRQ002

TMR4 event → IRQ000 → SPI3 transaction → DMA2 complete → IRQ001
MCAN1 FIFO/error → IRQ003
USART1 RX DMA + receiver timeout → IRQ004
```

## Clock

`board_clock_config.c` 描述 8 MHz crystal、PLL 與 bus divider；`board_clock_hc32f448.c` 負責 protected
register、Flash wait state、SRAM wait、cache/prefetch 與 ready flag。

clock 是所有 timing 的共同根因。UART baud、CAN bit timing、TMR4 週期同時異常時，先查 clock，
不要分別修改三套常數掩蓋問題。

## ADC 與 20 kHz trigger

三顆 ADC 以同步模式取樣，TMR4 special compare 經 AOS 觸發 ADC1，ADC1 EOCA 送到 IRQ002。
`board_adc_build_config()` 中的 channel mux 有一項 target capability：部分型號使用 PA1/PA3 layout，
由 `APP_PROFILE_ADC_PA1_PA3_LAYOUT` 選擇。

開機包含兩階段平均：

- 三相 current offset 與 bus voltage 的 1,000-sample average；
- output analog sensor U/V 的 1,000-sample average。

runtime control IRQ 直接讀固定 SRAM raw sample。更改 channel、取樣時間、觸發點或比例時，需同步
檢查 `platform.c` 與 `motor_control.c` 對 raw index 的解讀。

## SPI3、DMA2 與 motor-side encoder

IRQ000 只開始一次 SPI transaction，DMA2 收完資料後由 IRQ001 更新 position state。這使 ADC/FOC
不需要 busy-wait SPI。設定包含 pin mux、SPI mode、DMA trigger `371`、DMA IRQ source `65`，以及
11 個 encoder register/value。

若換 sensor 或 PCB：先修改 `board_position_config.c` 的 register image，再修改 HC32 writer；接著
用 logic analyzer 驗證 SCK polarity、word width、chip-select、sample cadence 與 DMA complete 順序。

## TMR4、PWM 與 sampling timebase

目前 register image 的關鍵值：period 5000、neutral compare 2500、rising/falling dead time 80 ticks、
special compare 120。timer 同時負責六路 complementary PWM、ADC 取樣點與 position trigger 節奏。

修改 PWM 頻率不是只改 `period`：還會影響 controller sample period、fault debounce 的實際時間、
position 速度 decimation、ADC 取樣點、dead-time 實際時間與 commissioning 迴圈。應視為跨模組變更，
詳見 [第 10 章](10-change-recipes.md)。

disabled/fault 主要由 neutral compare 與 controller state 實作；FC 並不是另一套 pin-mux gate。第一次
上板必須物理隔離 driver／母線，不能假設「尚未 Enable」就代表所有功率腳沒有波形。

## MCAN1

MCAN 使用 PLLQ 80 MHz。PB6 為 RX、PB7 為 TX；message RAM、兩個 standard filter、FIFO 與 Tx
queue 由 `board_mcan_*` 建立。第一個 filter 接收節點命令 family，第二個接收 `0x7FF` parameter
traffic。

data-rate selector `0..4` 使用 classic CAN 路徑，`5..11` 啟用 CAN FD；arbitration timing 與 data
timing 的對照集中在 `board_mcan_config.c`。selector 10、11 不是相鄰值內插，請勿自行簡化表格。

IRQ003 每次處理一個 FIFO0 element，再由硬體重觸發後續訊息；tail 還需依序處理 bus-off、error、
IR 與 NVIC ack。不要把 handler 改成無界 drain loop。

## UART

USART1 位於 PA11/PA12，921600 8N1。APP 使用 RX DMA（200-byte buffer）和 TMR0 receiver timeout，
IRQ004 將一個 burst 交給 `debug_console_process_dma_frame()`，再 rearm DMA。

這不是一般 line-oriented shell：frame length、binary payload 與 timeout 都是協定的一部分。新增文字
命令時仍要防止超長 frame、未終止字串與在 IRQ 中執行慢工作。

## Power-stage self-test、identity 與 LED

- `board_power_stage_*` 於 startup 逐路切換六個 bridge input latch 並讀 ADC 判斷 failure bitmap。
- `board_identity_*` 讀 PC14/PC15 strap，提供 hardware variant。
- `board_led_*` 管理 PC13 紅燈、PH2 綠燈與 fault indicator。

power-stage self-test 失敗會進阻塞診斷迴圈。修改它時必須保留「測試前功率隔離、失敗不進 control
loop」的安全性；host register image 無法替代真實 gate driver 驗證。

## Flash

`board_flash_hc32f448.c` 擁有 boot record、zero、calibration、configuration sector 的寫入。真正
erase/program primitive 放在 SRAM；操作期間中斷與取指條件非常敏感。高層應呼叫 platform/store
介面，不要在協定 parser 直接寫 EFM register。

## 修改 board layer 的最低驗證

1. config 與 HC32 writer 同步修改。
2. 建置所有受影響 target，跑 image-layout／差分。
3. 無功率量 clock、pin mux、IRQ source、clear/ack 與波形。
4. 涉及 ADC/PWM/功率級時，再做低壓限流與 fault 注入。
5. 保存 target、板號、映像 hash、量測條件與原始 trace。

[上一章：startup 與記憶體](02-startup-memory.md) · [下一章：感測器與校正](04-sensors-calibration.md)
