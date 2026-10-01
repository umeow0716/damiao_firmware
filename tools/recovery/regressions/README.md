# DM4310 persistent differential regressions

本目錄保存從全域掃描建立的 factory/source 行為差分驗證器。驗證器會讀取 factory reference
與目前 source-built ELF，但不會把 factory binary 連結進產品映像，也不屬於一般 CMake/make
流程。

請從 repository root 執行全組驗證：

```sh
python tools/recovery/verify_dm4310_full_regressions.py
```

個別腳本可能利用同目錄中的另一支 verifier 建立測試 harness；因此搬移或改名時必須重跑完整
驗證組。完整驗證組目前應找到 65 支 `dm4310_*_verify.py`。
