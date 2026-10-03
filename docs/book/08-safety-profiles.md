# 8. 安全、fault 與 target profile

九個 target 共用程式，但電氣尺度、機械常數、控制預設值與固定 SRAM layout 不同。安全策略必須
同時尊重 profile、persistent configuration、runtime measurement 與真實硬體能力。

## Target 矩陣

下表是 source profile 的辨識用摘要，不是產品 datasheet 額定保證：

| target | 版本 | SRAM layout | current full scale (A) | pole pairs | gear ratio | velocity max | torque max | bus OV default |
|---|---:|---|---:|---:|---:|---:|---:|---:|
| `dm10010` | 5617.04 | shifted | 99.7388 | 21 | 10 | 20 | 200 | 54 V |
| `dm3507` | 5717.04 | standard | 10.2612 | 14 | 7 | 50 | 5 | 32 V |
| `dm3507_48v` | 6517.04 | shifted | 10.2612 | 14 | 7 | 50 | 5 | 60 V |
| `dm4310` | 5017.04 | standard | 10.2612 | 14 | 10 | 30 | 10 | 32 V |
| `dm4310_48v` | 6017.04 | shifted | 10.2612 | 14 | 10 | 50 | 10 | 60 V |
| `dm4340` | 5117.04 | standard | 10.2612 | 14 | 40 | 10 | 28 | 32 V |
| `dm4340_48v` | 6117.04 | shifted | 10.2612 | 14 | 40 | 20 | 28 | 60 V |
| `dm8006` | 6317.04 | relocated | 41.0448 | 21 | 6 | 45 | 20 | 54 V |
| `dm8009` | 6417.04 | relocated | 41.0448 | 21 | 9 | 45 | 54 | 54 V |

完整常數與 exact float bits 在 `app/include/app_profile.h`。修改時保留 decimal/hex-float 與 `_BITS`
對應，避免 compiler rounding 或比較 threshold 無意改變。

## Profile ownership

每個 `config/targets/<target>.cmake` 只選：

- 一個 `DAMIAO_MODEL_*` identity；
- 必要的 shifted／relocated layout；
- V3 共用 linker 與 AEABI wrapper。

產品數值集中於 `app_profile.h`。若新增欄位：先定義它是 factory default、硬體能力上限、runtime
derived constant，還是使用者可持久化設定；四者不可用同一個 macro 含糊帶過。

## Fault code

| code | 名稱 | 來源 |
|---:|---|---|
| 0 | none | 正常未 Enable |
| 1 | enabled status | motor state 已 Enable |
| 2 | output calibration missing | 4096 table 有 erased entry等 |
| 3 | output calibration invalid | 相鄰 correction step 異常 |
| 4 | output sensor | startup U/V mean 異常 |
| 8 | bus overvoltage | runtime debounce |
| 9 | bus undervoltage | runtime debounce |
| 10 | overcurrent | runtime debounce |
| 11 | MOS overtemperature | runtime debounce |
| 12 | motor overtemperature | runtime debounce |
| 13 | communication lost | armed 時 command age 超時 |
| 14 | overload | 保留的產品 fault |

startup fault 2–4 與 runtime fault 8–14 的來源不同；不要把 calibration 問題誤診為 current loop fault。

## Debounce 與 latch

`safety_update()` 每個 20 kHz control tick 執行。原行為在 bad sample 計數「超過」threshold 時觸發：

| 條件 | threshold | 20 kHz 下約略時間 |
|---|---:|---:|
| motor/MOS overtemperature | 8000 | 400.05 ms |
| overcurrent | 5000 | 250.05 ms |
| undervoltage | 5000 | 250.05 ms |
| overvoltage | 20000 | 1000.05 ms |

若改控制頻率，tick threshold 的實際秒數會一起改。Clear Fault 會清 latch、communication age 與回報
fault，但保留部分 analog debounce counter；壞條件仍存在時可能下一 tick 再次觸發，這是預期行為。

communication age 只在 motor enabled 時增加，收到正確節點 ID 的 control frame 會依原語意重置，
即使 command family 最後只回 feedback。不要把「合法 setpoint」才 reset timeout 當成理所當然。

## 48 V 不只是改一個電壓值

48 V target 使用 shifted SRAM layout，且 profile 的 startup/runtime overvoltage、機械預設值或速度上限
可能不同。不要從 24 V target 複製 binary 或只改 `BUS_OVERVOLTAGE`；至少核對：

- current/voltage scale 與 ADC divider；
- startup bus limit 與 parameter WRITE validation；
- PWM／gate driver hardware revision；
- motor R/L/flux、pole pairs、gear ratio與速度／扭矩 limit；
- calibration record 與 application identity。

## 新增 target 的完成條件

1. 有可追溯的 reference firmware與解密 APP。
2. 新增獨立 target profile與唯一 model identity。
3. 判定固定 SRAM 屬於三種既有 layout 或建立有證據的新 layout。
4. 補齊 `app_profile.h` 常數，不以另一型號 macro 冒充。
5. CMake／Makefile／architecture verifier／image-layout verifier認得新 target。
6. 完成 build、plain/enc、AES round-trip、映像 layout、startup copy 與 factory/source 差分。
7. 依硬體 revision 重做無功率與低壓限流驗收。

[上一章：更新映像](07-bootloader-update.md) · [下一章：建置與驗證](09-build-test-debug.md)
