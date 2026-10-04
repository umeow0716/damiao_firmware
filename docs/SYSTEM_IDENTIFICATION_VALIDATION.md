# 辨識功能：開發、驗證與下一階段

## 目前交付範圍

分支 `feature/system-identification`。已實作 DM4310 **第一階段：被動同步量測 telemetry v1**。
協定完整定義見 [封包、尺度、時間與狀態契約](SYSTEM_IDENTIFICATION_PROTOCOL.md)。
未修改 `damiao_can`／`damiao_ws`，未刷機、未啟動馬達、未做實機 timing／帶載驗收。
既有四種 variant 的控制與封包不變；辨識能力只編入 opt-in `dm4310_sysid`。

```sh
make dm4310-sysid
```

產物：`build/dm4310_sysid.elf`／`.app.bin`，
`dist/development/sysid/dm4310_plain.bin`、`dm4310_enc.bin`；
manifest 在 `build/package/sysid/dm4310/manifest.json`。
預設 `make` 不加第五種 variant，不執行此處的 tests。沒有新增 send／自動刷機命令。
banner 為 `DMBOT Motor Driver(system identification v1)`；capability 使用
`DAMIAO_SYSTEM_IDENTIFICATION`，基底另啟用 raw＋command feedback disabled。

## 原始碼入口

```text
CAN 6F0 → board RX 第三 exact filter → CAN IRQ request queue
                                            ↓
                                     main identification_service
                                            ↓
ADC/Park → 原控制/outer loop → 原 safety/SVPWM → measurement snapshot
                                                ↓
                                      Iq AA filter + interval average
                                                ↓
                                         16-slot sample queue
                                                ↓
main service → board_mcan_try_send_identification → MCAN TX FIFO → FD64
```

| 檔案 | 職責／修改入口 |
|---|---|
| `app/include/system_identification.h` | capability 的訊號／平台 API 與 CAN ID；不擴大固定 ABI struct |
| `app/src/system_identification.c` | 協定、timebase、command provenance、濾波、request/sample queue、capture watchdog |
| `app/src/identification_platform.c` | PRIMASK critical section、FD readiness 與非阻塞 sender adapter |
| `app/src/motor_control.c` | 記錄 current clamp／voltage saturation、control-consumed sequence，讀取原有 runtime measurements |
| `app/src/position_sensor.c` | DMA publication tick marker；原 encoder／velocity 算法不改 |
| `app/src/interrupts.c` | command write begin/end、辨識 request IRQ 入列、ADC 後置快照 |
| `app/src/main.c` | 每輪有界 TX service |
| `board/src/board_mcan_hc32f448.c` | 第三 RX filter、辨識 request DLC 檢查、實際 FD64 message-RAM 寫入／DLC／TXBAR |
| `CMakeLists.txt`／`Makefile` | DM4310 opt-in target／獨立封裝，不新增正常 make test gate |

private queue/filter state 用一般 `.bss` 配置；不改固定 SRAM 地址、IRQ helper入口、legacy frame
struct或 controller states。APP 使用一般 linker 排列，沒有固定 `.text` alignment trick。
existing frame send 八 byte 限制保留；FD64 使用另外的有界 board sender，而不是只加大 CanFrame。

## 離線驗證命令

不放 `/tmp`，腳本都在 repository；binary／log 在 `build/` 可重建。

```sh
mkdir -p build/tests
cc -std=c11 -Wall -Wextra -Werror -Iapp/include -Icommon/include \
  tools/recovery/test_system_identification.c app/src/identification_scan.c -lm \
  -o build/tests/test_system_identification
build/tests/test_system_identification
.venv/bin/python tools/recovery/verify_identification_protocol.py
.venv/bin/python tools/recovery/verify_identification_transport.py
.venv/bin/python tools/recovery/verify_feedback_variants.py --model dm4310
.venv/bin/python tools/recovery/verify_sram_startup_copies.py \
  --elf build/dm4310_sysid.elf --linker linker/hc32f448_v3_app.ld \
  --startup startup/startup_hc32f448.S \
  --readelf tools/arm-gnu-toolchain/bin/arm-none-eabi-readelf
.venv/bin/python tools/verify_firmware_outputs.py \
  --dist dist/development/sysid --package-root build/package/sysid \
  --expected-variant sysid --build-suffix _sysid --models dm4310
make build
```

`.venv` 的 recovery Python 依赖 Unicorn／pyelftools；不屬 production build dependency。
host tests 用平台 mock，ARM checks 執行實際 source-built ELF，不把原廠 bin link 回程式。

