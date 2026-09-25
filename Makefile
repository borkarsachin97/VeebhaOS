# VeebhaOS Root Makefile Helper
# SPDX-License-Identifier: MIT

BUILD_DIR ?= build
CMAKE ?= cmake
BOARD ?= simulator

.PHONY: all build sim test asan clean

all: build

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
