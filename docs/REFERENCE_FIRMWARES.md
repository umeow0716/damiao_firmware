# 參考韌體目錄與完整性

最後更新：2026-10-03

`reference/` 只保存官方更新檔與其解密副本，不參與產品建置。一般 `make`／CMake 不得連結、
嵌入或複製這些 binary；它們只供反組譯、差分與獨立 recovery regression 使用。

## 目錄規則

```text
reference/
├── V3/   # V3、V3 48V；目前九個 source-recovery target
└── V4/   # V4 研究樣本；目前不屬於產品建置 target
```

每份官方密文使用原檔名 `*.bin`，解密副本在相同目錄使用 `*.decrypted.bin`。不再以
`_decrypted.bin`、省略 `(V3)` 或根目錄散放等多種命名方式表示同一件事。

## V3 清單

| 適用型號 | 官方版本 | bytes | encrypted SHA-256 | decrypted SHA-256 |
|---|---:|---:|---|---|
| DM4310 | 5017.04 | 51,284 | `61f049d7f5da1a918af8fc8d47af1d3ba060e80ccc344f41c82859a09576817c` | `65aab219268e9159b196d4578d3cd530e6fa90a731a82670a0d3d3be609b59d4` |
| DM4340 | 5117.04 | 51,284 | `c7b51cb37ba51353e5d755f275ee0b7e4ec4b2afc4e7eb4e91ee2d0d8a60d1ff` | `182ab855cb072a694778f6abe62e7953730648a170c010d73318d75255a06a79` |
| DM10010 | 5617.04 | 51,292 | `87e3f8b9fe61a56455718068f12d45cfa4d3692a85f7639a81615a56631352c2` | `700e4857a5224bf0c6d65fa7c5cee301572e775d7e3d45c875b18c74d696dea5` |
| DM3507 | 5717.04 | 51,284 | `66c5f656e56499e55a926e6c11aeaad6b381c0214caf87f9be580132d081a933` | `b734f12048feabf506e4fe8bf854d6c478572d2c8881aef31eee21064eea3009` |
| DM4310 48V | 6017.04 | 51,292 | `2ad03f2a474bc3abc5229d62fa12f67fb3a17a2a017c14cc52f51fcf19fc0dbd` | `06794f5a74372bbf57a034d40271007bee673f331aad2252fb323f9398dae088` |
| DM4340 48V | 6117.04 | 51,300 | `decc1f651f4c57952b4846abdc86adbbd74f1518b89a23bd2a826f21dd4bd556` | `a7b7d0954d05654cc3af18189931ceb99a662173f6680d03ad4e9bb7a09b90c2` |
| DM8006 | 6317.04 | 51,348 | `a111c525481ba5a9d1ec0316040af9012c131152cf52da01a95d818ec50c54ee` | `3229bb43ef357f83a95ce009f8e38db7e03fcecf7386b4b4124a8260444cb769` |
| DM8009 | 6417.04 | 51,348 | `80aa64b88936bafc24800f2d75defb4864161c62eaec50b488d373d40c1fe3d1` | `ef4586d0c684bf052a3119dea215e3bda761525a88d3ef8698b6478d0de4f270` |
| DM3507 48V | 6517.04 | 51,292 | `94fc0326b6063bd8123446553e71f150dcf8ddaa039652d30f77dbac1c7ac47a` | `81621d438189571a79d9f825f88ddb56502c3620dc3325c7ba9075dbb741dbda` |

## V4 清單

| 適用型號 | 官方版本 | bytes | initial MSP | encrypted SHA-256 | decrypted SHA-256 |
|---|---:|---:|---:|---|---|
| DM4310 | 7018.04 | 58,796 | `0x1ffffdd0` | `a0d3a352be076a9bc0efffc94fb92e1850d0d67e7af43d988d362d414b842ff8` | `26d6e4ffeefb611aab8a20c726cdab88c1e9b75df2f6c5da56334f6863e93fca` |
| DM4310 | 7038.02 | 55,508 | `0x1ffff8d0` | `f7dcd69e78f773c4fc8733fea29e2b404c19b78de7b7447e683d685f98786e29` | `05f974e23dcbe9ab102e02645849a7ba95ece8e3eb9e1bb5ffe9c7d56051d5af` |
| DM4340 | 7060.02 | 55,508 | `0x1ffff8d0` | `3a398cbbb11180ca9b619c14f82ce1ab6dbdaf3e780aa3c3dd6306c94be17e7b` | `2f85ad6d1bf06171efc4a6d9526d19d25353ff5cad83ec8a94c1ae5fd331ce2f` |

三份 V4 的 reset vector 都是 `0x00020389`。解密時已同時驗證 Cortex-M vector table，以及
將 plaintext 重新 AES-256-CTR 加密後逐 byte 等於原始 ciphertext。

## 重新產生 decrypted 副本

更新格式的 key、counter 與限制由 source-owned profile 提供：

```sh
python3 tools/decrypt_factory_update.py \
  --profile config/update_profile.json \
  --encrypted 'reference/V4/APP_DM4310(V4)_V7038_02.bin' \
  --output 'reference/V4/APP_DM4310(V4)_V7038_02.decrypted.bin'
```

工具只在 vector 合法且 AES round-trip 成功後寫檔。若有已經 hash-pinned 的 bootloader，也可將
`--profile` 改成 `--bootloader <path>`；兩種 key source 不可同時指定。

`--factory-config-output` 是既有 DM4310 V3 V5017.04 專用功能，會先核對 plaintext SHA-256；
V4 的 scatter layout 不同，工具會拒絕以 V3 位址誤切 V4 設定資料。

## 變更 reference 時的最低檢查

1. 保留官方密文，不得只留下 decrypted 檔。
2. 依硬體世代放入 `V3/` 或 `V4/`，檔名不自行改寫版本資訊。
3. 以 `decrypt_factory_update.py` 產生 plaintext，不接受跳過 vector 或 round-trip 的結果。
4. 用 `sha256sum reference/V3/*.bin reference/V4/*.bin` 更新本頁。
5. 若搬動檔案，搜尋所有 recovery script 的 `reference/` 路徑並跑完整回歸。
6. 新參考檔不會自動變成 source-recovery target；是否納入必須另行更新 target profile、差分器與進度表。

版本與算法差異請接續閱讀 [V3/V4 實作分析](FIRMWARE_VERSION_ANALYSIS.md)。
