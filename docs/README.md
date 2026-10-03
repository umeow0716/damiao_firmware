# 韌體文件總覽

這裡的正式文件以「能繼續開發」為目標：先說明系統邊界，再提供原始碼地圖、修改入口、驗證
方法與不可跨越的硬體／ABI 限制。若舊的復原報告與本頁或 `book/` 不一致，日常開發以目前
source、target profile、linker 與本套手冊為準；舊報告只代表當時的分析快照。

## 我現在該看哪一份？

| 目的 | 建議入口 |
|---|---|
| 第一次接手專案 | [完整韌體手冊](book/README.md) → [系統架構](book/01-architecture.md) |
| 想知道某個功能在哪個檔案 | [原始碼地圖](SOURCE_MAP.md) |
| 改一項功能，不確定會牽動什麼 | [常見修改實例](book/10-change-recipes.md) |
| 建置、封裝、檢查輸出 | [開發與建置](DEVELOPMENT.md) |
| 改 target 常數或新增型號 | [安全、fault 與 target profile](book/08-safety-profiles.md) |
| 改 CAN／UART／參數 | [通訊協定](book/06-protocols.md) |
| 改控制器、FOC 或 PWM | [馬達控制](book/05-motor-control.md) |
| 改 linker、SRAM 物件或 IRQ | [startup 與記憶體](book/02-startup-memory.md) |
| 刷機或理解 `.enc.bin` | [更新格式](UPDATE_FORMAT.md) |
| 查官方版本編碼、V3/V4 差異 | [參考韌體目錄](REFERENCE_FIRMWARES.md) → [V3/V4 實作分析](FIRMWARE_VERSION_ANALYSIS.md) |
| 第一次上板 | [上板驗收清單](PORTING_CHECKLIST.md) |
| 查 source 與 factory 的復原證據 | [復原狀態](FIRMWARE_RECOVERY_PROGRESS.md) |

## 正式開發文件

- [原始碼地圖](SOURCE_MAP.md)：逐層、逐檔案說明責任與主要函式。
- [開發與建置](DEVELOPMENT.md)：toolchain、命令、產物、大小與提交 gate。
- [更新格式](UPDATE_FORMAT.md)：APP partition、AES-CTR payload 與傳輸注意事項。
- [參考韌體目錄](REFERENCE_FIRMWARES.md)：`reference/V3`、`reference/V4` 的檔案、雜湊與解密方法。
- [V3/V4 實作分析](FIRMWARE_VERSION_ANALYSIS.md)：版本編碼、校正、馬達辨識與即時控制差異。
- [上板驗收清單](PORTING_CHECKLIST.md)：硬體 bring-up 的安全順序。
- [完整韌體手冊](book/README.md)：以下 12 章的完整導覽。

## 完整手冊章節

1. [架構與執行流程](book/01-architecture.md)
2. [startup、記憶體與固定 SRAM ABI](book/02-startup-memory.md)
3. [板級周邊](book/03-board-peripherals.md)
4. [感測器、校正與持久化資料](book/04-sensors-calibration.md)
5. [馬達控制與即時資料流](book/05-motor-control.md)
6. [CAN、參數與 UART 協定](book/06-protocols.md)
7. [既有 bootloader 與更新映像](book/07-bootloader-update.md)
8. [安全、fault 與 target profile](book/08-safety-profiles.md)
9. [建置、驗證與除錯](book/09-build-test-debug.md)
10. [常見修改實例](book/10-change-recipes.md)
11. [上板驗收與故障復原](book/11-validation-recovery.md)
12. [證據、完成度與已知邊界](book/12-evidence-status.md)

## 文件的證據等級

閱讀任何「已完成」敘述時，請辨識它屬於哪一層：

| 等級 | 能證明什麼 | 不能取代什麼 |
|---|---|---|
| 編譯／linker | 語法、ABI、section、容量與 undefined symbol | 控制行為、硬體波形 |
| factory/source 差分 | 指定輸入下的暫存器、SRAM、FPSCR、MMIO 與順序相符 | 所有未探索輸入、實體電氣差異 |
| host 測試 | 純演算法或 parser 的可重現輸入輸出 | IRQ timing、ADC 雜訊、功率級 |
| 無功率實機 | clock、腳位、通訊、感測與 PWM 波形 | 帶載穩定性與熱安全 |
| 低壓限流／帶載 | 真實相序、電流迴路、fault 時序與負載行為 | 未測硬體 revision 或工作範圍 |

目前九型號已完成 source recovery、離線差分、映像 layout 與封裝驗證；實機功率級與不同硬體
revision 的安全驗收仍必須由實際設備完成。

## 歷史與復原資料

本機可能另有 `DUMP_ANALYSIS.md`、`FULL_APP_SCAN.md`、`EQUIVALENCE_REPORT.md` 等舊報告。它們
對追查反組譯依據很有價值，但可能包含舊 target 名稱、舊 Makefile 命令、舊映像大小或已撤除的
source bootloader。這些文件不屬於現行建置契約；要判斷目前狀態，依序看：

1. 現在的 source／linker／target profile；
2. 本套正式開發手冊；
3. `FIRMWARE_RECOVERY_PROGRESS.md` 的最新驗收結果；
4. 最後才回查歷史報告與 `reference/`／`recovered/` 證據。
