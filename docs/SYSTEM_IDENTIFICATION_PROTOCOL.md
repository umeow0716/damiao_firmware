# 單馬達辨識量測協定 v1

本文件是 `damiao_can`／`damiao_ws` 的接手契約，描述目前已實作的**第一階段被動量測**。
對應 target：`dm4310_sysid`、`dm4340_sysid`、`dm8009_sysid`；既有四種 variant 不提供此能力。最新版 guard／live 擴充見第 10 節。
第二階段內部正弦激振尚未實作，不能用本文件的 START 啟動馬達或激振。
建置、證據、限制及第二階段工作清單見 [開發與驗收](SYSTEM_IDENTIFICATION_VALIDATION.md)。

## 1. 傳輸與相容性

| 用途 | CAN ID | 格式／長度 | 備註 |
|---|---|---|---|
| 辨識管理 request | `0x6F0` | standard，data frame，8 bytes | classic 或 FD8；不接受 RTR／extended |
| ACK／管理回覆 | `0x6F1` | ACK 為 classic8；INFO／STATUS 為 FD64+BRS | management reply 優先於 sample |
| telemetry sample | `0x6F2` | standard，FD64+BRS，DLC=15 | 每筆完整一幀，沒有分片 |

低 byte `F0/F1/F2` 不等於支援的 node ID 1–8，不會進入既有節點控制分支。
第三個 standard exact filter 收 `0x6F0`；原本 node mask 與 `0x7FF` filter 保留。
不改既有八 byte command、feedback、LIVE、參數、更新／bootloader 格式。
本 APP 不新增 loader 操作，這些 ID 只在 APP 執行時提供服務。

同一 bus **只能有一顆辨識 target 啟用 streaming**。回覆 ID 沒有依 node 分配，不支援多軸
同步辨識；多顆同 ID、不同資料同時傳送會發生 CAN data-phase conflict，不是可接受的排隊方式。
一般其餘軸仍依原有協定控制。capture 本身不管理其他軸，也不保證 hold position。

START 必須在 MCAN 已具備 FDOE、BRSE、非 INIT、非 bus-off 的狀態下執行，否則回錯誤 4。
本功能不自動切换 data rate、不自動寫 Flash。出廠 selector 4 是 classic CAN，不能直接 streaming。
需在安全、停止輸出的條件下按既有流程設定／重新初始化 CAN FD，並確認**整條 bus 與所有節點**
可容忍 FD。selector 9 的既有設定為 nominal 1 Mbit/s、data 5 Mbit/s；詳見
[板級周邊文件](book/03-board-peripherals.md)。只修改 FCB dispatch 指標不等於完成硬體重新初始化。

以下所有多 byte 欄位均為 **little-endian**；`f32` 是 IEEE-754 binary32。禁止用 native struct
padding 解包。下文 offset 從 payload 第一 byte 起算，不含 SocketCAN header。

## 2. 管理 request：8 bytes

| offset | 大小 | 型別 | 定義 |
|---:|---:|---|---|
| 0 | 1 | u8 | protocol version，目前為 1 |
| 1 | 1 | u8 | 目標 node，限定 1–8；不是自己 node 時不回覆 |
| 2 | 1 | u8 | operation，見下表 |
| 3 | 1 | u8 | argument；START 用，其他已知 operation 必須為 0 |
| 4 | 4 | u32 | host request sequence，回覆原樣 echo；START 成功後也是 session ID |

| operation | 名稱 | argument | 行為 |
|---:|---|---|---|
| 1 | CAPTURE_START | 0=500 Hz；1=1,000 Hz | 清除上一 session queue、sample sequence／drop count，重置量測濾波，開始被動 capture |
| 2 | CAPTURE_STOP | 0 | 立即停止**產生新樣本**，已入 queue 的資料繼續排送；重複 STOP 成功 |
| 3 | CAPTURE_HEARTBEAT | 0 | 更新量測存活時間；不代替原廠 control watchdog，不使能馬達 |
| 4 | INFO | 0 | 回 64-byte 設定／狀態快照 |
| 5 | STATUS | 0 | 回相同 layout 的 64-byte 狀態快照 |

