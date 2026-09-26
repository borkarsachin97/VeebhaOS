/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Simulator Board Descriptor
 *
 * SPDX-License-Identifier: MIT
 */

#include "boards/board_info.h"

static const board_info_t s_sim_board_info = {
    .brand              = "Veebha",
    .model              = "Desktop Simulator",
    .codename           = "sim_host",
#if defined(__x86_64__) || defined(_M_X64)
    .cpu_name           = "Host x86_64 Processor",
    .arch               = "x86_64 / POSIX",
#elif defined(__aarch64__) || defined(_M_ARM64)
    .cpu_name           = "Host ARM64 Processor",
    .arch               = "aarch64 / POSIX",
#else
    .cpu_name           = "Host Native Core",
    .arch               = "POSIX Standard",
#endif
    .cpu_clock_mhz      = 3200,
    .ram_size_kb        = 8192,
    .flash_size_kb      = 4096,
    .display_width      = 176,
    .display_height     = 220,
    .display_controller = "SDL2 Texture Window",
    .color_format       = "RGB565",
    .gpu_blitter        = "Software CPU (LVGL)",
    .modem_type         = "Simulated GSM/LTE",
    .connectivity_chip  = "Simulated BT 4.0 / USB",
    .os_version         = VEEBHA_OS_VERSION,
    .os_developer       = VEEBHA_OS_DEVELOPER,
    .build_author       = VEEBHA_BUILD_AUTHOR,
    .build_timestamp    = __DATE__ " " __TIME__
};

const board_info_t * board_get_info(void)
{
    return &s_sim_board_info;
}

const board_info_t * board_info_get(void)
{
    return &s_sim_board_info;
}
