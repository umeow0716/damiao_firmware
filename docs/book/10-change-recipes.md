# 10. 常見修改實例

本章不是可直接複製的 patch，而是告訴你每類需求的正確 ownership、最小修改面與驗收項目。

## 例 1：調整某型號的預設速度、扭矩或馬達參數

修改入口：`app/include/app_profile.h` 對應 model block。

1. 更新 semantic float 與必要的 `_BITS` 常數。
2. 檢查 `app_config_load_defaults()` 是否引用該欄位。
3. 判斷已燒錄裝置的 `0x3E000` persistent record 是否會覆蓋新 default；若會，規劃 migration／erase，
   不要誤以為重刷 APP 就會套用。
4. 建該 target及相近 target，確認沒有把共用值誤改。
5. 做 profile差分與低壓 command limit 驗收。

不要把一個 target 的數值寫成散落在 `motor_control.c` 的 `#if MODEL`。

## 例 2：新增一個 motor target

修改入口：`config/targets/`、`app_profile.h`、`CMakeLists.txt`、`Makefile`。

1. 建立 `<target>.cmake`，只定義唯一 model identity 與 SRAM layout。
2. 在 `app_profile.h` 補齊所有 profile macro。
3. 加入 CMake／Makefile target list。
4. 更新 `verify_source_architecture.py`、image-layout model table、reference mapping。
5. 產生解密 reference、完成全函式掃描與差分，不借用另一 target 的 factory 結論。
6. 更新本手冊 target 表，建置 plain/enc，完成上板驗收。

若新 reference 不符合三種既有 SRAM layout，先建立第四種有證據的 layout；不要在
`MEMORY_LAYOUT_ADDRESS()` 以型號名稱堆特例。

## 例 3：新增 CAN command

修改入口：`can_protocol.h/.c` → `app_commands.c` → `interrupts.c`。

1. 先分配不衝突的 CAN ID family／payload/version。
2. decoder 只負責 framing、range 與 typed command。
3. command handler 定義 state transition、安全條件與 feedback。
4. 若工作會阻塞，新增有界 payload + deferred event，不在 IRQ 直接做。
5. 測合法、非法 DLC、錯 ID、fault/disabled state、重送與 feedback byte。
6. 上板抓 MCAN FIFO/ACK/bus-off trace。

若只是 telemetry，優先新增版本化 ID，不改既有 8-byte feedback layout。

## 例 4：新增或修改 `0x7FF` parameter

修改入口：`app_config.*`、`parameter_protocol.c`，必要時 `platform.c`。

1. 決定它是 persistent、live、derived 或 read-only。
2. 若擴充 37-word record，必須先確認 on-flash compatibility、staging 固定 ABI 與 sector schema；這
   不是單純增加 struct field。
3. 在 READ/WRITE/LIVE/STORE 中只加入必要分支，定義 normalization 與拒絕回覆。
4. live peripheral 變更要同步 reconfigure；persistent 變更由 main deferred store。
5. 測 write-but-not-store、store+reset、非法 float bits、舊 record migration。

若只需要新開發功能且不必被舊工具看見，建立版本化擴充 record／新 command 通常比破壞 37-word
ABI 更乾淨。

## 例 5：修改 PID、observer 或控制模式

修改入口：`motor_control.c`、`motor_types.h`、`commissioning.c`。

1. 先確認是 outer loop、current loop、observer 還是 coefficient derivation。
2. 可調值進 `MotorConfig`／profile；每 tick state 放合適 runtime owner。
3. 不在 20 kHz 路徑重算固定係數、使用 printf、Flash、malloc或無界 loop。
4. 保留 disabled/fault reset、limit、anti-windup與 mode-switch 行為。
5. 做長序列數值測試與 IRQ cycle/timing量測。
6. 低壓限流依序驗證 d/q current，再開 speed/position/MIT/hybrid。

若要增加較大的控制器 state，優先由普通 C state 持有並透過窄介面傳入；不要直接擴充固定 19-float
`CurrentController`。

## 例 6：改 PWM 頻率或 dead time

修改入口：`board_sampling_timer_config.c`，但這是跨系統修改。

