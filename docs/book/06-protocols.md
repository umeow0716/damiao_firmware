# 6. CAN、參數與 UART 協定

本韌體有三條外部控制面：節點 CAN 命令／feedback、standard ID `0x7FF` 參數協定，以及 USART1
setup console。它們最後都會碰到同一份 runtime/configuration，修改時必須避免「一條路徑更新、
另一條仍使用舊值」。

## Motor CAN 命令

`can_protocol.c` 依 CAN ID family 與目前 control mode 解碼 setpoint；`app_commands.c` 負責命令的
狀態語意；IRQ003 在處理後立即編碼 8-byte feedback。

四個 8-byte special command 的前七 bytes 都是 `FF`：

| 最後一 byte | 命令 | 行為摘要 |
|---:|---|---|
| `FC` | Enable | fault/status `< 2` 時進 armed/motor mode |
| `FD` | Disable | 離開 motor mode、清 command dynamic state |
| `FE` | Set zero | 記錄兩套 encoder zero，deferred 寫 Flash |
| `FB` | Clear fault | 清 latch/age/report；壞條件仍存在時可再次觸發 |

例如節點 ID 為 `0x001`：

```sh
cansend can0 001#FFFFFFFFFFFFFFFC
cansend can0 001#FFFFFFFFFFFFFFFD
```

special command 是 binary payload，不是 ASCII。節點 ID 與 CAN rate 可能已被 persistent config 修改，
不能一律假設為 factory default。

## Feedback

feedback 由 `can_protocol_encode_feedback*()` 從固定 runtime state 量化，包含 position、velocity、
current/temperature/fault 等 protocol 欄位。方向、gear ratio、量化上下限與 fault 位元會影響封包；
新增欄位不能直接延長既有 8-byte frame，否則舊 master 會誤解。

若要新增 telemetry，優先設計新 CAN ID／版本化 frame，而不是改動既有 feedback 的 byte 定義。

## `0x7FF` 參數協定

主要 operation：

| operation | 值 | 用途 |
|---|---:|---|
| READ | `0x33` | 讀 configuration、calibration、zero 或 live derived value |
| WRITE | `0x55` | 驗證後更新 live 37-word record／runtime coefficient |
| STORE | `0xAA` | 回 ACK，設定 deferred event，由 main 寫 Flash |
| LIVE | `0xCC` | 讀即時 position/current/temperature 等 |

parameter word `0x00..0x24` 對應 37-word configuration，其中部分位址保留或只讀；WRITE validation
集中在 `parameter_protocol.c::write_register()`。READ 另提供 output calibration、zero、direction 與
位置等延伸位址。

協定還保留 discovery、node/master ID、version 與進入 bootloader 的 legacy frame。修改一般參數
dispatch 時不能破壞這些前置分支。

STORE 的 Flash operation 刻意不在 MCAN IRQ 執行：IRQ 先回覆並張貼 event，主迴圈再關閉必要中斷、
寫入 sector、收尾並恢復。新增需要持久化的參數也應沿用此模型。

## UART setup console

UART 使用 binary burst framing，不是以換行結尾的 shell。top-level mode 包含 menu、motor、setup；
ESC 回 menu，`X` 要求進入既有 bootloader。setup mode 的主要命令 family：

- `Uc`：direction/alignment commissioning。
- `Ul`：output sensor calibration。
- `Ud`／`UM`：分 chunk 上傳 motor/output calibration。
- `Ue`：讀 motor parameters或啟動 motor identification。
- `Uf`：讀 device/application identity。
- `Ug`：匯出／匯入 128-byte firmware-control block。
- `FCB?`：修改 CAN data-rate selector，其中 selector 10、11 使用 `:`、`;` 編碼。

精確 length 與 binary payload 以 `debug_console.c::process_frame()` 為準。新增命令時：

1. 先決定它屬於哪個 console mode，避免在 motor mode 誤觸維護命令。
2. 在讀 `data[n]` 前驗證最小長度；RX buffer 上限 200 bytes。
3. IRQ 只驗證、複製有限 payload、回短 ACK、張貼 event。
4. 慢操作放 `main()`，並定義成功、失敗、reset 與重送語意。

## MCAN／UART 共用設定的一致性

CAN node ID、master ID、data-rate selector、control mode 等可由 protocol 修改。更新時要同時考慮：

- fixed staging record；
- typed `MotorConfig` mirror；
- MCAN live filter／classic-FD dispatch；
- dependent controller/filter coefficient；
- 是否立即生效、是否需 STORE、reset 後是否保留。

只改 `g_app.config` 而沒有更新 fixed staging，或只改 staging 沒有 reconfigure peripheral，都會形成
難以重現的「當下正常、reset 後不同」問題。

## 協定擴充原則

- 保持既有 ID、operation、byte order、DLC 與回覆時機相容。
- 新 protocol 先定義版本與 ownership；不要借用「看似沒用」的 retained scratch byte。
- parser 對 malformed input 要有明確界線；若為 factory compatibility 刻意保留怪異 overlap，需在
  註解與差分 test 明確說明。
- CAN IRQ 不做無界 FIFO drain，UART IRQ 不做長計算。
- 增加實例 capture：正常、邊界、非法、重送、bus-off／timeout、reset persistence。

[上一章：馬達控制](05-motor-control.md) · [下一章：更新映像](07-bootloader-update.md)
