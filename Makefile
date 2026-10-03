SHELL := /bin/sh

PYTHON ?= python3
CMAKE ?= cmake
BUILD_DIR ?= build
DIST_DIR ?= dist
DIST_DEV := $(DIST_DIR)/development
DIST_FACTORY := $(DIST_DEV)/factory
DIST_RAW := $(DIST_DEV)/raw
TOOLCHAIN := cmake/arm-none-eabi-toolchain.cmake
CAN_IF ?= can0
.DEFAULT_GOAL := firmwares

PACK_ROOT := $(BUILD_DIR)/package
FIRMWARE_MODELS := dm10010 dm3507 dm3507_48v dm4310 dm4310_48v \
	dm4340 dm4340_48v dm8006 dm8009
RAW_FIRMWARE_TARGETS := $(addsuffix _raw,$(FIRMWARE_MODELS))
FIRMWARE_TARGETS := $(FIRMWARE_MODELS) $(RAW_FIRMWARE_TARGETS)

.PHONY: help all configure build firmwares $(FIRMWARE_MODELS)
.PHONY: verify-firmware-outputs clean send send-raw provision-calibration

help:
	@echo "DM firmware source-build targets:"
	@echo "  make                  Build factory/raw APP variants for every model"
	@echo "  make firmwares        Same as make"
	@echo "  make MODEL            Emit one model's factory/raw plain + encrypted images"
	@echo "                        MODEL: $(FIRMWARE_MODELS)"
	@echo "  make build            Build all profiled source APP targets"
	@echo "  make verify-firmware-outputs  Check generated files without cross-model equality"
	@echo "  make clean            Remove build and dist outputs"
	@echo "  make send CAN_IF=can0 Send the factory DM4310 package through the loader"
	@echo "  make send-raw CAN_IF=can0  Send the raw-feedback DM4310 package"
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
	$(CMAKE) --build $(BUILD_DIR) --target $(FIRMWARE_TARGETS) -j

$(DIST_FACTORY) $(DIST_RAW):
	mkdir -p $@

$(FIRMWARE_MODELS): %: configure $(DIST_FACTORY) $(DIST_RAW)
	mkdir -p $(PACK_ROOT)/factory/$@ $(PACK_ROOT)/raw/$@
	$(CMAKE) --build $(BUILD_DIR) --target $@ $@_raw -j
	$(PYTHON) tools/pack_update.py --app $(BUILD_DIR)/$@.app.bin \
		--profile config/update_profile.json --output-dir $(PACK_ROOT)/factory/$@ \
		--purpose development --firmware-variant factory \
		--plain-name $@_plain.bin \
		--encrypted-name $@_enc.bin
	$(PYTHON) tools/pack_update.py --app $(BUILD_DIR)/$@_raw.app.bin \
		--profile config/update_profile.json --output-dir $(PACK_ROOT)/raw/$@ \
		--purpose development --firmware-variant raw \
		--plain-name $@_plain.bin \
		--encrypted-name $@_enc.bin
	cp $(PACK_ROOT)/factory/$@/$@_plain.bin $(DIST_FACTORY)/$@_plain.bin
	cp $(PACK_ROOT)/factory/$@/$@_enc.bin $(DIST_FACTORY)/$@_enc.bin
	cp $(PACK_ROOT)/raw/$@/$@_plain.bin $(DIST_RAW)/$@_plain.bin
	cp $(PACK_ROOT)/raw/$@/$@_enc.bin $(DIST_RAW)/$@_enc.bin
	@echo "wrote factory and raw $@ images under $(DIST_DEV)"

firmwares: $(FIRMWARE_MODELS)
	$(MAKE) verify-firmware-outputs
	@echo "wrote all factory/raw model firmware images to $(DIST_DEV)"

verify-firmware-outputs:
	$(PYTHON) tools/verify_firmware_outputs.py --dist $(DIST_FACTORY) \
		--build-dir $(BUILD_DIR) --package-root $(PACK_ROOT)/factory \
		--profile config/update_profile.json --expected-variant factory
	$(PYTHON) tools/verify_firmware_outputs.py --dist $(DIST_RAW) \
		--build-dir $(BUILD_DIR) --build-suffix _raw \
		--package-root $(PACK_ROOT)/raw \
		--profile config/update_profile.json --expected-variant raw

send: dm4310
	$(PYTHON) tools/send_update.py --interface $(CAN_IF) \
		--frames $(PACK_ROOT)/factory/dm4310/app_update.frames.jsonl --yes

send-raw: dm4310
	$(PYTHON) tools/send_update.py --interface $(CAN_IF) \
		--frames $(PACK_ROOT)/raw/dm4310/app_update.frames.jsonl --yes

provision-calibration:
	$(PYTHON) tools/provision_calibration.py \
		--output-dir $(DIST_DIR)/calibration

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)
