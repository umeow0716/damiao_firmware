SHELL := /bin/sh

PYTHON ?= python3
CMAKE ?= cmake
BUILD_DIR ?= build
DIST_DIR ?= dist
DIST_DEV := $(DIST_DIR)/development
TOOLCHAIN := cmake/arm-none-eabi-toolchain.cmake
CAN_IF ?= can0
.DEFAULT_GOAL := firmwares

SOURCE_APP_BIN := $(BUILD_DIR)/damiao_app.bin
PACK_ROOT := $(BUILD_DIR)/package

.PHONY: help all configure build firmwares dm4310 dm8009 verify-firmware-outputs clean send provision-calibration

help:
	@echo "DM firmware source-build targets:"
	@echo "  make                  Build source APP and emit DM4310 + DM8009 firmware images"
	@echo "  make firmwares        Same as make"
	@echo "  make dm4310           Emit only dist/development/dm4310_plain.bin + dm4310_enc.bin"
	@echo "  make dm8009           Emit only dist/development/dm8009_plain.bin + dm8009_enc.bin"
	@echo "  make build            Build the source APP only"
	@echo "  make verify-firmware-outputs  Check generated files without cross-model equality"
	@echo "  make clean            Remove build and dist outputs"
	@echo "  make send CAN_IF=can0 Send dist/development/dm4310_enc.bin through the loader"
	@echo ""
	@echo "Toolchain: locked by cmake/arm-none-eabi-toolchain.cmake"
	@echo "Default toolchain root: tools/arm-gnu-toolchain"
	@echo "Override only when needed: DM_ARM_TOOLCHAIN_ROOT=/path/to/arm-gnu-toolchain make"
	@echo ""
	@echo "Current DM8009 behavior: built from the same source APP target until a separate DM8009 source target is restored."
	@echo "Bootloader source is intentionally not built or tracked in this app-only workspace."
	@echo "No DM4310-vs-DM8009 byte-equality check is enforced; the two models may diverge during development."

all: firmwares

configure:
	$(CMAKE) -S . -B $(BUILD_DIR) \
		-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN) \
		-DCMAKE_BUILD_TYPE=RelWithDebInfo

build: configure
	$(CMAKE) --build $(BUILD_DIR) --target damiao_app -j

$(DIST_DEV):
	mkdir -p $(DIST_DEV)

$(PACK_ROOT)/dm4310 $(PACK_ROOT)/dm8009:
	mkdir -p $@

dm4310: build $(DIST_DEV) $(PACK_ROOT)/dm4310
	$(PYTHON) tools/pack_update.py --app $(SOURCE_APP_BIN) \
		--profile config/update_profile.json --output-dir $(PACK_ROOT)/dm4310 \
		--purpose development \
		--plain-name dm4310_plain.bin \
		--encrypted-name dm4310_enc.bin
	cp $(PACK_ROOT)/dm4310/dm4310_plain.bin $(DIST_DEV)/dm4310_plain.bin
	cp $(PACK_ROOT)/dm4310/dm4310_enc.bin $(DIST_DEV)/dm4310_enc.bin
	@echo "wrote $(DIST_DEV)/dm4310_plain.bin"
	@echo "wrote $(DIST_DEV)/dm4310_enc.bin"

dm8009: build $(DIST_DEV) $(PACK_ROOT)/dm8009
	$(PYTHON) tools/pack_update.py --app $(SOURCE_APP_BIN) \
		--profile config/update_profile.json --output-dir $(PACK_ROOT)/dm8009 \
		--purpose development \
		--plain-name dm8009_plain.bin \
		--encrypted-name dm8009_enc.bin
	cp $(PACK_ROOT)/dm8009/dm8009_plain.bin $(DIST_DEV)/dm8009_plain.bin
	cp $(PACK_ROOT)/dm8009/dm8009_enc.bin $(DIST_DEV)/dm8009_enc.bin
	@echo "wrote $(DIST_DEV)/dm8009_plain.bin"
	@echo "wrote $(DIST_DEV)/dm8009_enc.bin"

firmwares: dm4310 dm8009 verify-firmware-outputs
	@echo "wrote DM4310 + DM8009 firmware images to $(DIST_DEV)"

verify-firmware-outputs:
	$(PYTHON) tools/verify_firmware_outputs.py --dist $(DIST_DEV)

send: dm4310
	$(PYTHON) tools/send_update.py --interface $(CAN_IF) \
		--encrypted $(DIST_DEV)/dm4310_enc.bin --yes

provision-calibration:
	$(PYTHON) tools/provision_calibration.py \
		--output-dir $(DIST_DIR)/calibration

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)
