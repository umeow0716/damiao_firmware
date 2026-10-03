# 4. 感測器、校正與持久化資料

V3 韌體同時使用 motor-side SPI encoder、output-side 類比 sin/cos sensor、三相電流、bus voltage、
MOS temperature 與 motor temperature。校正資料分成「每次開機重測」與「每台裝置專屬 Flash
record」；同型號不代表可以共用每機校正表。

## 兩套位置來源

### Motor-side encoder

資料路徑：

```text
SPI3/DMA raw word
  → 右移 2 bit，14-bit position
  → 256 點 correction table 線性插值
  → wrap / revolution count / continuous rotor angle
  → 除 gear ratio、扣 motor-output zero
  → output position + 1 kHz velocity
```

主要實作在 `position_sensor.c`，硬體在 `board_position_*`。motor record 有 259 words：前 256 個
是 correction float、word 256 是 electrical offset、word 258 是 direction。載入時各個 NaN correction
獨立正規化為 0；offset NaN 變 0、direction NaN 變 1。這些細節是 factory 行為，不能用「整份
record 全拒收」取代。

### Output-side 類比 sensor

ADC U/V 經 center、gain、phase 四個 calibration float 還原角度，再查 4096-entry `uint16_t`
correction table並 unwrap。主要實作在 `output_sensor.c` 與 `sensor_calibration.c`。

output sensor 用於 startup 對齊、zero、校正與診斷；控制 feedback 的 gear-scaled position／velocity
主要由 motor-side encoder 建立。遇到方向、圈數或零點問題，先辨識是哪一條資料路徑。

## 電流、電壓與溫度

- startup 以 1,000 samples 建立 U/V/W current offset，不應硬編成另一張板子的常數。
- runtime 將 ADC raw 減 offset、乘 target current scale，進 Clarke/Park。
- bus voltage 在 startup 與 runtime 都會換算並參與 over/undervoltage policy。
- MOS/motor temperature 經 `app/generated/temperature_table.c` 查表，其中 motor temperature 另有濾波。

修改 analog scale 必須有量測依據。factory/source 差分只能證明數學與原版一致，不能證明另一批
shunt、放大器、NTC 或 divider 的實際 tolerance。

## 持久化 Flash records

| 位址 | 內容 | 寫入入口 |
|---:|---|---|
| `0x0001E000` | boot record／application identity | `platform_confirm_*`、enter bootloader |
| `0x00036000` | output 與 motor-output zero，各 1 float | CAN FE → deferred Flash |
| `0x00038000` | output sensor calibration，4 floats | output calibration upload/commit |
| `0x0003A000` | 4096 × `uint16_t` output correction | output calibration upload/commit |
| `0x0003C000` | 259-word motor encoder record | motor calibration upload/commit |
| `0x0003E000` | 37-word product configuration | parameter STORE／UART firmware control |

Flash erase 以 sector 為單位；更新 prefix 時必須保存未覆寫的 sector 內容。高層使用
`platform_store_*`，底層由 `board_flash_replace_sector_prefix()` 處理，不要直接 cast Flash address
後寫入。

## 校正資料的生命週期

```text
Flash record
   ↓ startup decode / validation
固定 SRAM staging + typed runtime state
   ↓ IRQ / controller 使用
UART commissioning 或 upload
   ↓ 完成事件
main() 關中斷、寫 Flash、reload、恢復 runtime
```

UART 收到一個 calibration chunk 並回 ACK，不代表整份 record 已安全提交。只有終止 index、deferred
commit、Flash readback，以及 reset 後重新載入都成功，才算完成 provisioning。

## Commissioning

`commissioning.c` 包含三類流程：

1. direction、pole-pair 與 motor encoder alignment；
2. output sensor 4096 點掃描與 calibration；
3. motor identification：RLS 電氣參數、flux observer、sine regression 與機械參數。

數學核心在 `commissioning_math.c`，其中多處刻意保留 binary32／binary64 運算順序與非 fused VFP
語意。這些流程會驅動馬達，不能因 host vector 通過就直接在完整母線執行。

## provisioning 原則

- 每份校正 bundle 綁定正確裝置識別；不要把 factory dump 複製到另一台。
- 先 dry-run、備份原 sector、記錄 plain firmware hash 與供電條件。
- 寫入後 reset，重新讀回並由 startup validation 通過。
- output table 的 `0xFFFF`、相鄰步距過大與 U/V mean fault 要分開診斷。
- zero 命令只改 zero offsets；不應順便重做 encoder correction 或 product configuration。

## 想改感測器時

硬體介面變更從 `board_*` 開始，解碼／濾波從 sensor module 開始，產品比例與限制放 profile/config；
不要把三者混在一個函式。至少準備：raw capture、golden angle/current vector、wrap boundary、NaN／
erased record、正負方向、reset persistence 與低壓實機 trace。

[上一章：板級周邊](03-board-peripherals.md) · [下一章：馬達控制](05-motor-control.md)
