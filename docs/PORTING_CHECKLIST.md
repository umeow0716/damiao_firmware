# 上板與移植驗收清單

完整理由見 [手冊第 11 章](book/11-validation-recovery.md)。每一級未完成前，不進入下一級。

## 0. 備份與復原

- [ ] 記錄 source commit、target、board revision、motor serial、toolchain。
- [ ] 保存 ELF/plain/enc/manifest SHA-256。
- [ ] 備份 `0x1E000`、`0x36000`、`0x38000`、`0x3A000`、`0x3C000`、`0x3E000`。
- [ ] 確認 SWD／ROM ISP／既有 loader 至少一條可用。
- [ ] 母線與 gate driver物理隔離，限流電源與急停可用。
- [ ] 確認 target plain沒有選錯；programmer只刷 plain @ `0x20000`。

## 1. 離線

- [ ] `make firmwares`。
- [ ] `verify_source_architecture.py`。
- [ ] 九 target image-layout與 startup-copy。
- [ ] 受影響 target full differential。
- [ ] APP Flash、fixed SRAM、`.app_state`、BSS、stack無 overflow／overlap。
- [ ] plain/enc、manifest、AES round-trip與 target identity一致。

## 2. 無功率數位驗證

- [ ] VTOR `0x20000`，vector/MSP/Reset Handler正確。
- [ ] core 200 MHz、MCAN 80 MHz、UART 921600、TMR4 20 kHz。
- [ ] LED與 hardware strap。
- [ ] UART TX/RX DMA + timeout IRQ4。
- [ ] MCAN loopback、filter、classic/FD、IRQ3、bus-off。
- [ ] SPI3/DMA2、IRQ0/1、encoder raw cadence。
- [ ] ADC trigger與 IRQ2 clear/ack。

## 3. 類比與感測

- [ ] 三相 current 1,000-sample offset與noise。
- [ ] 已知 current、bus voltage、temperature scale。
- [ ] output U/V mean與sin/cos ellipse。
- [ ] 正反手轉多圈，兩套 encoder raw/correction/unwrap/velocity。
- [ ] calibration/zero/config reset後可正確 reload。

## 4. 無母線 PWM

- [ ] startup self-test六路 mapping與 failure bitmap。
- [ ] neutral compare、三相 high/low實際 pin對應。
- [ ] complementary polarity、80-tick dead time、ADC sampling point。
- [ ] FC/FD/fault/FB 的 compare、LED、feedback時序。
- [ ] 發現相序／high-low錯誤立即停止並修 source。

## 5. 低壓限流

- [ ] 小 q-current 驗證 electrical angle、Park sign、相序。
- [ ] d/q current loop正負命令與飽和。
- [ ] speed、position-speed、MIT、hybrid逐一開啟。
- [ ] CAN loss、OV/UV、overcurrent、temperature fault注入。
- [ ] fault debounce、neutral、latch、FB clear與仍壞時 retrip。
- [ ] 保存 ADC/dq/αβ/PWM/gate/communication原始 trace。

## 6. Commissioning 與帶載

- [ ] direction/pole-pair/alignment在低壓限流下完成。
- [ ] output calibration scan有機械空間與timeout。
- [ ] motor identification每階段有limit、timeout、failure disarm。
- [ ] Flash commit後reset/readback。
- [ ] 逐步增加 voltage/torque/speed/load，完成熱穩態與 shutdown。

## 驗收紀錄

- [ ] 每次記錄日期、操作者、hash、target、板／馬達、供電、接線、負載、命令。
- [ ] 附 CAN/UART/register/logic/scope原始資料，不只寫「正常」。
- [ ] FAIL 保留症狀、復原步驟與未解風險。
- [ ] 所有 intentional deviation有產品決策與文件。
