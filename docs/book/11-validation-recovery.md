# 11. 上板驗收與故障復原

離線差分回答「source 在已知條件下是否重現 reference 行為」；上板驗收回答「這張真實 PCB、感測
器、gate driver 與馬達是否安全工作」。兩者都需要，且上板必須由低風險到高風險逐級進行。

## Gate 0：保存與復原準備

- 記錄 board revision、motor serial、target、toolchain與 source commit。
- 備份 boot record `0x1E000`、zero `0x36000`、output records `0x38000/0x3A000`、motor record
  `0x3C000`、configuration `0x3E000`。
- 保存 plain/enc/manifest SHA-256，確認 plain target 沒選錯。
- 準備 SWD／ROM ISP，可在 APP、CAN、UART 都失效時復原。
- 物理隔離母線／gate driver；不要把軟體 disabled 當成唯一安全隔離。

未完成這一級，不進行任何會改 Flash 或驅動功率級的測試。

## Gate 1：離線映像

- `make firmwares` 通過。
- source architecture、九型號 image-layout、startup-copy 與 differential 通過。
- map 顯示 APP、固定 SRAM、`.app_state`、BSS沒有 overflow／overlap。
- 確認要刷的是 `<target>_plain.bin @ 0x20000`，或由 loader 傳正確 frames。

## Gate 2：無功率數位周邊

- VTOR 為 `0x20000`，MSP／Reset vector 正確。
- core 200 MHz、MCAN 80 MHz、UART 921600、TMR4 20 kHz。
- PC13/PH2 LED、PC14/PC15 strap、USART1 TX/RX。
- MCAN loopback、node/0x7FF filters、classic/FD selector、bus-off。
- SPI3 pin、polarity、word、DMA2 complete與 IRQ0/1 cadence。
- ADC trigger、IRQ2、UART timeout IRQ4 的 source、priority、clear/ack。

保存 register dump、logic-analyzer trace與 firmware hash。

## Gate 3：類比與位置，不接功率

- 三相 current raw 的 1,000-sample mean、雜訊與 offset。
- 已知電流／電壓下的 scale與正負方向。
- output U/V mean、sin/cos ellipse與 4096 table validation。
- 手轉正反多圈，同步記錄 SPI raw/corrected/unwrap與 output analog angle。
- reset 後校正、zero與 configuration 仍正確。

若需 provisioning，先 dry-run與 device identity核對；chunk ACK、Flash commit、reset reload 都要留下
證據。

## Gate 4：PWM 波形，無母線或 driver 隔離

- startup/power-stage self-test 不會產生危險 gate 行為。
- neutral compare、六路 pin mapping、polarity、complementary waveform。
- rising/falling dead time、ADC special compare sampling point。
- FC/FD/fault/FB 時 compare、state、LED與回覆順序。
- 任一相位或 high/low 對應錯誤立即停止，不用軟體符號猜實際 gate。

## Gate 5：低壓、限流、空載

1. 限制 DC supply current與 q-current request。
2. 先只測 d/q current loop，確認 electrical angle、相序、Park sign。
3. 正負小命令，記錄 raw current、dq、voltage αβ、PWM compare與 shutdown timing。
4. 再依序啟用 speed、position-speed、MIT、hybrid。
5. 注入 CAN loss、OV/UV、overcurrent、temperature threshold；驗證 debounce、latch、neutral與 clear。

任何異常先 FD／斷電，再分析 trace；不要以提高 limit 或關閉 fault 讓測試「跑完」。

## Gate 6：Commissioning 與帶載

direction/alignment、output scan、motor identification 都會主動驅動馬達，應有獨立風險評估：

- 固定機構或保留安全活動範圍；
- 低壓限流、空載起步；
- 每個 phase 有 timeout、limit與失敗後 disarm；
- 保存 estimator input/output與原版對照；
- 完成後重新檢查 persistent record，而非只看 UART 成功訊息。

最後才逐步增加電壓、扭矩、速度與負載，並涵蓋熱穩態。

## 更新中斷與 brick recovery

模擬在 chunk、erase、program、final marker 前後斷電，確認裝置仍能留在 loader並重送。若 APP 不啟動：

1. 停止功率與重複自動 reset。
2. 讀 VTOR/vector、boot record與 APP Flash頭部。
3. 比對刷入 plain SHA；不要先覆寫 calibration/config sector。
4. 由既有 loader、SWD或 ROM ISP 恢復正確 target plain。
5. 再逐 sector恢復同一裝置的備份資料。

## 驗收紀錄最小欄位

每次測試至少保存：日期、操作者、source commit、ELF/plain SHA、target、板號、馬達號、供電與限流、
接線／負載、命令序列、原始 CAN/UART/logic/scope trace、預期、實際、PASS/FAIL與復原方式。

更精簡的逐項清單見 [PORTING_CHECKLIST.md](../PORTING_CHECKLIST.md)。

[上一章：修改實例](10-change-recipes.md) · [下一章：證據與邊界](12-evidence-status.md)
