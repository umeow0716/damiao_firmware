SHELL := /bin/sh

PYTHON ?= python3
CMAKE ?= cmake
BUILD_DIR ?= build
DIST_DIR ?= dist
DIST_DEV := $(DIST_DIR)/development
TOOLCHAIN := cmake/arm-none-eabi-toolchain.cmake
CAN_IF ?= can0
.DEFAULT_GOAL := firmwares

DM4310_APP_BIN := $(BUILD_DIR)/dm4310.app.bin
DM4340_APP_BIN := $(BUILD_DIR)/dm4340.app.bin
DM8009_APP_BIN := $(BUILD_DIR)/dm8009.app.bin
PACK_ROOT := $(BUILD_DIR)/package

.PHONY: help all configure build firmwares dm4310 dm4340 dm8009 verify-firmware-outputs clean send provision-calibration

help:
	@echo "DM firmware source-build targets:"
	@echo "  make                  Build source APP and emit all model firmware images"
	@echo "  make firmwares        Same as make"
	@echo "  make dm4310           Emit only dist/development/dm4310_plain.bin + dm4310_enc.bin"
	@echo "  make dm4340           Emit only dist/development/dm4340_plain.bin + dm4340_enc.bin"
	@echo "  make dm8009           Emit only dist/development/dm8009_plain.bin + dm8009_enc.bin"
	@echo "  make build            Build all profiled source APP targets"
	@echo "  make verify-firmware-outputs  Check generated files without cross-model equality"
	@echo "  make clean            Remove build and dist outputs"
	@echo "  make send CAN_IF=can0 Send dist/development/dm4310_enc.bin through the loader"
	@echo ""
	@echo "Toolchain: locked by cmake/arm-none-eabi-toolchain.cmake"
	@echo "Default toolchain root: tools/arm-gnu-toolchain"
	@echo "Override only when needed: DM_ARM_TOOLCHAIN_ROOT=/path/to/arm-gnu-toolchain make"
	@echo ""
	@echo "Current DM8009 behavior: source APP target tracks the recovered V3/V6417 factory profile."
	@echo "Bootloader source is intentionally not built or tracked in this app-only workspace."
	@echo "No DM4310-vs-DM8009 byte-equality check is enforced; the two models may diverge during development."

all: firmwares

configure:
	$(CMAKE) -S . -B $(BUILD_DIR) \
		-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN) \
		-DCMAKE_BUILD_TYPE=RelWithDebInfo

build: configure
	$(CMAKE) --build $(BUILD_DIR) --target dm4310 dm4340 dm8009 -j

$(DIST_DEV):
	mkdir -p $(DIST_DEV)

$(PACK_ROOT)/dm4310 $(PACK_ROOT)/dm4340 $(PACK_ROOT)/dm8009:
	mkdir -p $@

dm4310: configure $(DIST_DEV) $(PACK_ROOT)/dm4310
	$(CMAKE) --build $(BUILD_DIR) --target dm4310 -j
	$(PYTHON) tools/pack_update.py --app $(DM4310_APP_BIN) \
		--profile config/update_profile.json --output-dir $(PACK_ROOT)/dm4310 \
		--purpose development \
		--plain-name dm4310_plain.bin \
		--encrypted-name dm4310_enc.bin
	cp $(PACK_ROOT)/dm4310/dm4310_plain.bin $(DIST_DEV)/dm4310_plain.bin
	cp $(PACK_ROOT)/dm4310/dm4310_enc.bin $(DIST_DEV)/dm4310_enc.bin
	@echo "wrote $(DIST_DEV)/dm4310_plain.bin"
	@echo "wrote $(DIST_DEV)/dm4310_enc.bin"

dm4340: configure $(DIST_DEV) $(PACK_ROOT)/dm4340
	$(CMAKE) --build $(BUILD_DIR) --target dm4340 -j
	$(PYTHON) tools/pack_update.py --app $(DM4340_APP_BIN) \
		--profile config/update_profile.json --output-dir $(PACK_ROOT)/dm4340 \
		--purpose development \
		--plain-name dm4340_plain.bin \
		--encrypted-name dm4340_enc.bin
	cp $(PACK_ROOT)/dm4340/dm4340_plain.bin $(DIST_DEV)/dm4340_plain.bin
	cp $(PACK_ROOT)/dm4340/dm4340_enc.bin $(DIST_DEV)/dm4340_enc.bin
	@echo "wrote $(DIST_DEV)/dm4340_plain.bin"
	@echo "wrote $(DIST_DEV)/dm4340_enc.bin"

dm8009: configure $(DIST_DEV) $(PACK_ROOT)/dm8009
	$(CMAKE) --build $(BUILD_DIR) --target dm8009 -j
	$(PYTHON) tools/pack_update.py --app $(DM8009_APP_BIN) \
		--profile config/update_profile.json --output-dir $(PACK_ROOT)/dm8009 \
		--purpose development \
		--plain-name dm8009_plain.bin \
		--encrypted-name dm8009_enc.bin
	cp $(PACK_ROOT)/dm8009/dm8009_plain.bin $(DIST_DEV)/dm8009_plain.bin
	cp $(PACK_ROOT)/dm8009/dm8009_enc.bin $(DIST_DEV)/dm8009_enc.bin
	@echo "wrote $(DIST_DEV)/dm8009_plain.bin"
	@echo "wrote $(DIST_DEV)/dm8009_enc.bin"

firmwares: dm4310 dm4340 dm8009
	$(MAKE) verify-firmware-outputs
	@echo "wrote all model firmware images to $(DIST_DEV)"

verify-firmware-outputs:
	$(PYTHON) tools/verify_firmware_outputs.py --dist $(DIST_DEV) \
		--build-dir $(BUILD_DIR) --package-root $(PACK_ROOT) \
		--profile config/update_profile.json

send: dm4310
	$(PYTHON) tools/send_update.py --interface $(CAN_IF) \
		--encrypted $(DIST_DEV)/dm4310_enc.bin --yes

provision-calibration:
	$(PYTHON) tools/provision_calibration.py \
		--output-dir $(DIST_DIR)/calibration

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)
