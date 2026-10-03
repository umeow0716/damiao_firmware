# DM V3 recovery verification

這個目錄保存不屬於一般韌體 build 的 factory/source 差分驗證器。腳本不會由 CMake 或 make
自動執行，也不會把 factory binary 連結進產品映像。

型號 profile、共用 V3 source 與 recovery-only 工具的隔離規則可獨立檢查：

```sh
python3 tools/recovery/verify_source_architecture.py
```

先完成目前的 firmware build，再於 repository root 執行。若分析環境不存在，可重建在任意
位置；套件版本保存在 `tools/recovery/requirements.txt`：

```sh
python -m venv .venv
.venv/bin/python -m pip install -r tools/recovery/requirements.txt
```

永久保存的 65 支全域差分驗證可依型號執行：

```sh
.venv/bin/python tools/recovery/verify_dm4310_full_regressions.py --model dm4310
```

`--model` 支援 `dm10010`、`dm3507`、`dm3507_48v`、`dm4310`、`dm4310_48v`、
`dm4340`、`dm4340_48v`、`dm8006` 與 `dm8009`。每個型號都必須對自己的 factory
reference 執行，不能以跨型號 byte equality 代替驗證。

較慢但使用整理後共用 loader 的核心流程驗證可另外執行：

```sh
.venv/bin/python tools/recovery/verify_dm4310_core_regressions.py --model dm4310
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
.venv/bin/python tools/recovery/verify_dm4310_image_layout.py --model dm4310
```

已停止的 DM4310 factory byte-exact 歷史報告（不屬於完成閘門）：

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
`/tmp`。九型號完成狀態與最終閘門記錄於 `docs/FIRMWARE_RECOVERY_PROGRESS.md`；
DM4310 的逐項證據另見 `docs/DM4310_RECOVERY_PROGRESS.md`。