已覆蓋：8／64 byte layout、ACK／INFO、argument/version/op/busy、NaN／Inf、500／1,000 Hz、
command 部分寫入 unknown、非 MIT 只在 outer loop 更新 applied、disabled/fault capture狀態、
sample/request overflow、TX busy 不忙等、sample/tick/queue counter wrap、reserved command seq、
capture heartbeat timeout／renew、configuration change abort、抗混疊 numerical transfer gain。
實際 ARM sender check 逐 byte 核對64 byte payload、DLC15/FDF/BRS、RAM canary、TXBAR，
覆蓋null／length／ID／INIT／classic／bus-off／full／invalid slot拒絕與 PRIMASK 恢復。
原廠 motor watchdog 不由新 heartbeat 更新；需配合原有 safety tests／實機驗收確認。

九型號四 variant 共36 APP 重建，與開發前工作區 APP 的 SHA-256 全部相同。
這證明既有 target 的產物未變，不代表新增 target 已完成實機安全驗證。

## ISR timing 與容量邊界

硬時基 nominal50 µs。新 ADC 路徑增加量測 snapshot、4級IIR、平均與最多一筆64 byte queue
入列；沒有 CAN 傳送、UART、Flash、malloc 或等待。main TX 最多兩次嘗試，sample16／request4
queue 都有固定上限。sample queue payload1024 bytes，request payload32 bytes，另有 filter／
metadata／pending64-byte reply。精確 BSS／APP 大小以當次 ELF／map及本文件末節驗收紀錄為準。

短暫 PRIMASK 區域有：ADC snapshot（阻止 encoder DMA 拆開位置／marker）、main request
處理＋reply build、main sample64-byte copy、board TX header＋最多16個word。都沒有等待迴圈。
這些區域仍會延遲 priority0/1/2 IRQ；不能僅以「非阻塞」推論 jitter=0。
優先序原樣：sampling timer0、encoder DMA1、ADC2、CAN3。

ARM emulator 的 instruction count 只用於確認路徑有界，**不是 MCU cycles，更不是 WCET**。
Flash wait-state、FPU／exception stacking、DMA／AHB contention、其他中斷與 main critical
section 都需要實機測量。以 DWT cycle counter／GPIO量測 ISR整段與各段，再檢查 worst-case
是否小於50 µs並有充分餘裕；若不符合，先停用辨識能力，不能降低原控制器安全頻率硬塞功能。
UART deferred output／參數Flash操作等原有慢路徑可能讓16／32 msqueue滿；counter會揭露但不
保證永遠無drop。單motor FD64 的 payload純資料約256／512 kbit/s，另有仲裁、header、CRC、
bit stuffing與其他7軸control/refresh，必須實測整條bus負載。

## 實機驗收步驟（尚未執行）

1. 經使用者另行授權後，先無功率／輸出 disabled，確認 board與所有bus節點的FD設定與相容性。
2. loopback／分析儀核對6F0 filter、classic ACK8、FD64/BRS/DLC15、payload，確認legacy7FF不受影響。
3. disabled狀態START500／1k、100msheartbeat、STOP／drain／STATUS，核對node／session／sequence／tick。
4. 人為塞滿 TX／降低接收能力，確認忙碌重試、queue drop／sequence gap、bus-off恢復與資料排送。
5. scope／DWT量測ADC、DMA與main critical section的worst-case，尤其emit/samplequeuefull/CAN壓力路徑。
6. 驗證tick實際頻率／回繞解析與encoderDMA marker延遲；确认窗口每20tick一次，無遺漏IRQ。
7. 保護措施、急停與低能量fixture完備後，才驗證current calibration、Iq方向、K尺度、armed/fault。
8. 維持capture heartbeat但中止legacy控制訊息，確認原motorwatchdog仍停機；反向只中止capture
   heartbeat，確認capture停止而不自行改原controller command。重力負載需機械支撑，不能靠零力矩。
9. 用已知訊號核對H(z)／資料年齡，FRF排除warmup、mode切換、fault、限幅、drop；不得預設encoder
   position／velocity與filteredIq同時取樣。STOP不是停機驗證，另測disable／fault輸出處置。

## 第二階段尚未實作的工作

本版只收集外部legacy命令下的資料，**沒有內部激振**。不應讓host以未知operation嘗試啟動。
第一階段build／測試已過，可在後續提交實作下列独立excitation state machine：

- versionedconfigure/start/stop/status與多參數封包／hostseq，非法參數、NaN/Inf、busy、重複start。
- MIT純力矩single sine：固定20k/1ktick計算phase，不累加CAN到達間隔；freq/amplitude/bias、
  ramp／duration、torque/velocity/position行程／temperature限制與取樣可重建的實際命令history。
- 不更動原control gains，不寫Flash，不把辨識參數寫回；驗證其他target隔離。
- ownership／enable/disable/fault/reset／timeout狀態轉移，hostheartbeat不能繞過原motorwatchdog。
- 正常結束的smoothenvelope與明確completion事件；fault立即退出，不等待ramp。
- 正常stop的安全策略需選定並寫清楚：不把torque=0當停止；有重力負載需明確接手／disable／外部支撑。
- received/accepted/applied/completed各自seq／時間、持久可查的status／errorcode。
- standalone浮點邊界、phase／duration回繞、ramp、limits與故障測試，重新量測APP/RAM／ISRtiming。

