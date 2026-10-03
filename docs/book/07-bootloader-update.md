# 7. 既有 bootloader 與更新映像

本工作區不建置 bootloader，但 APP 必須正確維護 boot record、能要求 reset 回 loader，並輸出既有
loader 可接受的加密 payload。這個邊界不能與 APP source recovery 混為一談。

## plain、encrypted 與 frames

| 產物 | 用途 | 可否直接寫 `0x20000` |
|---|---|---|
| `<model>_plain.bin` | debugger／programmer 的 APP 明文 | 可以 |
| `<model>_enc.bin` | AES-256-CTR ciphertext，供既有 loader 解密 | 不可以 |
| `app_update.frames.jsonl` | 已含 chunk header/CRC 的 SocketCAN frame 清單 | 由 sender 傳送 |
| `manifest.json` | hash、大小、vector、chunk、profile provenance | 不刷入 |

`dist/development/<variant>/` 分別保留四種行為版本的 plain/encrypted 最終映像；完整 package
metadata 與 frames 位於 `build/package/<variant>/<model>/`。variant 為 `factory`、`raw`、
`no_response` 或 `raw_no_response`。

## APP partition 與 vector validation

packer 在加密前確認：

- image 非空且不超過 `0x00020000..0x0002FFFF`；
- vector[0] 是合理、對齊且位於 SRAM 的 MSP；
- vector[1] 是 Thumb reset address，位於 APP partition；
- profile 的 `app_base/app_end/chunk_size/key/counter` 格式有效。

因此 `_enc.bin` 不是任意檔案加密後改副檔名；來源必須是正確 link address 的 APP binary。

## 加密與 chunk contract

`config/update_profile.json` 是現行契約：

- AES-256-CTR；counter 以 128-bit big-endian 整數遞增；
- ciphertext 與 plaintext 同長；
- chunk 上限 `0x2000`（8192）bytes；
- sequence 從 `chunk_count - 1` 遞減到 0；
- 每 chunk 使用 CRC-8/MAXIM，計算範圍是 ciphertext chunk；
- record 為 `# sequence # uint16_le(length) ciphertext crc8`；
- CAN update ID `0x7FF`、status ID `0x7FE`；目前 transport profile 為 1 Mbit/s arbitration、
  5 Mbit/s data、CAN FD + BRS。

key/counter 已在 profile 中版本化並帶有來源 loader SHA-256。若未來更新 loader 或格式，建立新
profile 與 migration plan；不要靜默覆寫舊 profile 讓同一名稱代表不同協定。

## 封裝流程

`make dm4310` 實際做：

```text
C/ASM → dm4310.elf
      → objcopy dm4310.app.bin
      → validate vectors / partition
      → AES-CTR + chunk + CRC + frames + manifest
      → dist/development/factory/dm4310_plain.bin
      → dist/development/factory/dm4310_enc.bin

C/ASM + raw feedback define → dm4310_raw.elf
      → 同一套 validate／封裝流程
      → dist/development/raw/dm4310_plain.bin
      → dist/development/raw/dm4310_enc.bin

C/ASM + no-response define → dm4310_no_response.elf
C/ASM + raw + no-response  → dm4310_raw_no_response.elf
      → 各自輸出到同名 variant 目錄
```

`make firmwares` 完成九型號的四種 target 後，`tools/verify_firmware_outputs.py` 會分別核對：
ELF objcopy 與 plain 逐 byte 相同、plain/enc 大小、variant manifest、AES round-trip 與 source
identity。它不要求不同型號或不同 variant 互相 byte-equal。

## APP 端 loader handoff

UART `X` 或指定 CAN legacy request 會：

1. 更新 boot record 為 request-update／unconfirmed 狀態；
2. flush 必要回覆；
3. 延遲並 reset，交回既有 loader。

APP 啟動早期則確認 application 並寫入 target-specific identity。這些寫入順序決定 update 後 warm
reset 是否被 loader 接受，不應和一般 configuration sector 操作合併。

## 傳輸與錯誤處理

sender 逐 chunk pacing frame，等待 `0x7FE` 回覆：

- `CRCERROR`、`TimERROR`：只拒絕目前 chunk，可以重送同一 ciphertext chunk；
- `EFMERROR`、`APPERROR`：視為不可安全重試，立即停止；
- 最後一包除 `OK!` 外還要等待 `complete`。

傳送前必須確認裝置已停在 loader、CAN interface timing 正確，且已有 SWD／ROM ISP 復原途徑。
不要在一般運轉中的 APP 上盲目送 update frames。

## 安全更新檢查表

1. 記錄 target、板號、目前 boot record 與所有 calibration/config sector backup。
2. 建置指定 target，保存 plain/enc/manifest hash。
3. programmer 使用 plain；loader transport 使用 frames/encrypted，兩者不可互換。
4. 傳輸時保存完整 `0x7FE` trace，不只記「成功」。
5. reset 後讀 APP vectors、boot record，確認 target identity 與通訊。
6. 再檢查 configuration、zero、兩套 calibration 沒被 APP update 擦除。

完整格式另見 [UPDATE_FORMAT.md](../UPDATE_FORMAT.md)。

[上一章：通訊協定](06-protocols.md) · [下一章：安全與 target](08-safety-profiles.md)
