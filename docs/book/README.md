# 達妙 V3 韌體開發手冊

本手冊描述目前工作區中九個 V3 sub-version 04 APP target。它不是反組譯函式清單，也不要求一般
C 函式固定在 factory 位址；目標是讓工程師知道每層負責什麼、怎麼安全修改，以及修改後要用
什麼證據證明沒有破壞既有行為。

## 閱讀路線

第一次接手建議依序讀：

1. [架構與執行流程](01-architecture.md)
2. [startup、記憶體與固定 SRAM ABI](02-startup-memory.md)
3. [板級周邊](03-board-peripherals.md)
4. [感測器、校正與持久化資料](04-sensors-calibration.md)
5. [馬達控制與即時資料流](05-motor-control.md)
6. [CAN、參數與 UART 協定](06-protocols.md)
7. [既有 bootloader 與更新映像](07-bootloader-update.md)
8. [安全、fault 與 target profile](08-safety-profiles.md)
9. [建置、驗證與除錯](09-build-test-debug.md)
10. [常見修改實例](10-change-recipes.md)
11. [上板驗收與故障復原](11-validation-recovery.md)
12. [證據、完成度與已知邊界](12-evidence-status.md)

只想找檔案可直接看 [原始碼地圖](../SOURCE_MAP.md)；只想開始開發可看
[開發與建置](../DEVELOPMENT.md)。

## 全書共同假設

- APP Flash partition 是 `0x00020000..0x0002FFFF`，上限 64 KiB。
- 本工作區不建置 bootloader；`_enc.bin` 是交給裝置既有 loader 的 transport payload。
- 九個 target 共用產品邏輯，差異集中於 target profile 與固定 SRAM layout。
- `reference/`、`recovered/` 與 recovery verifier 是證據，不是產品 source dependency。
- 正常 `make` 不執行或連結 recovery tests。
- 離線差分完成不等於所有硬體 revision 都已完成帶載安全驗收。

## 維護文件時的規則

功能、target、位址、命令或驗收流程變更時，同一個 commit 至少更新對應章節與
[原始碼地圖](../SOURCE_MAP.md)。文件中的命令必須能在 repository 根目錄直接執行；歷史數字若不再
是穩定契約，應連結到自動產生的 map／progress，而不是複製一份日後會過時的值。