其他 operation 回 unsupported，尚無 excitation configure/start/stop 操作。
request sequence 不作排序或去重：START active 時一律 busy，重複 STOP／HEARTBEAT 沒有副作用。
重送 START 前先 STATUS；不要把丟失 ACK 視為 START 未接受。
接收 queue 深度 4，滿時丟棄新 request，`request_drop_count` 遞增；該 request 沒有 ACK。
host 必須用 timeout＋STATUS 分辨丟 request／ACK 與裝置忙碌。

## 3. ACK：8 bytes，ID 0x6F1

| offset | 大小 | 型別 | 定義 |
|---:|---:|---|---|
| 0 | 1 | u8 | 回覆端版本 1（收到未知版本也回 1） |
| 1 | 1 | u8 | message type `0x80` |
| 2 | 1 | u8 | 原 request operation |
| 3 | 1 | u8 | result |
| 4 | 4 | u32 | 原 request sequence |

| result | 意義 |
|---:|---|
| 0 | accepted／操作成功 |
| 1 | protocol version 不支援 |
| 2 | argument 不合法 |
| 3 | busy：capture 已 active，不能重複 START |
| 4 | FD transport 尚未 ready |
| 5 | operation 不支援 |
| 6 | 尚無有效正數 torque constant／gear ratio |

成功 INFO／STATUS 直接回 FD64，不再另外回 ACK8；錯誤仍回 ACK8。
ACK=accepted 表示主迴圈已完成管理操作，不是僅 CAN IRQ 收到，也不是馬達已使能／力矩已建立。
STOP ACK 不是「馬達已停止」：本操作不改 controller command，負載可能繼續運動。

## 4. INFO／STATUS：64 bytes，ID 0x6F1

| offset | 大小 | 型別 | 定義／單位 |
|---:|---:|---|---|
| 0 | 1 | u8 | version=1 |
| 1 | 1 | u8 | INFO=`0x81`，STATUS=`0x82` |
| 2 | 1 | u8 | node |
| 3 | 1 | u8 | active：0／1 |
| 4 | 4 | u32 | request sequence |
| 8 | 4 | u32 | session ID，最近成功 START 的 request sequence |
| 12 | 4 | u32 | control tick，從 APP 開始控制取樣計數 |
| 16 | 4 | u32 | 本 session sample_drop_count |
| 20 | 4 | u32 | request_drop_count，APP boot 後累積 |
| 24 | 4 | u32 | tx_retry_count，APP boot 後累積的 software enqueue 失敗次數 |
| 28 | 4 | u32 | next sample sequence，包括被 queue 丟棄的樣本 |
| 32 | 4 | f32 | output torque constant，N·m/A |
| 36 | 4 | f32 | gear ratio，motor-side/output-side |
| 40 | 4 | f32 | factory velocity LPF previous weight；**不套用到 telemetry velocity** |
| 44 | 4 | f32 | Iq 抗混疊 cascade 每級 new weight `beta` |
| 48 | 4 | f32 | 當下控制模式使用的 q-axis current limit，A |
| 52 | 4 | u32 | 已接受的 legacy SETPOINT 本地計數器 |
| 56 | 4 | u32 | 最近 control-consumed sequence；armed 狀態需另看 sample，非物理輸出證明 |
| 60 | 1 | u8 | sample period：0=尚未 START，20／40 control ticks |
| 61 | 1 | u8 | stop reason，見下表 |
| 62 | 1 | u8 | legacy control mode：1 MIT、2 POSVEL、3 VEL、4 HYBRID |
| 63 | 1 | u8 | 原始 firmware fault code，非 bitmask，沿用原 fault 定義 |

stop reason：0=未開始或 active；1=host STOP；2=capture heartbeat 逾時；
3=資料非有限值／數值 overflow，或 capture 中 torque constant、gear ratio、direction 改變。
INFO／STATUS 是主迴圈快照，不是額外控制取樣。必須檢查 active，而不能只看 reason=0。
boot 後所有 counter／session reset，不會自動恢復 capture。

## 5. Sample：64 bytes，ID 0x6F2

