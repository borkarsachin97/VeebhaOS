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

#ifndef DRIVERS_HAL_DISPLAY_H
#define DRIVERS_HAL_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"

/**
 * Display Rotation Enums
 */
typedef enum {
    HAL_DISP_ROT_0 = 0,
    HAL_DISP_ROT_90 = 90,
    HAL_DISP_ROT_180 = 180,
    HAL_DISP_ROT_270 = 270
} hal_disp_rot_t;

/**
 * Initialize the display subsystem.
 * Creates the LVGL display instance and configures draw buffers.
 *
 * @return true on success, false on initialization failure.
 */
bool hal_display_init(void);

/**
 * Deinitialize the display subsystem and free associated resources.
 */
void hal_display_deinit(void);

/**
 * Set the hardware/display rotation angle.
 *
 * @param rot Rotation angle (0, 90, 180, 270 degrees).
 */
void hal_display_set_rotation(hal_disp_rot_t rot);

/**
 * Get current horizontal resolution.
 */
uint16_t hal_display_get_hor_res(void);

/**
 * Get current vertical resolution.
 */
uint16_t hal_display_get_ver_res(void);

/**
 * Retrieve the active LVGL display object handle.
 */
lv_display_t * hal_display_get_lv_display(void);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_HAL_DISPLAY_H */
