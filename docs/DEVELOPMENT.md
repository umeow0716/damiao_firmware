# 開發、建置與提交指南

本文件是日常操作手冊；系統原理見 [完整韌體手冊](book/README.md)，檔案 ownership 見
[原始碼地圖](SOURCE_MAP.md)。

## 開發環境

需要：

- CMake 3.20 以上；
- GNU Make；
- Python 3；
- repository 的 `tools/arm-gnu-toolchain/`，或設定 `DM_ARM_TOOLCHAIN_ROOT`；
- 若要跑 factory/source differential，另需 recovery Python dependencies 與本機 reference 資料。

不需要 ARMCC；正式基線是 Arm GNU toolchain、C11、Cortex-M4F hard-float。

## 第一次建置

```sh
make dm4310
```

輸出：

- `build/dm4310.elf`、`.map`、`.hex`、`.app.bin`；
- `build/package/dm4310/` 的 plain、encrypted、frames、manifest；
- `dist/development/dm4310_plain.bin` 與 `dm4310_enc.bin`。

全部 target：

```sh
make firmwares
```

這會建置九個 target、封裝 18 份最終 bin，並驗證 plain/ELF identity、manifest、AES round-trip 與
target 對應。正常 Makefile 不執行 recovery tests，也不把 test code 連進 firmware。

## 常用命令

```sh
make help
make build                         # 只建 ELF/app.bin
make dm8009                        # 單 target + package
make verify-firmware-outputs       # 重驗目前 dist/package
make send CAN_IF=can0              # 重建並傳送 DM4310 package frames
make provision-calibration         # 產生 calibration provisioning 資料
make clean                         # 刪除 build/ 與 dist/
```

`make send` 會改寫裝置 APP Flash，且要求裝置已在既有 loader；在有可用 recovery path 與備份前不要
執行。其他 target 若要傳送，直接指定其 frames：

```sh
python3 tools/send_update.py --interface can0 \
  --frames build/package/dm8009/app_update.frames.jsonl --yes
```

## 修改流程

1. 先從 [SOURCE_MAP.md](SOURCE_MAP.md) 找 ownership。
2. 確認是否觸及 fixed SRAM、persistent record、IRQ timing 或 hardware safety。
3. 做最小且語意化的修改；model差異集中 profile/capability。
4. 建最接近的 target；若碰共用邏輯或 layout，建九個 target。
5. 執行相稱的離線與硬體驗收。
6. 更新文件與 protocol／memory map。
7. 檢查 diff、產物 hash與工作樹後提交。

## Coding style 與結構

- C/H 使用 repository `.clang-format`。
- public function 在對應 header 有 prototype；private helper 用 `static`。
- 名稱描述產品語意，不使用 `FUN_XXXXXXXX`、反組譯地址或「其實共用卻帶 dm4310」的名稱。
- 不在共用 `.c` 散落 `DAMIAO_MODEL_*`；唯一 model選擇集中 `app_profile.h`。
- 小數若需 exact bit comparison，同時維護具語意的 float與 `_BITS` 常數。
- 註解說明原因、ABI或硬體限制，不逐行翻譯程式。
- 一般函式不固定 Flash地址；不要使用 padding、函式排序 trick或 factory `.incbin`。
- `_Static_assert` 僅用於真實 ABI/persistent contract；private C struct保留演進空間。

格式化前先看 `.clang-format-ignore`，不要把 third-party、generated或刻意固定 ASM當成一般 C 重排。

## 驗證選擇表

| 變更 | 最低離線驗證 | 額外驗證 |
|---|---|---|
| 文件 | `tools/check_markdown_links.py`、`git diff --check` | 人工核對命令／路徑 |
| 普通非即時 C | 受影響 target build | 對應 unit/differential |
| profile | 九 target build、source architecture | 該 target差分、persistent default行為 |
| CAN/UART/parser | build + protocol regressions | 實機 trace、malformed/timeout |
| control/math | full regressions | 長序列、IRQ timing、低壓限流 |
| board register | image layout + regressions | 無功率 logic/scope |
| linker/fixed SRAM/startup | 九 target layout + startup copies + full regressions | warm/cold boot、HardFault診斷 |
| Flash/update | package verifier | 斷電、loader ACK、SWD recovery |

典型完整離線檢查：

```sh
git diff --check
python3 tools/check_markdown_links.py
python3 tools/recovery/verify_source_architecture.py
make firmwares

for model in dm10010 dm3507 dm3507_48v dm4310 dm4310_48v \
             dm4340 dm4340_48v dm8006 dm8009; do
    python3 tools/recovery/verify_dm4310_image_layout.py --model "$model" || exit
    python3 tools/recovery/verify_dm4310_full_regressions.py --model "$model" || exit
done
```

Recovery 工具依賴本機 reference／Python packages 時，缺少依賴應明確記錄為「未執行」，不可只跑
build 後宣稱 differential PASS。

## 大小與效能

正式基線 `-Os`，不要在 source 塞 attribute 到處個別最佳化。比較其他最佳化等級時使用另一個
build directory：

```sh
cmake -S . -B build-o2 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-toolchain.cmake \
  -DFIRMWARE_SIZE_OPTIMIZATION=-O2
cmake --build build-o2 --target dm4310 -j
```

評估時同時比較 Flash、固定 SRAM slot、stack、20 kHz worst-case timing與差分結果。不要只看
`arm-none-eabi-size` 的單一 total。

## 測試碼放置

復原、differential、host harness 放 `tools/recovery/`，一般 Python工具放 `tools/`。測試可讀 product
ELF/source，但產品 CMake target不得讀 reference binary或 link測試 object。不要把唯一重要分析腳本
放 `/tmp`；可重現工具與必要 tables應留在 repository。

## 提交內容

提交 source與文件，不提交 `build*/`、`dist/`、toolchain、venv、dumped factory binary。提交訊息應
說明功能與驗收；若硬體 gate尚未做，明確列出，不用「完成」掩蓋。

任何會改外部 protocol、persistent format、target identity或安全限制的 commit，都應附 migration
或 compatibility說明。