| offset | 大小 | 型別 | 定義／單位 |
|---:|---:|---|---|
| 0 | 1 | u8 | version=1 |
| 1 | 1 | u8 | type=`0x90` |
| 2 | 1 | u8 | node |
| 3 | 1 | u8 | flags，見下表 |
| 4 | 4 | u32 | sample sequence，本 session 從 0 起 |
| 8 | 4 | u32 | sample endpoint control tick `n` |
| 12 | 4 | u32 | endpoint control-consumed command sequence；disabled／不確定時 `0xFFFFFFFF` |
| 16 | 4 | u32 | 本 session sample_drop_count，**產生該封包當下**的值 |
| 20 | 4 | f32 | 最新 encoder output-side position，rad；沿用 firmware zero／direction |
| 24 | 4 | f32 | output-side raw velocity window average，rad/s |
| 28 | 4 | f32 | 四級抗混疊後再 interval average 的 measured Iq，A |
| 32 | 4 | f32 | `offset28 × output torque constant`，N·m |
| 36 | 4 | f32 | endpoint 實際讀取的 MIT feedforward command torque，N·m；不是完整控制力矩 |
| 40 | 4 | f32 | endpoint 瞬時 measured Iq，A；**僅診斷、未抗混疊** |
| 44 | 4 | u32 | 最近 command sequence／mode 變更被控制路徑讀取的 tick |
| 48 | 4 | f32 | endpoint motor temperature，°C；沿用原廠溫度濾波 |
| 52 | 4 | u32 | 最新 1 kHz encoder velocity 更新 tick `v` |
| 56 | 4 | u32 | 最新 encoder DMA 位置發布 marker tick `p` |
| 60 | 2 | u16 | interval 長度 N：500 Hz=40；1,000 Hz=20 |
| 62 | 1 | u8 | endpoint control mode，同 INFO |
| 63 | 1 | u8 | endpoint fault code，同 INFO |

| flags bit | 意義 |
|---:|---|
| 0 | endpoint armed，原 `console_mode == 2`；不等於保證輸出力矩有效 |
| 1 | interval 內至少一次 requested Iq 被 current limit 截斷 |
| 2 | interval 內至少一次電壓向量被限幅，或 d/q output 到達 scalar limit（含恰好等於邊界） |
| 3 | 濾波 warmup 未達 2,000 control ticks=100 ms；分析須排除 |
| 4 | endpoint command 未確定／disabled；offset12 不可拿來建立 command 關聯 |
| 5–7 | 保留，0 |

bit2 是保守的 controller voltage/modulation saturation 指標，**不是硬體 PWM 比較器量測**。
current/voltage flags 是整段 OR，armed/fault/mode 是 endpoint 值；故障與 mode 變更期間不應
用 endpoint 狀態推論整段皆相同。bus voltage 未放入此 64-byte layout，可透過既有 LIVE 查詢。
IQ／torque 的外部正方向與 raw feedback 一致，position 已含 firmware 校正零位，host offset 不在裝置內。

## 6. 時間、命令來源與資料年齡

時基是 ADC/FOC 控制呼叫計數，名義 20,000 Hz，1 tick=50 µs，**不是 host monotonic clock**。
每次控制 IRQ 完成計算、SVPWM 後加一。32-bit tick 約 59 小時 39 分回繞；所有差值用 unsigned
modulo `2^32`。sample sequence 每次到達輸出 interval 都增加，即使 queue 滿；同樣 modulo `2^32`。
request sequence 不參與時基。長時間 capture 由 host unwrap；跨 reboot／新 session 不延續時間線。
若實機遺漏 ADC IRQ，counter 無法補出真實牆鐘時間；須以 ISR overrun／時鐘實測排除。

不是每一個欄位都同時採樣：

