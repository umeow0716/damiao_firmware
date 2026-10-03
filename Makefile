SHELL := /bin/sh

PYTHON ?= python3
CMAKE ?= cmake
BUILD_DIR ?= build
DIST_DIR ?= dist
DIST_DEV := $(DIST_DIR)/development
TOOLCHAIN := cmake/arm-none-eabi-toolchain.cmake
CAN_IF ?= can0
.DEFAULT_GOAL := firmwares

PACK_ROOT := $(BUILD_DIR)/package
FIRMWARE_MODELS := dm10010 dm3507 dm3507_48v dm4310 dm4310_48v \
	dm4340 dm4340_48v dm8006 dm8009

.PHONY: help all configure build firmwares $(FIRMWARE_MODELS) verify-firmware-outputs clean send provision-calibration

help:
	@echo "DM firmware source-build targets:"
	@echo "  make                  Build source APP and emit all model firmware images"
	@echo "  make firmwares        Same as make"
	@echo "  make MODEL            Emit one model's plain + encrypted images"
	@echo "                        MODEL: $(FIRMWARE_MODELS)"
	@echo "  make build            Build all profiled source APP targets"
	@echo "  make verify-firmware-outputs  Check generated files without cross-model equality"
	@echo "  make clean            Remove build and dist outputs"
	@echo "  make send CAN_IF=can0 Send the packaged DM4310 CAN frames through the loader"
	@echo ""
	@echo "Toolchain: locked by cmake/arm-none-eabi-toolchain.cmake"
	@echo "Default toolchain root: tools/arm-gnu-toolchain"
	@echo "Override only when needed: DM_ARM_TOOLCHAIN_ROOT=/path/to/arm-gnu-toolchain make"
	@echo ""
	@echo "All model targets use the V3 sub-version 04 target profiles."
	@echo "Bootloader source is intentionally not built or tracked in this app-only workspace."
	@echo "No cross-model byte-equality check is enforced; model images may legitimately diverge."

all: firmwares

configure:
	$(CMAKE) -S . -B $(BUILD_DIR) \
		-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN) \
		-DCMAKE_BUILD_TYPE=RelWithDebInfo

build: configure
	$(CMAKE) --build $(BUILD_DIR) --target $(FIRMWARE_MODELS) -j

$(DIST_DEV):
	mkdir -p $(DIST_DEV)


$(FIRMWARE_MODELS): %: configure $(DIST_DEV)
	mkdir -p $(PACK_ROOT)/$@
	$(CMAKE) --build $(BUILD_DIR) --target $@ -j
	$(PYTHON) tools/pack_update.py --app $(BUILD_DIR)/$@.app.bin \
		--profile config/update_profile.json --output-dir $(PACK_ROOT)/$@ \
		--purpose development \
		--plain-name $@_plain.bin \
		--encrypted-name $@_enc.bin
	cp $(PACK_ROOT)/$@/$@_plain.bin $(DIST_DEV)/$@_plain.bin
	cp $(PACK_ROOT)/$@/$@_enc.bin $(DIST_DEV)/$@_enc.bin
	@echo "wrote $(DIST_DEV)/$@_plain.bin"
	@echo "wrote $(DIST_DEV)/$@_enc.bin"

firmwares: $(FIRMWARE_MODELS)
	$(MAKE) verify-firmware-outputs
	@echo "wrote all model firmware images to $(DIST_DEV)"

verify-firmware-outputs:
	$(PYTHON) tools/verify_firmware_outputs.py --dist $(DIST_DEV) \
		--build-dir $(BUILD_DIR) --package-root $(PACK_ROOT) \
		--profile config/update_profile.json

send: dm4310
	$(PYTHON) tools/send_update.py --interface $(CAN_IF) \
		--frames $(PACK_ROOT)/dm4310/app_update.frames.jsonl --yes

provision-calibration:
	$(PYTHON) tools/provision_calibration.py \
		--output-dir $(DIST_DIR)/calibration

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)
