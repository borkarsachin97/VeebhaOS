# VeebhaOS Board Support Package: Desktop Simulator
# SPDX-License-Identifier: MIT

BOARD_NAME := simulator

BOARD_CFLAGS += -Iboards/simulator \
                -Iboards/simulator/port \
                -Iboards/simulator/drivers \
                -DCONFIG_BOARD_SIMULATOR

BOARD_SRCS += boards/simulator/board_info.c \
              boards/simulator/sim_main.c \
              boards/simulator/sim_keyboard_map.c \
              boards/simulator/drivers/hal_display_sim.c \
              boards/simulator/drivers/hal_keypad_sim.c \
              boards/simulator/drivers/hal_power_sim.c \
              boards/simulator/drivers/hal_audio_sim.c \
              boards/simulator/port/port.c \
              boards/simulator/port/utils/wait_for_event.c \
              boards/simulator/port/heap_3.c