| 訊號 | 真正來源／時間 |
|---|---|
| instantaneous Iq | 該次 IRQ 的 ADC current → Park；ADC acquisition 早於本次新的控制／SVPWM |
| averaged Iq／torque | endpoint `n` 結束的 N 個 20 kHz 樣本，先四級 LPF 再非重疊 average；另外還有 LPF 的過去狀態 |
| position | snapshot 時最新的 encoder DMA publication；marker `p` 為 publication 當下「已完成」control tick，時間落在名義 `[p,p+1)`；不代表 encoder 硬體 latch 時間 |
| velocity | `v` tick 更新的一個 1 ms encoder 累積差分窗口；500 Hz 報最後兩個更新值的 average，1,000 Hz 報一個 |
| command sequence／torque | 本次 prepare current controllers 已讀取的 command；之後 fault/reset 可能阻止有效輸出，須看 flags／fault |
| temperature | endpoint 原廠 filtered motor temperature，不是本次重新取得的瞬時溫度 |

位置與其 DMA marker 在短 critical section 內成對讀取，避免 IRQ1 中途更新造成錯配。
encoder acquisition／DMA completion／position publication 的額外延遲須實機量測，不能從 marker 推得。
velocity 的時間窗口以 `v` 为右端：1,000 Hz 名義 `(v-20,v]`，500 Hz `(v-40,v]`；有效時間約
為 `v-N/2` tick，但 encoder 最近發布時間與數位差分的端點仍有硬體延遲／量化誤差。
`n-v` 說明 held velocity 的資料年齡。極少數 reset／異常 divider 造成窗口沒有更新時值為 0，
host 應依 `v` 是否前進偵測並排除，不把這個 0 當作真實靜止。

命令本地 sequence 自 APP boot 起 0；**每個 decoder 接受的 legacy SETPOINT**增加一次。
不包含 enable、disable、refresh、清 fault、set-zero、管理 request。counter 跳過 `0xFFFFFFFF`
保留值，因此由 `0xFFFFFFFE` 回到 0；不是普通完整 u32 modulo sequence。
MIT 在 20 kHz 路徑讀取後更新 applied sequence；VEL/POSVEL/HYBRID 在 1 kHz outer loop
讀取後才更新。CAN IRQ 被 ADC IRQ 插入 command 寫入中時標示 unknown；不重寫原有 command ABI。
offset44 記錄 sequence／mode 變更的 consumption tick，和 Iq acquisition 不是同一事件。
它不表示電流或機械響應已完成，也不表示命令在每個 20 kHz 子樣本內都一樣。

例如當 tick=n 的 ADC 量到 Iq，再用新命令計算 PWM，這個 Iq 主要對應先前 PWM；host 不能
把 offset12 的「新 applied sequence」解讀為該 ADC 樣本已受到新命令作用。
command torque offset36 僅 MIT feedforward；MIT `Kp/Kd != 0` 时還有 feedback 項，VEL 等模式
則由 outer loop 產生 Iq reference，該欄可能保留之前 MIT 值，**不得拿它當 VEL 實際控制力矩**。

原有命令没有 host sequence 欄位；本協定不把 legacy counter 偽裝成 host tag。想建立精確命令
對照需單一 writer、由 STATUS 建立 counter baseline，記錄所有已接受 SETPOINT，處理丟命令／reset。
若需要每一個 command 的 host-tagged ACK、逐 tick command history，屬後續協定擴充，不在 v1。

## 7. 抗混疊與量測尺度

Iq 每級：`y[k] = y[k-1] + beta * (x[k] - y[k-1])`，四級串接，
`beta=0.015465039`（binary32，名義 `2π·50/(20000+2π·50)`）。START 第一個樣本將四級狀態
初始化為當時 Iq，前 100 ms 設 warmup。再把 N=20/40 個四級輸出作非重疊算術平均。

完整數位傳遞函數：

```text
H(z) = [beta / (1-(1-beta)z^-1)]^4 × (1/N) × Σ(j=0..N-1) z^-j
```

四級合成 -3 dB 約 21.5 Hz，不是 50 Hz；低頻 group delay 約 12.73 ms，加上平均窗口
0.475 ms（1 kHz）／0.975 ms（500 Hz）。實際各頻率 amplitude／phase 必須由 H 求出，不能
只扣一個固定延遲。20 Hz measured gain 約 0.73947／0.73801；FRF 分析必須納入這個顯著衰減。
四級在 250／500 Hz 的抑制約 57／80 dB（再加平均窗口）；不是理想 brick-wall，仍可能 alias。
offset40 瞬時 Iq 不套用 H，也**不能替代** offset28 作抗混疊輸入。

