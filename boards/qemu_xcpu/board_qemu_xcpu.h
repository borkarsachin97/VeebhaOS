/*
 * VeebhaOS - QEMU RDA8809 MIPS XCPU Board Configuration
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BOARDS_QEMU_XCPU_BOARD_QEMU_XCPU_H
#define BOARDS_QEMU_XCPU_BOARD_QEMU_XCPU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "boards/board_info.h"
#include <stdint.h>
#include <stdbool.h>

/* Display & Color Metrics */
#define CONFIG_DISP_HOR_RES         176
#define CONFIG_DISP_VER_RES         220
#define CONFIG_DISP_COLOR_DEPTH     16
#define CONFIG_LV_MEM_SIZE          (256 * 1024)

/* RAM-only freestanding execution */
#ifndef CONFIG_IS_RAMRUN_ONLY
#define CONFIG_IS_RAMRUN_ONLY       1
#endif

/* Status and Softkey bar dimensions */
#define CONFIG_STATUS_BAR_HEIGHT    18
#define CONFIG_SOFTKEY_BAR_HEIGHT   20

/* Hardware capabilities */
#define CONFIG_BOARD_SUPPORT_2G         1
#define CONFIG_BOARD_SUPPORT_BLUETOOTH  1
#define CONFIG_BOARD_SUPPORT_FM_RADIO   1
#define CONFIG_BOARD_SUPPORT_SDCARD     1
#define CONFIG_BOARD_SUPPORT_TORCH      1
#define CONFIG_BOARD_SUPPORT_AUDIO_JACK 1
#define CONFIG_BOARD_SUPPORT_VIBRATOR   1

/* Board hardware initialization */
void board_qemu_hardware_init(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_QEMU_XCPU_BOARD_QEMU_XCPU_H */
