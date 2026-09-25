/*
 * VeebhaOS - QEMU RDA8809 MIPS XCPU Board Information
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "boards/board_info.h"

static const board_info_t s_qemu_xcpu_board = {
    .brand = "Veebha",
    .model = "RDA8809 Phone (QEMU)",
    .codename = "qemu_xcpu",
    .cpu_name = "MIPS XCPU",
    .arch = "MIPS32r1 (32-bit Little Endian)",
    .cpu_clock_mhz = 104,
    .ram_size_kb = 8192,
    .flash_size_kb = 4096,
    .display_width = 176,
    .display_height = 220,
    .display_controller = "ILI9225G / GOUDA 2D Blitter",
    .color_format = "RGB565 16-bit",
    .gpu_blitter = "Hardware GOUDA DMA",
    .modem_type = "Baseband Comregs",
    .connectivity_chip = "RDA5876 (BT + FM)",
    .os_version = "1.0.0-baremetal",
    .build_timestamp = __DATE__ " " __TIME__
};

const board_info_t * board_get_info(void)
{
    return &s_qemu_xcpu_board;
}
