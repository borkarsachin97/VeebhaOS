/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */

#ifndef BOARDS_SIMULATOR_BOARD_SIMULATOR_H
#define BOARDS_SIMULATOR_BOARD_SIMULATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_BOARD_NAME "VeebhaOS SDL2 Simulator"

/* Display Configuration: 176x220 portrait RGB565 */
#define CONFIG_DISP_HOR_RES 176
#define CONFIG_DISP_VER_RES 220
#define CONFIG_COLOR_DEPTH  16

/* Simulator Window Scaling (Integer scaling for high-DPI desktop clarity) */
#ifndef CONFIG_SIM_WINDOW_SCALE
#define CONFIG_SIM_WINDOW_SCALE 3
#endif

/* Simulator Environment Marker */
#define CONFIG_SIMULATOR 1

/* Mock Peripherals */
#define CONFIG_HAS_BATTERY   1
#define CONFIG_HAS_RTC       1
#define CONFIG_HAS_BLUETOOTH 1
#define CONFIG_HAS_CELLULAR  1
#define CONFIG_HAS_SDCARD    1
#define CONFIG_HAS_AUDIO     1

/* Memory Fence: 2.0 MB Dynamic Pool Limit */
#define CONFIG_LV_MEM_SIZE (2 * 1024 * 1024U)

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_SIMULATOR_BOARD_SIMULATOR_H */