velocity 不套 factory 一階 LPF，但 encoder difference 自帶 1 ms 矩形窗口；500 Hz 再平均兩個
窗口。此窗口不是高抑制抗混疊器，高頻 encoder noise／高速運動仍可能 alias。
position 未另加 LPF／抗混疊；本版面向低頻機械辨識，非寬頻 encoder noise 分析。
INFO 的 factory coefficient 只是協助記錄控制器 velocity filtering，不代表 telemetry coefficient。

torque estimate 是 measured electrical Iq × **output-side** torque constant，不再乘 gear ratio。
不是軸端 torque sensor，受 ADC offset/gain、current calibration、磁鏈／溫度與 transmission loss 影響。
負載 torque／摩擦辨識不能把它當獨立力矩感測器。禁止辨識開始時自动校正、改 controller 參數
或將結果寫回控制器；capture 中禁止 set-zero、校正、參數写入／配置修改。
K／gear ratio／direction 變更會停止 capture；其他會改變觀測語義的操作由 host 確保隔離。

對現有三種量測的影響：friction ramp 的 breakaway velocity 是窗口值，filtered torque 又有
約13 ms延遲，不能直接套「同一列首次越界的 torque」而不做時間對齊；command對照仍須保留。
velocity 的定速平台需排除過渡與warmup，平台平均值可減少此延遲影響。FRF 則須保存v1濾波設定：
若 `T_report=H_iq*T_true`、`V_report=H_velocity*V_true`，則
`G_report=V_report/T_report=G_true*H_velocity/H_iq`，所以
`G_true=G_report*H_iq/H_velocity`；不是單純對所有曲線除以同一濾波器。
command→reported torque 的傳遞函數則包含H_iq及電氣／取樣延遲。
H_velocity僅是名義差分／平均窗口模型，encoder發布延遲須另測，不能把公式當作已驗證的補償。
低幅值highfrequency去濾波會放大noise，需用coherence／SNR限制可用頻段。

## 8. Queue、丟失、heartbeat 與狀態

sample queue 深度 16，約容納 16 ms（1 kHz）／32 ms（500 Hz）。滿時 drop **new**，
`sample_drop_count` 遞增、sequence 繼續前進，舊資料不覆蓋。新成功 START 明確丟棄舊 session
尚未送出的 queue，session／sequence／sample drop 重新開始；這是使用者要求的 session reset，
不是正常 streaming 的隱性丟樣本。STOP／timeout／invalid data 留下舊 queue 繼續排送。

主迴圈每次最多嘗試一個 management reply 加一個 sample；未成功入硬體 FIFO 時保留並計
`tx_retry_count`。ADC 路徑只作有界濾波／快照／入列，不呼叫 CAN sender。CAN IRQ 只入 request
queue；舊命令路徑仍依原設計執行。sender 不忙等 FIFO，檢查 INIT、bus-off、full、slot index。
寫入成功只代表已交給 MCAN，**不是 bus ACK 或 host 已接收**；硬體／host 端丟失請看 sample
sequence gap、MCAN error與 host socket drop。tx_retry_count 不等於 lost sample count。
舊封包中的 drop counter 可能尚未反映後來發生的 overflow；STOP 後須再 STATUS 取得最終值。

CAPTURE_HEARTBEAT 必須小於 500 ms；建議每 100 ms 一次，watchdog 於差值達 10,000 tick 時
停止 capture。它只保護量測 session，**不 refresh 原廠 motor 通訊逾時**。原有 SETPOINT／合法
motor command 必須照舊維持，enable/disable/fault/watchdog 邏輯没有被繞過。
disable／fault 不直接停止被動量測，讓 host 能保留停機資料；sample armed=0、applied=unknown。
capture timeout／STOP 本身不是停機命令；第 10 節的獨立 setpoint／scan lease 與 guard 另外走 disabled／fault／reset 路徑。應用仍須明確 disable。