同步檢查：

- timer period、neutral/special compare、dead-time ticks；
- ADC trigger 與 IRQ deadline；
- controller sample period與 1 kHz divider；
- safety debounce 的實際時間；
- position velocity decimation；
- commissioning step count/frequency；
- CAN status event rate與 CPU load。

驗收需要示波器與低壓限流，不可只靠 register image。

## 例 7：換 ADC channel、電流放大器或 voltage divider

修改入口：`board_adc_config.c`／`board_adc_hc32f448.c`、`platform.c`、profile scale。

1. 更新 pin analog mode、channel mux與 raw index。
2. 用已知電流／電壓重新取得 slope、offset、方向與 tolerance。
3. 檢查 startup average、current clamp、overcurrent與 bus OV/UV threshold。
4. 確認三相順序與 Park sign。
5. 保存 raw ADC + 外部量測，不從另一硬體 revision 猜 scale。

## 例 8：改 encoder 方向、濾波或換 sensor

修改入口：硬體在 `board_position_*`，演算法在 `position_sensor.c`／`output_sensor.c`，產品值在
profile/config/calibration record。

方向錯誤先分辨：SPI raw direction、motor electrical direction、gear/output direction、output analog
sensor phase。不要一次把多條路徑都乘 `-1`。

測試要跨 0/2π、正反多圈、速度正負、靜止抖動、missing/NaN record，最後再低壓 commutation。

## 例 9：新增 startup 自檢或 fault

修改入口：startup self-test 在 `platform.c`／`board_power_stage_*`；runtime fault 在 `safety.c`。

1. 定義 fault code、觸發來源、debounce、latch、clear 與 feedback。
2. startup 阻塞診斷仍要保留維護通訊與安全輸出。
3. runtime fault 不能在 IRQ 做慢診斷。
4. 新 tick threshold 要換算真實時間，並考慮 control frequency 變更。
5. 測 fault active、短暫 glitch、clear while bad、clear after recovery、reset。

## 例 10：新增 UART 維護命令

修改入口：`debug_console.c::process_frame()`。

1. 分配 mode、command/subcommand、exact length 與 response。
2. 先驗長度再讀 payload；限制在 200-byte RX DMA capacity。
3. IRQ 只複製有界資料與張貼 `AppEvents/RuntimeStatus`。
4. main 執行慢工作，完成後回 menu或回應明確狀態。
5. 測 burst 分段、timeout、malformed length、ESC、重送與同時 CAN traffic。

## 例 11：新增一般 runtime state

優先順序：

1. 僅 main 使用：普通 static／`g_app`。
2. main 與 IRQ 共用：`g_app` 或一般 BSS 的明確 owner，加 critical section／原子語意。
3. 只有在固定 ASM/IRQ 必須以 offset 讀取時，才新增 fixed ABI 欄位。

完成後看 `__app_state_free_bytes__`／`__source_bss_free_bytes__` 與 stack。不要為一般 state 改
linker 固定位址或移除 ABI assertion。

## 例 12：修改固定 SRAM helper

1. 從 `sram_runtime.c` 找 section 名稱。
2. 從 linker 查該 layout 的地址、大小上限與相鄰 section。
3. 若是 veneer，修改被呼叫的普通 C implementation。
4. 若是完整 kernel，只做必要變更，保留 register/FPSCR/MMIO contract。
5. 建 standard、shifted、relocated，跑 startup copy、image layout 與精確差分。

不要以移除 ASSERT、縮短 padding 或改下一個 section 地址來掩蓋 overflow。

## 例 13：減少韌體大小

先用 `nm --size-sort` 與 map 找真實大項，再考慮：共用重複常數、刪除不可達路徑、縮短格式字串、
讓普通函式可被 GC。不要犧牲：浮點順序、fixed kernel、校正表、fault、安全診斷或可讀的模組邊界。

比較 `-Oz` 可用獨立 build directory，不直接改正式旗標。尺寸改善只有在九型號差分與 timing 通過後
才可採用。

[上一章：建置與驗證](09-build-test-debug.md) · [下一章：上板驗收](11-validation-recovery.md)
