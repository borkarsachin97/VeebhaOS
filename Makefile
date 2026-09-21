# VeebhaOS Root Makefile Helper

BUILD_DIR ?= build
CMAKE ?= cmake
CTEST ?= ctest

.PHONY: all build sim clean test asan

all: build

build:
	@$(CMAKE) -B $(BUILD_DIR) -S .
	@$(CMAKE) --build $(BUILD_DIR) -j$$(nproc)

sim: build
	@echo "[SIM] Launching VeebhaOS Desktop Simulator..."
	@./$(BUILD_DIR)/veebhaos_sim

test:
	@$(CMAKE) -B $(BUILD_DIR) -S .
	@$(CMAKE) --build $(BUILD_DIR) -j$$(nproc)
	@echo "[TEST] Running automated test verification..."
	@./$(BUILD_DIR)/veebhaos_sim --test

asan:
	@$(CMAKE) -B $(BUILD_DIR) -S . -DENABLE_ASAN=ON
	@$(CMAKE) --build $(BUILD_DIR) -j$$(nproc)
	@echo "[ASAN] Running with AddressSanitizer..."
	@./$(BUILD_DIR)/veebhaos_sim --test

clean:
	@echo "[CLEAN] Removing build directory..."
	@rm -rf $(BUILD_DIR)
