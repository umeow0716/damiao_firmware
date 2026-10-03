# Damiao V3/V4 版本與實作差異分析

最後更新：2026-10-03

本文件回答三件事：四碼版本號怎麼解讀、目前三份 V4 彼此差在哪裡，以及「V4 校正／馬達驅動有
增強」能否由 binary 證實。結論來自官方版本規則與本地 decrypted image 的 Thumb-2 disassembly；
Ghidra C 只用來協助閱讀，最終判斷以函式邊界、指令、常數、呼叫圖與初始化資料為準。

## 先說結論

- 官方規則成立：四碼的前兩碼是電機系列／硬體相容編碼，後兩碼是軟體版本，尾端 `_NN` 是
  子版本。升級時必須先匹配前兩碼。
- 「前兩碼」不是永遠一對一對應馬達名稱。V4 的 `70` 同時涵蓋 24 V DM4310 與 DM4340；
  型號差異放在出廠設定與封裝檔名中。48 V 43 系列另用 `71`。
- V4 的校正確實有實作級增強：更多板端自動處理、雙向／多輪取樣、256 點表格整理、感測器
  範圍檢查與校正資料持久化，不只是字串或參數改名。
- V4 的即時控制仍是相同的 FOC 骨架；明確升級點是角度校正表示法、每個 IRQ 都執行的
  wrap-aware 角度／速度觀測器，以及 ISR 執行時間監測。沒有證據顯示 PI、Clarke/Park 或 SVPWM
  被替換成完全不同的控制法。
- V7018 到 V7038 的馬達辨識演算法有再改版；V7038 的第一階段改成累積平方與互乘統計量後
  求解，不再保存 V7018 的 200 點樣本陣列。
- 本地 V7038 與 V7060 的控制程式幾乎相同。後者主要是 DM4340 的識別、馬達參數與辨識激勵
  預設值，不能把 `60 > 38` 解讀成另一代 FOC。

## 版本號的可靠解讀