request 已收到（IRQ 入列）、已接受（主迴圈 ACK）、motor command 已讀取（sample applied）、
輸出電流已響應、機械停止，是五個不同事件。第一階段沒有 test-completed 事件。

## 9. 範例封包與 host 實作順序

這些範例只是格式，不授權自動改裝置設定或啟動輸出。

```text
# node 1，500 Hz，session/request seq=0x12345678
request 6F0 [8]  01 01 01 00 78 56 34 12
ACK     6F1 [8]  01 80 01 00 78 56 34 12
# 同格式 1 kHz：argument=01
request 6F0 [8]  01 01 01 01 78 56 34 12
# already active -> busy
ACK     6F1 [8]  01 80 01 03 78 56 34 12
# HEARTBEAT，seq=0x12345679
request 6F0 [8]  01 01 03 00 79 56 34 12
ACK     6F1 [8]  01 80 03 00 79 56 34 12
# STATUS seq=0x1234567A，成功回 64 bytes type82，不另回 ACK
request 6F0 [8]  01 01 05 00 7A 56 34 12
# STOP seq=0x1234567B
request 6F0 [8]  01 01 02 00 7B 56 34 12
ACK     6F1 [8]  01 80 02 00 7B 56 34 12
```

完整 sample FD64（人造 fixture，不是實機 capture）：node1，flags=armed+warmup，sample0，
endpoint tick40，applied seq1，position1.25 rad，velocity-2 rad/s，mean Iq3 A，torque1.5 N·m，
command-1 N·m，instant Iq3 A，applied tick1，temperature30°C，velocity tick40，position marker39，N40，MIT，無fault。

```text
6F2 FD64 BRS
01 90 01 09 00 00 00 00 28 00 00 00 01 00 00 00
00 00 00 00 00 00 A0 3F 00 00 00 C0 00 00 40 40
00 00 C0 3F 00 00 80 BF 00 00 40 40 01 00 00 00
00 00 F0 41 28 00 00 00 27 00 00 00 28 00 01 00
```

最小 Python decoder（額外檢查 CAN standard/data/FD/length flags）：

```python
import struct

def decode_sample(payload):
    if len(payload) != 64 or payload[1] != 0x90 or not (payload[0]==1 or (payload[0]==2 and payload[62]==3)):
        raise ValueError("unsupported sample")
    seq, tick, applied, drops = struct.unpack_from("<4I", payload, 4)
    position, velocity, iq_mean, torque, command, iq_instant = struct.unpack_from("<6f", payload, 20)
    applied_tick = struct.unpack_from("<I", payload, 44)[0]
    temperature = struct.unpack_from("<f", payload, 48)[0]
    velocity_tick, position_tick, period, mode, fault = struct.unpack_from("<IIHBB", payload, 52)
    return dict(node=payload[2], flags=payload[3], sequence=seq, tick=tick,
                applied=applied, dropped=drops, position=position, velocity=velocity,
                iq_mean=iq_mean, torque=torque, command=command, iq_instant=iq_instant,
                command_kind="velocity_setpoint_rad_s" if payload[0]==2 else "mit_feedforward_nm",
                velocity_setpoint_changed=bool(payload[3]&32),
                applied_tick=applied_tick, temperature=temperature,
                velocity_tick=velocity_tick, position_tick=position_tick,
                period=period, mode=mode, fault=fault)
```

host 建議順序：INFO 核對 version／K／gear／濾波 → STATUS 確認 inactive → START 使用新的
session tag → 保持 legacy control watchdog＋100 ms capture heartbeat → 排除 warmup／fault／
saturation／sequence gap 的區段 → STOP → drain queue → STATUS 留存最終 drops／reason。
host clock 仍可另存 tx/rx 時間作 bus latency 診斷，但辨識時間線以 firmware tick／各訊號窗口為準。
現有 `damiao_ws` 使用 refresh 收資料的程式不能原樣直接讀本協定；API 需要獨立 FD socket
接收路徑與 decoder，不得以 legacy refresh response 假裝同步 telemetry。

## 10. Independent guard and latest-state extension (2026-10-04)

