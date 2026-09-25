/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * OBTEL B10 (RDA8809) Board Information Descriptor
 *
 * SPDX-License-Identifier: MIT
 */

#include "boards/board_info.h"
#include "sdk/include/veebha_version.h"

static const board_info_t s_obtel_b10_info = {
    .brand              = "OBTEL",
    .model              = "B10",
    .codename           = "312",
    .cpu_name           = "XCPU - RDA 8809",
    .arch               = "MIPS32r1 (32-bit Little Endian)",
    .cpu_clock_mhz      = 312,
    .ram_size_kb        = 8192,
    .flash_size_kb      = 4096,
    .display_width      = 176,
    .display_height     = 220,
    .display_controller = "ILI9225G/9226 SPI/8080",
    .color_format       = "RGB565",
    .gpu_blitter        = "Hardware 2D BitBLT DMA Engine",
    .modem_type         = "RDA8809 GSM/GPRS Baseband",
    .connectivity_chip  = "RDA5876 Dual BT/FM Transceiver",
    .os_version         = VEEBHA_OS_VERSION,
    .build_timestamp    = __DATE__ " " __TIME__
};

const board_info_t *board_info_get(void)
{
    return &s_obtel_b10_info;
}

const board_info_t *board_get_info(void)
{
    return &s_obtel_b10_info;
}