[dmBots 官方版本說明](https://github.com/dmBots/motor-firmware/blob/master/%E7%89%88%E6%9C%AC%E8%AF%B4%E6%98%8E/%E5%9B%BA%E4%BB%B6%E7%89%88%E6%9C%AC%E8%AF%B4%E6%98%8E.md)
明確定義前兩碼為電機系列編碼、後兩碼為版本號、`_NN` 為子版本。官方同時警告升級時要選擇
前兩碼相同的韌體。

### V3 本地樣本

| 前兩碼 | 型號／硬體 | 後兩碼 | 子版本 |
|---:|---|---:|---:|
| 50 | DM4310 V3 | 17 | 04 |
| 51 | DM4340 V3 | 17 | 04 |
| 56 | DM10010 V3 | 17 | 04 |
| 57 | DM3507 V3 | 17 | 04 |
| 60 | DM4310 V3 48V | 17 | 04 |
| 61 | DM4340 V3 48V | 17 | 04 |
| 63 | DM8006 V3 | 17 | 04 |
| 64 | DM8009 V3 | 17 | 04 |
| 65 | DM3507 V3 48V | 17 | 04 |

九份映像都是同一個 V17 軟體世代配上不同硬體 profile。官方 V17 說明列出的公開變更為：

1. 修正 CAN 參數讀寫與控制指令同時傳送時的錯位／掉幀；
2. 零點 Flash 儲存位置換頁；
3. 移除 UART 設定零點指令（官方 `master` 簡版目前只列前兩項與升級注意事項）。

目前 `reference/` 沒有 V13/V15，所以本專案可以確認 V17 binary 的行為，不能只靠 release note
宣稱已對 V13→V17 的每一項差分完成反組譯。

### V4 為什麼同時有 7018、7038、7060？

官方 V4 檔案清單呈現以下相容碼：

| 前兩碼 | 官方目錄中的產品 |
|---:|---|
| 70 | DM4310 V4 24V、DM4340 V4 24V |
| 71 | DM4310 V4 48V、DM4340 V4 48V |
| 72 | DM6248 V4 |
| 73 | DM8006 V4、DM8009 V4 |

因此 `70` 應讀成「V4 24 V 43 系列相容平台」，不是「DM4310 專屬代碼」。`18`、`38`、`60`
仍是軟體版本碼，但不同馬達可停留在不同 release branch；數字較大不保證控制核心必然不同。
本地 binary 正好提供反例：7038 與 7060 幾乎是同一份程式配不同型號資料。

## 分析範圍與量化

| 映像 | bytes | Ghidra functions | 函式內指令 | 函式 body bytes |
|---|---:|---:|---:|---:|
| DM4310 V3 5017.04 | 51,284 | 191 | 12,504 | 38,444 |
| DM4310 V4 7018.04 | 58,796 | 210 | 15,580 | 48,554 |
| DM4310 V4 7038.02 | 55,508 | 195 | 14,704 | 45,966 |
| DM4340 V4 7060.02 | 55,508 | 195 | 14,704 | 45,966 |

函式數是反組譯器依入口點建立的統計，不等於原始 C 函式數；它只適合在相同分析流程中比較。
以不要求相同地址的函式 multiset 比對：

| 比較 | byte-exact | normalized instruction | instruction shape |
|---|---:|---:|---:|
| V3 5017 → V4 7018 | 91 | 113 | 139 |
| V3 5017 → V4 7038 | 93 | 112 | 138 |
| V4 7018 → V4 7038 | 119 | 143 | 176 |
| V4 7038 → V4 7060 | 192/195 | 192/195 | 194/195 |

地址、literal pool、SRAM layout 或編譯排程變動都會讓 byte hash 不同，因此不能把未匹配函式數
直接當成「算法改了幾個」。下文只列控制流與數學實作可以確認的差異。

## V3 各型號之間

以 DM4310 V3 為基準，不要求函式位址相同時，DM4340 有 190/191 個 byte-exact 函式、191/191
個相同 instruction shape；差異集中在出廠設定。其他 V3 型號仍大量共用實作：

| 型號 | exact common functions | normalized common | shape common |
|---|---:|---:|---:|
| DM10010 | 163/191 | 173/191 | 184/191 |
| DM3507 | 149/191 | 176/191 | 187/191 |
| DM3507 48V | 147/191 | 172/191 | 184/191 |
| DM4310 48V | 148/191 | 172/191 | 188/191 |
| DM4340 48V | 148/191 | 172/191 | 188/191 |
| DM8006 | 152/185 | 159/185 | 171/185 |
| DM8009 | 136/185 | 158/185 | 171/185 |

這支持目前 source tree 採取「共用控制實作 + target profile + 必要 layout 分流」的架構。函式地址
漂移與常數不同很多時，仍可有相同 instruction shape；不能為了 factory 地址把共用邏輯複製九份。

## V4 7038 與 7060：實際只差什麼

兩份映像有 195 個相同入口函式，其中 192 個函式逐 byte 相同。只有三個函式 body 不同：

| 地址 | 差異 |
|---:|---|
| `0x00022874` | 載入型號設定；版本字從 `7038` 改為 `7060`，馬達辨識 runtime excitation 預設從 `1.0f` 改為 `2.0f`。 |
| `0x000252a4` | startup/main 傳遞的版本識別從 `7038` 改為 `7060`。 |
| `0x00026920` | UART/debug 顯示的版本整數從 `7038` 改為 `7060`。 |

另外，函式外的 scatter-initialized factory record 包含型號參數：

| 設定 | DM4310 V7038 | DM4340 V7060 |
|---|---:|---:|
| rotor inertia | `1.8e-5` | `2.0e-5` |
| phase resistance | `0.85` | `0.88` |
| phase inductance | `0.000345` | `0.000360` |
| flux linkage | `0.00450` | `0.00485` |
| gear ratio | `10` | `40` |
| velocity mapping maximum | `30` | `10` |
| torque mapping maximum | `10` | `28` |
| speed-loop Kp | `0.00372` | `0.00384` |

FOC IRQ、校正、通訊、安全與其餘 motor-identification 指令在這兩份映像中相同。V7060 因此是
相同 V4 24 V 平台實作的 DM4340 profile，而不是比 V7038 多一套控制演算法。

## 校正流程的變化

### V3 5017

- setup event 依序做方向／極對數偵測，再做 `pole_pairs × 256` 點的正反向 alignment sweep；
- sweep 會將量測樣本送出，校正工作流仍有明顯的 host-assisted 特徵；
- output-sensor calibration 函式約 288 條指令，主要找 U/V extrema、offset、比例與相位；
- current-sensor calibration 取 1,000 次平均，檢查 U/V sensor 是否斷線；
- motor identification 是另一個 event，不在 setup chain 裡自動串完。

### V4 7018

- setup event 變成方向偵測 → motor identification → 新的板端自動校正 worker；
- 自動校正 worker 約 612 條指令，包含多段 256 點資料處理、sensor range 檢查與校正結果整理；
- main 在 worker 返回後執行 `E_OFF`／sensor 診斷並持久化 `0x103` words；
- output-sensor calibration 擴張到約 786 條指令，進行多輪掃描、256 點表格處理、U/V/W/C
  診斷並持久化 `0x104` words。

### V4 7038／7060

- setup 與 output calibration 的數學工作流延續 V7018；
- worker 分別增至約 669 與 817 條指令；
- 驗證、輸出與 `0x103`／`0x104` words persistence 從 main 收進 worker，屬於責任重整；
- output calibration 仍有四輪掃描與多個 256-entry pass，不是退回 V3 的簡化流程。

所以「V4 校正增強」可由 disassembly 直接確認。可預期的目的包括減少外部工具依賴、用正反向
資料降低回差／方向偏差，以及在寫入前阻擋明顯失效的感測器；實際精度提升幅度仍需同一顆馬達
重複校正後比較 residual 才能量化。

## 馬達辨識的變化

三代都辨識相電阻、相電感、磁鏈與機械參數；差別主要在第一段電氣參數估測：

| 映像 | motor-ID 指令數 | 第一階段特徵 |
|---|---:|---|
| V3 5017 | 880 | 60,000 次 loop；20,000 次後持續送入固定 SRAM accumulator/helper。 |
| V4 7018 | 970 | 40,000 次 loop；後半段每 20 次保存一點，共 200 點，再由幅值／相位 helper 求參數。 |
| V4 7038/7060 | 985 | 40,000 次 loop；後半段在線累積 `x²`、`x·y`、`y²` 的 64-bit 統計量，再以閉式相關／normal-equation-like 計算求解。 |

`normal-equation-like` 是由乘積累積與後續除法／平方根資料流作出的算法分類，不是從原始符號名稱
得知。它避免 V7018 的兩個 200-float sample buffers，並讓所有後半段樣本直接參與統計；這很可能
是 V7038 縮小映像與提高估測穩健性的原因之一，但「精度提高多少」仍須硬體重複性測試。

V7060 的辨識程式與 V7038 相同，只把機械 excitation 預設由 `1.0f` 提到 `2.0f`，並使用 DM4340
的 R/L/flux/inertia/gear 參數。因此不能直接把該激勵值搬到 DM4310。

## 即時馬達驅動／FOC 的變化

三份代表映像的主要 PWM/FOC IRQ 分別是 336、335、335 條指令，V3→V4 的 mnemonic sequence
相似度約 86–88%。共同骨架仍是：

```text
ADC／角度取樣
  → offset/scale
  → Clarke/Park
  → mode/position/velocity outer loop
  → d/q current PI 與限幅
  → inverse Park
  → SVPWM／PWM update
```

可確認的 V4 改動如下：

1. **角度校正表改版**：V3 以量化角度直接查 4,096-entry `uint16_t` mapping；V4 改成 256-entry
   float correction，對相鄰點線性插值後加回原始角度。這與 V4 新的 256 點板端校正流程相配。
2. **機械觀測器升級**：V3 的較小狀態 helper 每 20 個 IRQ 更新一次；V4 使用帶角度 wrap、角度／
   速度／偏差狀態的 observer，且每次 IRQ 都更新，再把結果供較慢的 outer-loop 使用。
3. **IRQ timing instrumentation**：V4 在 IRQ 入口與出口讀 timer counter，保存本次與最大執行時間，
   可用來偵測即時路徑超時或評估餘裕。
4. **感測前處理重整**：V7038 將 V7018 inline 的 U/V sensor 前處理收進狀態 helper；這部分主要是
   state layout 與責任重新編排，不應單獨算成新控制法。

因此可以說 V4 的「馬達驅動增強」主要是 feedback quality、估測頻率與可觀測性提升。現有靜態
證據不足以宣稱 torque bandwidth、效率或穩定裕度一定比 V3 高；那些結論需要波形與帶載量測。

## 若要把 V4 改進帶回目前 source

建議分層，不要直接抄 V4 SRAM 地址或整段 assembly：

1. **低耦合、優先研究**：IRQ execution-time telemetry、校正結果 range check、校正資料寫入前驗證。
2. **中等風險**：V4 板端 256 點正反向校正與 V7038 motor-ID 統計求解；先做 host fixture，比較
   V3/V4 算法對相同 capture 的 residual 與重複性。
3. **高風險**：V4 wrap-aware observer。它同時依賴取樣週期、狀態係數、位置感測尺度、電流／
   torque feed-forward 與 V4 硬體噪聲特性，必須 source-level 重建並重新調參。
4. **不要移植**：V4 的固定 SRAM 位址、scatter layout、ROM-helper veneer 地址與 model-specific
   excitation。V3/V4 initial MSP 與 fixed/state region 已不同，raw address transplant 會破壞 ABI。

最低硬體驗收應包含：無功率 ADC/encoder 範圍、低壓限流校正、R/L/flux 多次辨識方差、正反轉
角度 residual、observer phase lag/noise、電流 step response、fault 關斷與最壞 IRQ 執行時間。

## 證據界線

| 敘述 | 證據等級 |
|---|---|
| 四碼與子版本規則 | 官方文件確認 |
| V3/V4 目錄對應的硬體碼 | 官方檔名目錄 + 本地檔名確認 |
| 7038/7060 只有三個函式 body 不同 | 全函式 byte hash、normalized token 與 disassembly 確認 |
| 7038/7060 出廠參數差異 | scatter decode 後 37-word factory record 確認 |
| 校正 loop、表長與 persistence word count | Thumb-2 control flow／常數／呼叫參數確認 |
| V7038 使用二階統計量求 motor-ID | 浮點乘積、64-bit 累積與後續求解資料流確認 |
| V4 observer 每 IRQ 執行 | IRQ call graph 與 branch placement 確認 |
| 精度、扭矩、效率或穩定性實際提升幅度 | **尚未由實機 A/B 證明** |

參考檔、SHA-256 與解密方法見 [參考韌體目錄](REFERENCE_FIRMWARES.md)。分析使用的 Ghidra
匯出器保存在 `tools/recovery/`；可重建的中間結果位於 ignored `build/v4-analysis/`，沒有依賴 `/tmp`。
