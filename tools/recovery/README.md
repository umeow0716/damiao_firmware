# DM4310 recovery verification

這個目錄保存不屬於一般韌體 build 的 factory/source 差分驗證器。腳本不會由 CMake 或 make
自動執行，也不會把 factory binary 連結進產品映像。

先完成目前的 DM4310 build，再於 repository root 執行。若分析環境不存在，可重建在任意
位置；套件版本保存在 `tools/recovery/requirements.txt`：

```sh
python -m venv .venv
.venv/bin/python -m pip install -r tools/recovery/requirements.txt
```

永久保存的 65 支全域差分驗證可一次執行：

```sh
for model in dm4310 dm4340 dm8009; do
    .venv/bin/python tools/recovery/verify_dm4310_full_regressions.py --model "$model"
done
```

較慢但使用整理後共用 loader 的核心流程驗證可另外執行：

```sh
for model in dm4310 dm4340 dm8009; do
    .venv/bin/python tools/recovery/verify_dm4310_core_regressions.py --model "$model"
done
```

實際執行 Reset/SystemInit/scatter/runtime 到 main entry 的組合矩陣：

```sh
.venv/bin/python tools/recovery/verify_dm4310_startup_composition.py
```

在同一台模擬機保留上述啟動狀態，接續驗證 main 初始化呼叫 ABI 與第一輪 deferred loop：

```sh
.venv/bin/python tools/recovery/verify_dm4310_boot_main_composition.py
```

最終 ELF／binary 的 64 KiB、VMA/LMA、copy-down、zero-fill、RAMB 與 heap/stack 佈局：

```sh
for model in dm4310 dm4340 dm8009; do
    .venv/bin/python tools/recovery/verify_dm4310_image_layout.py --model "$model"
done
```

DM4310 factory byte-exact 進度（size、SHA、vector、191 個真實 Ghidra function body、
fixed-SRAM section 與 load-image 組成）：

```sh
.venv/bin/python tools/recovery/report_dm4310_byte_parity.py \
    --details-output build/recovery/dm4310_byte_parity_functions.tsv
```

這個報告直接使用
`recovered/dm4310/tables/factory_function_body_ranges.tsv` 保存的非連續 function body
address sets，不以 `entry + size` 猜測範圍。需要重新匯出 ranges 時使用
`DumpFunctionBodyRanges.java`；報告與匯出器都不屬於一般 CMake/make。

固定 IRQ000/IRQ001 的 timer、DMA、pointer-pool、SRAM/MMIO 與 FPSCR 矩陣：

```sh
.venv/bin/python tools/recovery/verify_dm4310_position_irqs.py
```

IRQ003 錯誤發布後直接銜接 main deferred iteration 的組合矩陣：

```sh
.venv/bin/python tools/recovery/verify_dm4310_irq_main_composition.py
```

IRQ002 outer-loop tick 及 IRQ004 命令發布後銜接 main 的組合矩陣：

```sh
.venv/bin/python tools/recovery/verify_dm4310_adc_main_composition.py
.venv/bin/python tools/recovery/verify_dm4310_uart_main_composition.py
```

Python venv 可以重建；測試邏輯、factory 路徑與共用 Unicorn loader 均保存在本目錄，不依賴
`/tmp`。三型號完成狀態與最終閘門記錄於 `docs/FIRMWARE_RECOVERY_PROGRESS.md`；
DM4310 的逐項證據另見 `docs/DM4310_RECOVERY_PROGRESS.md`。
