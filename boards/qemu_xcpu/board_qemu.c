/*
 * VeebhaOS - QEMU MIPS32 Target Descriptors (Placeholder)
 *
 * SPDX-License-Identifier: MIT
 */

#include "boards/board_info.h"

static const board_info_t s_qemu_board = {
    .brand = "Veebha",
    .model = "QEMU Virtual Phone",
    .codename = "qemu_xcpu",
    .cpu_name = "MIPS32 24KEc",
    .arch = "MIPS32r1 (32-bit Little Endian)",
    .cpu_clock_mhz = 312,
    .ram_size_kb = 16384,
    .flash_size_kb = 8192,
    .display_width = 176,
    .display_height = 220,
    .display_controller = "ST7775R Virtual Framebuffer",
    .color_format = "RGB565 16-bit",
    .gpu_blitter = "Software CPU (LVGL)",
    .modem_type = "Simulated Baseband (AT UART)",
    .connectivity_chip = "Virtual Bluetooth 4.0",
    .os_version = "0.6.0-dev",
    .build_timestamp = __DATE__ " " __TIME__
};

const board_info_t * board_get_info(void)
{
    return &s_qemu_board;
}