未實作這些功能是分階段交付的明確邊界，不是隱藏在compiledefine後的「已完成」功能。

## 本次離線驗收紀錄（2026-10-04）

| 項目 | 結果 |
|---|---|
| DM4310 sysid APP plaintext／encrypted 長度 | 56,816 bytes；64 KiB partition 尚餘8,720 bytes |
| 相對DM4310 raw_no_response APP | 54,176 → 56,816，增加2,640 bytes |
| ELF text／data／bss | 56,632／0／22,460 bytes；bss 包含既有固定SRAM sections，不全是新queue |
| 相對raw_no_response BSS | 增加1,304 bytes；辨識state本體1,300 bytes，其餘為兩個saturation flag與padding |
| queue payload | samples16×64=1,024；requests4×8=32；pendingreply64，固定深度 |
| Native C／ASan＋UBSan | 通過封包、狀態、wrap、overflow、浮點邊界、濾波解析模型比對；無sanitizer錯誤 |
| 文件契約 | 完整FD64 fixture逐byte吻合C encoder；文件Python decoder與非法length/version/type測試通過 |
| ARM ELF實際sender | FD64＋classic8通過；10種拒絕case、PRIMASK=0/1恢復、messageRAM canary通過 |
| Fixed SRAM startup | 45個initialized SRAM sections都有startupcopy，linker ABI gates通過 |
| 既有模型回歸 | 全36個APP重建，與修改前SHA-256相同；DM4310四variantfeedback／response／banner測試通過 |
| 封裝 | source APP與dist plain相同；AES-CTR roundtrip、大小、vectors、manifest通過 |
| 實機／WCET／帶載 | **未執行**，仍需前述驗收 |

此snapshot之sysid plaintext SHA-256：
`418d6e123361bbabb362ccd6216b1b01e2cbff54ba120d07fd840922debf5388`。
後續修改以當次map／manifest為準，不將此大小或hash設成正常make的限制。

ARM emulator代表性path instruction counts：inactive22、filterinitialization161、accumulate157、
enqueue含velocityupdate305、queuefull含velocityupdate185；FD64sender173、classic8sender59。
samplecount不包含ADC其餘控制工作、measurement snapshot、command／saturation hooks或exception
entry/exit，不是整個IRQ的worst-case上界；不同flags／state也可有不同路徑。
實機timing仍須同時涵蓋新增及既有路徑，不能將instructions直接換成cycles宣稱通過。

新增單軸掃描、127 筆可靠保留／ACK／補傳及電流 reference cap 後，以上歷史 snapshot 大小與 instruction counts 不代表新版；新版契約見 [OFFSET_SCAN_PROTOCOL.md](OFFSET_SCAN_PROTOCOL.md)。

## Independent guard/live integration — 2026-10-04

Implemented protocol operation 21–28, a 250 ms independent control-setpoint lease, atomic guard staging/commit/readback, all-mode current-reference cap, temperature/state boundary checks and independent 500 Hz newest-only 0x6E0 stream. The opt-in control IRQ follows existing disabled/fault/reset semantics when latched; CLEAR_FAULT invalidates the old setpoint. The matching host requires guard setup on all eight nodes before enable and uses separate recording/live sockets.

All dm4310_sysid/dm4340_sysid/dm8009_sysid targets build. Native tests cover guard commit isolation/rejection, position/velocity/torque/MOS/NaN, current cap, lease boundary/wrap, heartbeat inability to renew a host command, late-command latch, scan lease/completed-home wait, latest overwrite/ownership/expiry. Address/undefined sanitizers pass for telemetry and scan native tests. Existing FD64 C/document fixture remains unchanged. ARM ELF transport tests pass; instruction counts are not hardware WCET. Rebuilding the three motor profiles' four original variants yielded 12/12 identical app binary SHA256 values before/after this extension.

No flashing or real motor/CAN execution was performed. Real stop distance, gravity support, PWM shutdown response, clock accuracy, bus worst case and IRQ WCET remain hardware acceptance tasks. A current-reference cap and sampled-position stop threshold do not guarantee calibrated shaft torque or mechanical stopping before a hard limit. Firmware artifacts are local build outputs; the user flashes them.

Independent guard also rejects stale position/velocity publication (>400 ticks/20 ms), including ARM preflight. Native fault-injection tests cover reason 8; host live tests distinguish a fresh packet from its stale encoder publication marker.

Makefile now builds/packages dm4310-sysid, dm4340-sysid and dm8009-sysid into dist/development/sysid. All three fresh packages passed verify_firmware_outputs.py source/size/AES round-trip/manifest checks. Packaging generates files only; no CAN update or flashing was executed.

Applied VEL extension: native tests cover version 2 field36, stable/mixed-reference intervals at both rates and live reference. verify_identification_protocol checks the documented decoder against MIT and VEL fixtures. No hardware CAN or motor operation was performed.