Applies to all three sysid variants; ordinary factory/raw/no-response variants do not provide it. Update all eight nodes, including holders. Management never enables a motor, writes offsets or persists these limits. Existing calibration sample/scan layouts stay unchanged.

Requests remain classic/FD8 at 0x6F0 with argument=0. New operations:

| op | Name | payload32 / reply |
|---:|---|---|
| 21 | SAFETY_STATUS | request sequence; FD64 type 0x84 at 0x6F1 |
| 22 | LIVE_START | live session token; ACK8; same-token retry is idempotent, different active token returns BUSY |
| 23 | LIVE_STOP | owned live token; ACK8 |
| 24–27 | GUARD_LOWER / UPPER / SPEED / TORQUE | raw position bounds, positive maximum absolute raw velocity and Iq-derived torque limit, f32 bits; ACK8 |
| 28 | GUARD_ARM | request sequence; ACK8; atomically commits a complete valid staged envelope |

GUARD_LOWER begins a new staging mask; the previous active envelope remains effective throughout staging. ARM requires all four fields, finite ordered bounds, positive speed/torque, no guard latch, healthy latest state inside new position/velocity bounds. Host reads back SAFETY_STATUS and compares the exact f32 values. Commands are raw firmware coordinates; offset/reversed conversion is the host's responsibility.

An unconfigured sysid node cannot remain armed. The 20 kHz control IRQ checks the previous published state (nominal one-tick delay; velocity itself updates around 1 kHz). It latches a fault on missing guard, expired control lease, nonfinite/invalid state, position/velocity/instantaneous Iq-derived torque violation, motor/MOS temperature above 70°C, or position/velocity publication older than 400 ticks (20 ms). ARM also rejects stale publication. The default lease is fixed 5,000 nominal 50 us ticks (250 ms), independent of persisted TIMEOUT. Only accepted motor SETPOINT refreshes it; refresh, ACK, enable and capture heartbeat never renew a host setpoint. A scan-owned trajectory instead requires healthy scan ownership, active capture and a heartbeat younger than 5,000 ticks, including completed-home wait. Scan abort reason 10 indicates this protection.

On violation the controller enters existing console disabled/state-change/fault/reset behavior, records communication fault 0xD unless an existing fault is already present, and clamps the current reference to zero. The latch cannot be cleared by a late command; explicit legacy CLEAR_FAULT or reboot clears it. CLEAR also invalidates the last setpoint. No host script automatically clears it. Active envelopes remain across CLEAR but are volatile across reboot. Guard current cap `min(existing_current_limit, guard_torque_limit/Kt)` covers all modes and all guarded nodes; scan torque limit is additionally applied while scan owns control. It is not a calibrated shaft-torque or mechanical stopping guarantee. Disabling torque removes gravity support; PWM behavior/physical stopping and real bus/WCET timing still require hardware validation.

SAFETY_STATUS payload:

| offset | type | meaning |
|---:|---|---|
| 0–3 | 4×u8 | version=1, type=0x84, node, flags: bit0 deadman enabled, bit1 latched, bit2 live active, bit3 guard enabled |
| 4 / 8 | u32 | request sequence / control tick |
| 12 / 16 | u32 | command lease ticks=5000 / live period ticks=40 |
| 20 / 24 | u32 | last accepted setpoint tick / overwritten-live count |
| 28 / 32 / 36 / 40 | f32 | active lower / upper / maximum velocity / torque limit |
| 44 | u8 | reason: 0 healthy; 1 unconfigured, 2 lease, 3 position, 4 velocity, 5 torque, 6 temperature, 7 invalid state, 8 encoder publication stale |
| 45–63 | bytes | reserved zero |

### Latest-state FD64 at 0x6E0

One live source per bus. This uses a distinct CAN ID with higher arbitration priority than 0x6F1–0x6F3. The service loop queues its newest snapshot before management/record replay. Existing hardware FIFO occupancy and other lower-ID traffic can still delay it; no hard latency guarantee. Firmware holds only the newest pending packet, overwrites obsolete state and increments a diagnostic counter. No ACK/replay/ordering-pair dependency. LIVE is independent of capture activation and uses the same nominal 500 ms heartbeat TTL to stop publishing when its host disappears; motor lease remains 250 ms.

