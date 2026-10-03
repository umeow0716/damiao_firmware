# 9. 建置、驗證與除錯

## Toolchain 與 flags

預設使用 `tools/arm-gnu-toolchain/` 的 Arm GNU toolchain；可用環境變數覆寫：

```sh
DM_ARM_TOOLCHAIN_ROOT=/opt/arm-gnu-toolchain make dm4310
```

重要編譯契約：C11、Cortex-M4、Thumb、FPv4-SP-D16、hard-float、全域 `-Os`、
`-ffp-contract=off`、function/data sections、`--gc-sections`，以及大量 warning + `-Werror`。

`-Os` 是目前尺寸、速度與差分驗證的平衡基線。只有 `safety_update()`、`output_atan2f()` 與
`svpwm_result()` 經量測後以 `DAMIAO_OPTIMIZE_SPEED` 使用函式級 `-O2`；全域 `-O2` 會使 DM4310
text 增加約 4 KiB。可用 CMake cache 做實驗 build，但切換 `-Oz/-O2/-O3` 或增加函式級例外後，
必須重新評估 binary size、IRQ timing、stack、固定 kernel size 與長序列浮點行為，不能只因能編譯
就改成正式設定。

## 日常命令

```sh
make                    # 九型號 × 四種 variant + plain/enc + output verification
make dm4310             # 單一型號的四種 variant package
make build              # 只建 36 個 ELF/app.bin，不複製 dist package
make verify-firmware-outputs
make help
```

`make` 不會編入或執行 recovery tests。這是刻意的隔離：日常開發可以改功能，不會被 factory 地址
或差分框架綁死；需要驗收時再明確執行下一節的工具。

## 產物位置

```text
build/<model>.elf              factory symbol、DWARF、真實 VMA/LMA
build/<model>_raw.elf          raw-feedback variant
build/<model>_no_response.elf  filtered/no-response variant
build/<model>_raw_no_response.elf  raw/no-response variant
build/<model><suffix>.app.bin  各 ELF objcopy 的 APP image
build/package/<variant>/<model>/  plain、enc、frames、cansend、manifest
dist/development/<variant>/    九型號最終 plain/enc；共四種 variant
```

目前四種 variant 的 plain 為 54,096–54,220 bytes，不能把這個範圍硬寫成 build gate；真正 gate 是
64 KiB partition、linker ASSERT 與每次輸出的實際 free symbol。

## 離線驗收層級

### 1. Source architecture

```sh
python3 tools/recovery/verify_source_architecture.py
```

檢查九個 profile 隔離、共用 V3 命名、無逆向式 `FUN_*`／model-named shared source，以及 recovery
工具未加入產品 build。

### 2. Image layout

```sh
python3 tools/recovery/verify_dm4310_image_layout.py --model dm4310
```

檢查 vector、64 KiB Flash、固定 SRAM VMA/LMA、section overlap、heap/stack、initialized copy 與 BSS。
`--model` 可換成任一支援 target。

### 3. Factory/source differential

```sh
python3 tools/recovery/verify_dm4310_full_regressions.py --model dm4310
```

執行該 target 的 65 項持久化差分。工具直接比較 factory machine code 與 source ELF 在已定義輸入下
的 register、SRAM、MMIO、FPSCR 與順序；它不是函式 hash 比對。

完整九型號驗收可用：

```sh
for model in dm10010 dm3507 dm3507_48v dm4310 dm4310_48v \
             dm4340 dm4340_48v dm8006 dm8009; do
    python3 tools/recovery/verify_dm4310_image_layout.py --model "$model" || exit
    python3 tools/recovery/verify_dm4310_full_regressions.py --model "$model" || exit
done
```

### 4. Package identity

`make firmwares` 最後自動執行 `verify_firmware_outputs.py`，核對每個 plain 是對應 ELF 的 objcopy、
encrypted 可 round-trip、manifest/hash/大小/variant 一致，而且各 target 沒有錯配。raw feedback 的
資料來源另以獨立工具驗證，不加入正常 Make 流程：

```sh
.venv/bin/python tools/recovery/verify_feedback_variants.py --model dm4310
```

## 看大小與記憶體

```sh
tools/arm-gnu-toolchain/bin/arm-none-eabi-size build/dm4310.elf
tools/arm-gnu-toolchain/bin/arm-none-eabi-objdump -h build/dm4310.elf
tools/arm-gnu-toolchain/bin/arm-none-eabi-nm -S --size-sort build/dm4310.elf | tail
rg '__flash_free_bytes__|__app_state_free_bytes__|__source_bss_free_bytes__' build/dm4310.map
```

`.bin` 末端由最後一個 loadable section 決定；`size` 的 data/bss 分類與實際 Flash load bytes不完全
相同。調查容量時以 linker map、section VMA/LMA 與 free symbol 一起判斷。

## 除錯方向

| 症狀 | 先查 |
|---|---|
| 所有 baud／period 都不對 | clock tree、XTAL、PLL ready、SystemCoreClock |
| 開機立刻 HardFault | vector/MSP、copy-down、hard-float ABI、固定 SRAM overlap |
| CAN 收不到但 UART 正常 | MCAN clock、PB6/PB7、selector、filter、message RAM、IRQ3 |
| 有角度但控制抖動 | motor/output sensor 分流、direction、electrical offset、phase order |
| reset 後參數消失 | WRITE vs STORE、deferred event、0x3E000 sector readback |
| `_plain.bin` 正常、loader 更新失敗 | 把 plain/enc/frames 用途混淆、CAN rate、boot record、chunk ACK |
| 只在某兩型號壞 | SRAM layout selector或 profile constant，而非一般共用邏輯 |

## 提交前最低檢查

```sh
git diff --check
python3 tools/check_markdown_links.py
python3 tools/recovery/verify_source_architecture.py
make firmwares
```

控制、IRQ、linker、profile 或固定 SRAM 有變更時，再跑九型號 image-layout 與 differential；硬體相關
變更還要依第 11 章上板。

[上一章：安全與 target](08-safety-profiles.md) · [下一章：修改實例](10-change-recipes.md)
