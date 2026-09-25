# VeebhaOS Board Support Package: OBTEL B10 (RDA8809 MIPS32r1)
# SPDX-License-Identifier: MIT

BOARD_NAME := obtel_b10

CROSS_COMPILE ?= mipsel-elf-

BOARD_CFLAGS += -mips32 -EL -msoft-float -nostdlib \
                -Iboards/obtel_b10 \
                -Iboards/obtel_b10/include \
                -Iboards/obtel_b10/port \
                -Iboards/obtel_b10/drivers \
                -DCONFIG_BOARD_OBTEL_B10

BOARD_LDFLAGS += -Tboards/obtel_b10/link.ld -nostdlib -Wl,--gc-sections

BOARD_SRCS += boards/obtel_b10/board_info.c \
              boards/obtel_b10/port/isram_stub.S \
              boards/obtel_b10/port/port.c \
              boards/obtel_b10/port/heap_4.c \
              boards/obtel_b10/drivers/hal_display_obtel.c \
              boards/obtel_b10/drivers/hal_keypad_obtel.c \
              boards/obtel_b10/drivers/hal_power_obtel.c \
              boards/obtel_b10/drivers/hal_audio_obtel.c