| offset | type | meaning |
|---:|---|---|
| 0–3 | 4×u8 | version=1, type=0x92, node, flags as calibration but no warmup bit |
| 4 / 8 / 12 / 16 | u32 | live session token / endpoint tick / applied command sequence / overwritten-live count |
| 20 / 24 | f32 | instantaneous raw position / latest raw velocity |
| 28 | f32 | MOS temperature °C (not calibration averaged Iq) |
| 32 / 36 / 40 | f32 | instantaneous Iq×Kt torque / applied MIT feedforward / instantaneous raw Iq |
| 44 / 48 | u32 / f32 | command consumption tick / filtered motor temperature °C |
| 52 / 56 | u32 | velocity publication tick / position publication tick |
| 60 / 62 / 63 | u16 / u8 / u8 | fixed period=40 / mode / fault |

Use an independent SDK object/socket `SystemIdentification(..., live_only=True)`, `start_live()`, `heartbeat()`, `safety_status()` and `read_latest()`. The live socket filters only 0x6F1 and 0x6E0; recording filters remain 0x6F1/0x6F2/0x6F3. `read_latest()` returns only newer endpoint ticks from the owned session and never places live packets into the reliable calibration queue. In the returned live measurement, `averaged_iq` is an alias of instantaneous Iq, and `mos_temperature` is explicit; live packets are not interchangeable with averaged calibration samples. Applications enforce freshness and keep independent durable recording. Live session tokens attribute ownership; they are not authentication.

## VEL applied reference extension (stream version 2)

Requests/ACK/INFO/STATUS/scan-management retain version 1. SAFETY_STATUS flags bit4 advertises `applied_velocity_supported`; no new frames or CAN IDs are added. Canonical type 0x90 and live type 0x92 use version 2 **only in VEL mode (mode=3)**. At offset36, version 2 contains the signed output-rad/s `velocity_target` actually used by the speed loop after its clamp and slew, instead of MIT feedforward torque. Applied sequence/tick retain their existing meaning. Other modes preserve the original version 1 field and units. An old SDK rejects the new capability/version rather than treating rad/s as Nm.

Canonical VEL flags bit5 means the applied velocity reference changed within the telemetry interval; reject such intervals for steady-platform fitting. The field is the endpoint reference, not an interval average. Also compare reference values between consecutive frames. Live is instantaneous and does not set bit5. Endpoint reference can precede physical tracking; it is not measured velocity. The API exposes `applied_velocity_setpoint`, `velocity_setpoint_valid`, and `velocity_setpoint_changed`; host reversal changes its sign once, position offset does not affect it.

Velocity measurement scripts check capability before enable, retain both requested and applied references, and classify steady platforms by firmware reference even during delayed canonical replay. Missing legacy reference is not inferred from a host command. Reliable retention/ACK and 500/1000 Hz cadence are unchanged.

### Motion command input validation (sysid builds)

Before changing setpoints or renewing the 250 ms command lease, sysid firmware rejects non-standard/remote frames, unsupported command families, wrong-node IDs, invalid DLC, non-finite floating-point motion fields, invalid gear ratios, and velocity scaling overflow. MIT/POSVEL/HYBRID and special commands require 8 bytes; VEL accepts 4 or 8 bytes (the current API sends 8). Special enable/disable/zero/clear-fault sequences require the complete 8-byte pattern. Malformed frames do not renew the sysid command lease; if valid traffic stops, the existing timeout/latch applies. This validation does not add authentication, a hardware watchdog, or power-stage isolation. Parameter requests also require an 8-byte standard data frame; parameter value semantics are unchanged. Recovery/factory builds retain their original receive behavior.

Checks: `tools/recovery/test_command_validation.c` runs the production validation helpers with ASan/UBSan; `tools/recovery/verify_command_validation.py --elf build/<model>_sysid.elf` executes command decoding from the built ARM image, including malformed-frame rejection without setpoint writes and normal VEL/special commands.
