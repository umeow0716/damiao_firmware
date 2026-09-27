# DM firmware workspace

## Normal build

This workspace has one normal build path:

```sh
make
```

It compiles the source-owned APP with the locked ARM GNU toolchain, then emits
both development firmware sets:

```text
dist/development/dm4310_plain.bin
dist/development/dm4310_enc.bin
dist/development/dm8009_plain.bin
dist/development/dm8009_enc.bin
```

There is no `make package` target.  `make` directly produces the firmware
artifacts.

## Current DM8009 behavior

DM8009 is currently built from the same source APP as DM4310.  Because no
DM8009-specific source changes have been restored yet, the generated DM4310 and
DM8009 artifacts are intentionally byte-identical.

The build enforces this with `cmp` after both images are produced.

## No reference plaintext path

The normal build does **not** use decrypted reference plaintext images.  It uses
only the compiled source APP plus `config/update_profile.json` for AES-256-CTR
packaging.

## Toolchain

The default/pinned toolchain root is:

```text
tools/arm-gnu-toolchain
```

The CMake toolchain file resolves compilers from:

```text
tools/arm-gnu-toolchain/bin/arm-none-eabi-*
```

If the same toolchain is stored outside the checkout, override explicitly:

```sh
DM_ARM_TOOLCHAIN_ROOT=/path/to/arm-gnu-toolchain make
```

## Available targets

```sh
make           # build both DM4310 and DM8009 firmware images
make firmwares # same as make
make dm4310    # build only DM4310 outputs
make dm8009    # build only DM8009 outputs, currently identical to DM4310
make build     # compile source APP only
make clean     # remove build and dist outputs
```

## Removed branches

The old `tests/` tree, `static-audit` branch, historical exact-reference build,
reference-plaintext packaging path, and `make package` target were removed from
this workspace.
