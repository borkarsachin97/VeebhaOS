# VeebhaOS Pluggable Board Support Package (BSP) Configuration
# SPDX-License-Identifier: MIT

BOARD ?= simulator

# Include board-specific definitions
-include boards/$(BOARD)/board.mk
