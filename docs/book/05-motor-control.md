# 5. 馬達控制與即時資料流

控制核心在 ADC1 EOCA 的 IRQ002，以 20 kHz 執行；每 20 tick 執行一次 1 kHz 外迴路。控制程式
跨越 `interrupts.c`、`motor_control.c`、`safety.c`、sensor modules、`common/motor_math.c` 與
TMR4 board driver。

## 20 kHz pipeline

```text
ADC raw
  → temperature / bus voltage / phase current scale
  → output analog angle + motor encoder latest sample
  → electrical angle + sin/cos
  → Clarke αβ → Park dq
  → 每 20 tick：position/speed outer loop + motion observer
  → d/q current controller
  → safety/fault + armed state
  → vector magnitude limit
  → inverse Park αβ
  → SVPWM compare
  → ADC/TMR4 interrupt acknowledge
```

實際順序由 `adc_foc_control_irq()` 明確編排。fault monitor 位於 controller helper 與 armed-state reset
之間；PWM publication、IRQ ack、communication age 等順序都已納入差分驗證。不要只看數學等價就
任意重排。

## 四種控制模式

| 模式 | `MotorControlMode` | 主要 q-current 來源 |
|---|---:|---|
| MIT | 1 | `kp·position_error + kd·velocity_error + torque_feedforward`，再除 torque constant |
| Position-Speed | 2 | position loop 產生 velocity target，再由 speed loop 產生 current |
| Speed | 3 | 限幅／slew 後的 velocity target進 speed loop |
| Hybrid | 4 | position-speed 路徑，但使用命令提供的 current limit |

所有合法模式最後都進相同 d/q current controller、vector limit 與 SVPWM。未知／disabled 模式不應
沿用舊 command output。

## current loop 與 observer

`CurrentController` 是固定 19-float ABI，d/q 兩軸共用同一演算法。`MotionObserver` 估測 velocity 與
disturbance current，供非 MIT 外迴路使用。controller 係數由 `app_config` 與 commissioning runtime
cache 推導，不應在每個 20 kHz tick 重算。

修改 controller 時要分清：

- **產品可調參數**：放 `MotorConfig`／parameter protocol／profile defaults。
- **推導係數**：在 configuration/commissioning 階段更新。
- **每 tick 狀態**：固定 runtime state；避免加入除法、格式化或無界迴圈。
- **純數學**：若多模組共用，放 `common/motor_math.c`。

## 浮點語意

build 使用 Cortex-M4F hard-float 與 `-ffp-contract=off`。這不是單純追求 factory 指令外觀：長時間
控制 tick 會因 VFMA 與分開 VMLA 的 rounding 差異累積。部分函式使用 inline ASM 保留 operand
順序、NaN/unordered 判斷、FPSCR side effect 或非標準呼叫 ABI。

可以用普通 C 表達的產品邏輯應維持 C；只有 ABI／浮點 side effect 已被證明必要的窄邊界才使用
ASM。不要把整個新控制功能寫成 ASM，也不要為了「清理」而無證據地刪除窄邊界 ASM。

## SVPWM 與輸出

current controller 產生 d/q voltage，經 vector magnitude limit 與 inverse Park 得到 α/β，最後
`svpwm_helper()`／`board_sampling_timer_write_space_vector()` 寫 TMR4 compare。

disabled 或 fault 時 controller dynamic state 會 reset，輸出回 neutral compare；pin mux/MOE 並不是
每次 FC/FD 都重新配置。修改輸出 policy 時必須同時驗證 compare 值、相序、dead time、ADC trigger
位置與 fault shutdown latency。

## 時間尺度

| 工作 | 名義頻率 | 修改風險 |
|---|---:|---|
| ADC/FOC/current loop | 20 kHz | 最高；直接影響 IRQ deadline |
| position/speed outer loop | 1 kHz | filter、slew、observer 與 fault 時間尺度 |
| control status event | 1 kHz | fault LED/main event |
| CAN | 事件驅動 | command age 與 bus load |
| main deferred work | 非即時 | 可阻塞，但執行 Flash 時需正確中斷／周邊收尾 |

更改 PWM/ADC 頻率時，所有以 tick 計數的 debounce、decimation、sample period 與 commissioning step
都要重新審查，不能只改 timer period。

## 驗證控制修改

最低限度包括：

1. 純數學邊界：零、正負飽和、NaN/Inf、wrap、量化極值。
2. 模式切換：FC、FD、fault、FB、MIT/position-speed/speed/hybrid。
3. 長序列：observer、filter、integrator 與 slew 的累積誤差。
4. ISR 順序與 timing：ack、PWM publication、fault latch、communication age。
5. 低壓限流實機：正負小命令、相序、d/q sign、shutdown timing、CAN loss。

普通功能修改不要求 factory byte equality，但固定 SRAM boundary 與已知輸入的行為差分不能被跳過。

[上一章：感測器與校正](04-sensors-calibration.md) · [下一章：通訊協定](06-protocols.md)
