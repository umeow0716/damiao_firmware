# APP 更新格式

現行契約由 `config/update_profile.json` 與 `tools/pack_update.py` 共同定義。來源 bootloader不在本
工作區；profile帶有其 provenance hash，不能在不改格式版本的情況下更換 key、counter或 framing。

## 兩種可刷映像

- `<model>_plain.bin`：APP明文，直接以 programmer/debugger寫到 `0x00020000`。
- `<model>_enc.bin`：AES-256-CTR ciphertext，只供既有 loader更新流程。

loader最後寫入 Flash的仍是明文 Cortex-M vector與程式。`_enc.bin` 直接寫入 `0x20000` 必然無法
正常啟動。

## Partition 與 validation

- APP base：`0x00020000`
- APP end（exclusive）：`0x00030000`
- 最大長度：65,536 bytes
- vector[0]：對齊且位於 HC32F448 SRAM
- vector[1]：Thumb address且位於 APP partition

## Cipher

- AES-256-CTR；加解密同一運算。
- 16-byte counter視為 128-bit big-endian integer，每 block加一。
- key與 initial counter讀自 versioned profile。
- ciphertext與 plain長度相同。

## Chunk record

最大 chunk為8192 bytes。sequence由 `chunk_count - 1` 遞減到0：

```text
offset  size        內容
0       1           0x23 ('#')
1       1           sequence
2       1           0x23 ('#')
3       2           ciphertext length，little-endian
5       length      ciphertext chunk
5+len   1           CRC-8/MAXIM(ciphertext chunk)
```

CRC是 reflected polynomial `0x8C`、initial 0。AES counter跨 chunk連續，不在每包重新初始化；因此
重送必須重送完全相同的 ciphertext chunk，不能重新從 initial counter單獨加密該包。

## CAN transport

- update standard ID：`0x7FF`
- status standard ID：`0x7FE`
- 現行 profile：1 Mbit/s nominal、5 Mbit/s data、CAN FD + BRS
- packer把每個 record切成最多8-byte frame並輸出 `app_update.frames.jsonl`

雖然 CAN FD可承載更長 payload，既有 loader framing使用最多8-byte資料片段；sender依 package輸出
原樣傳送，不應自行合併。

## Package 內容

```text
build/package/<model>/
├── <model>_plain.bin
├── <model>_enc.bin
├── app_update.frames.jsonl
├── app_update.cansend
└── manifest.json
```

manifest記錄 plain/encrypted SHA-256、大小、MSP、Reset Handler、chunk count、transport、profile與
provenance。`dist/development/` 僅複製最後的 plain/enc；實際 CAN sender要讀 package中的 frames。

## 發送

DM4310快捷 target：

```sh
make send CAN_IF=can0
```

其他 target：

```sh
make dm8009
python3 tools/send_update.py --interface can0 \
  --frames build/package/dm8009/app_update.frames.jsonl --yes
```

裝置必須已等待在 loader。sender逐 chunk等 `0x7FE` ACK；`CRCERROR`/`TimERROR`可重送目前 chunk，
`EFMERROR`/`APPERROR`立即停止，最後一包還需收到 `complete`。

## 變更格式時

1. 建立新的 profile name／format version，不覆寫舊契約。
2. 同步 loader、packer、sender與 round-trip/golden tests。
3. 定義舊裝置如何辨識與拒絕新格式。
4. 做中途斷電、chunk重送、wrong key/counter、CRC錯誤與最大映像測試。
5. 更新本文件與 manifest schema。
