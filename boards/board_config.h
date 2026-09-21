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

#ifndef BOARDS_BOARD_CONFIG_H
#define BOARDS_BOARD_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Master Board Switchboard
 * Selects the target board profile at compile time.
 * If no board target is defined via compiler flags (-D...),
 * default to the SDL2 desktop simulator.
 */
#if !defined(CONFIG_BOARD_SIMULATOR) && !defined(CONFIG_BOARD_PHOENIX) && !defined(CONFIG_BOARD_MUMBA)
#define CONFIG_BOARD_SIMULATOR 1
#endif

#if defined(CONFIG_BOARD_SIMULATOR) && CONFIG_BOARD_SIMULATOR
#include "simulator/board_simulator.h"
#elif defined(CONFIG_BOARD_PHOENIX) && CONFIG_BOARD_PHOENIX
#include "phoenix/board_phoenix.h"
#elif defined(CONFIG_BOARD_MUMBA) && CONFIG_BOARD_MUMBA
#include "mumba/board_mumba.h"
#else
#error "No valid board configuration selected in board_config.h!"
#endif

/* System viewport partitioning constants */
#ifndef CONFIG_STATUS_BAR_HEIGHT
#define CONFIG_STATUS_BAR_HEIGHT 18
#endif

#ifndef CONFIG_SOFTKEY_BAR_HEIGHT
#define CONFIG_SOFTKEY_BAR_HEIGHT 20
#endif

#ifndef CONFIG_VIEWPORT_HEIGHT
#define CONFIG_VIEWPORT_HEIGHT (CONFIG_DISP_VER_RES - CONFIG_STATUS_BAR_HEIGHT - CONFIG_SOFTKEY_BAR_HEIGHT)
#endif

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_BOARD_CONFIG_H */
