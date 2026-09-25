# VeebhaOS VAPP Application Makefile
# SPDX-License-Identifier: MIT

CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2
PYTHON ?= python3

VAPP_PACK = ../../tools/vapp_pack.py
TARGET_BIN = build/app.bin
TARGET_VAPP = build/2048_retro.vapp
OUTPUT_DIR = ../../vapps

all: build pack

build:
	@mkdir -p build
	@$(CC) $(CFLAGS) src/main.c -o $(TARGET_BIN)
	@echo "[BUILD] Compiled binary payload: $(TARGET_BIN)"

pack: build
	@$(PYTHON) $(VAPP_PACK) --manifest vapp.json --bin $(TARGET_BIN) --output $(TARGET_VAPP)
	@mkdir -p $(OUTPUT_DIR)
	@cp $(TARGET_VAPP) $(OUTPUT_DIR)/
	@echo "[DEPLOY] Copied $(TARGET_VAPP) -> $(OUTPUT_DIR)/"

clean:
	@rm -rf build

.PHONY: all build pack clean
