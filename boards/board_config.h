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
#if !defined(CONFIG_BOARD_SIMULATOR) && !defined(CONFIG_BOARD_OBTEL_B10)
#define CONFIG_BOARD_SIMULATOR 1
#endif

#if defined(CONFIG_BOARD_SIMULATOR) && CONFIG_BOARD_SIMULATOR
#include "simulator/board_simulator.h"
#elif defined(CONFIG_BOARD_OBTEL_B10) && CONFIG_BOARD_OBTEL_B10
#if defined(__has_include)
#if __has_include("obtel_b10/board_obtel_b10.h")
#include "obtel_b10/board_obtel_b10.h"
#else
#error "OBTEL B10 BSP header not found. Please clone the BSP repository into boards/obtel_b10/ or set BSP include path."
#endif
#else
#include "obtel_b10/board_obtel_b10.h"
#endif
#else
#error "No valid board configuration selected in board_config.h!"
#endif

/* Fallback hardware capability flags */
#ifndef CONFIG_BOARD_SUPPORT_2G
#define CONFIG_BOARD_SUPPORT_2G 1
#endif
#ifndef CONFIG_BOARD_SUPPORT_3G
#define CONFIG_BOARD_SUPPORT_3G 0
#endif
#ifndef CONFIG_BOARD_SUPPORT_4G
#define CONFIG_BOARD_SUPPORT_4G 0
#endif
#ifndef CONFIG_BOARD_SUPPORT_WIFI
#define CONFIG_BOARD_SUPPORT_WIFI 0
#endif
#ifndef CONFIG_BOARD_SUPPORT_BLUETOOTH
#define CONFIG_BOARD_SUPPORT_BLUETOOTH 1
#endif
#ifndef CONFIG_BOARD_SUPPORT_FM_RADIO
#define CONFIG_BOARD_SUPPORT_FM_RADIO 1
#endif
#ifndef CONFIG_BOARD_SUPPORT_SDCARD
#define CONFIG_BOARD_SUPPORT_SDCARD 1
#endif
#ifndef CONFIG_BOARD_SUPPORT_CAMERA
#define CONFIG_BOARD_SUPPORT_CAMERA 1
#endif
#ifndef CONFIG_BOARD_SUPPORT_TORCH
#define CONFIG_BOARD_SUPPORT_TORCH 1
#endif
#ifndef CONFIG_BOARD_SUPPORT_AUDIO_JACK
#define CONFIG_BOARD_SUPPORT_AUDIO_JACK 1
#endif
#ifndef CONFIG_BOARD_SUPPORT_VIBRATOR
#define CONFIG_BOARD_SUPPORT_VIBRATOR 1
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


/* ============================================================
 * Performance optimisation feature flags
 * Set to 1 to enable, 0 to disable. Can also be overridden via -D on the compiler command line.
 * ============================================================ */

/* Pre-allocate a hidden list screen at boot and reuse it for Settings/Tools. */
#ifndef CONFIG_USE_TEMPLATE_SCREEN
#define CONFIG_USE_TEMPLATE_SCREEN 1
#endif

/* Build placeholder-then-real-list lazy-load support.
 * Disabled by default; enable with -DCONFIG_ENABLE_LAZY_LOAD=1 at compile time. */
#ifndef CONFIG_ENABLE_LAZY_LOAD
#define CONFIG_ENABLE_LAZY_LOAD 0
#endif

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_BOARD_CONFIG_H */
