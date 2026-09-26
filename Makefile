# VeebhaOS Root Makefile Helper
# SPDX-License-Identifier: MIT

BUILD_DIR ?= build
CMAKE ?= cmake
BOARD ?= simulator

.PHONY: all build sim test asan clean

all: build

ifeq ($(BOARD),obtel_b10)

BSP_DIR ?= boards/obtel_b10

ifneq ($(wildcard $(BSP_DIR)/Makefile),)
build:
	@$(MAKE) -C $(BSP_DIR) all

clean:
	@$(MAKE) -C $(BSP_DIR) clean
	@rm -rf $(BUILD_DIR)
else
build:
	@echo "=================================================================="
	@echo "[!] OBTEL B10 Board Support Package (BSP) not found at: $(BSP_DIR)"
	@echo "[!] The RDA8809 hardware drivers are maintained in an external"
	@echo "[!] repository due to proprietary vendor SDK licensing."
	@echo ""
	@echo "    To build for OBTEL B10:"
	@echo "      1. Clone the BSP: git clone <bsp-repo> boards/obtel_b10"
	@echo "      2. Or specify BSP_DIR: make BOARD=obtel_b10 BSP_DIR=/path/to/bsp"
	@echo "=================================================================="
	@exit 1

clean:
	@rm -rf $(BUILD_DIR)
endif

else

$(BUILD_DIR)/CMakeCache.txt: CMakeLists.txt
	@$(CMAKE) -B $(BUILD_DIR) -S . -DBOARD=$(BOARD)

build: $(BUILD_DIR)/CMakeCache.txt
	@$(CMAKE) --build $(BUILD_DIR) -j$$(nproc)

sim: build
	@echo "[SIM] Launching VeebhaOS Desktop Simulator..."
	@./$(BUILD_DIR)/veebha_os

test: build
	@echo "[TEST] Running automated test verification..."
	@./$(BUILD_DIR)/veebha_os --test

asan:
	@$(CMAKE) -B $(BUILD_DIR) -S . -DBOARD=$(BOARD) -DENABLE_ASAN=ON
	@$(CMAKE) --build $(BUILD_DIR) -j$$(nproc)
	@echo "[ASAN] Running with AddressSanitizer..."
	@./$(BUILD_DIR)/veebha_os --test

clean:
	@echo "[CLEAN] Removing build directory..."
	@rm -rf $(BUILD_DIR)

endif
