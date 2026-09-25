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

#ifndef APPS_HOME_APP_IDLE_H
#define APPS_HOME_APP_IDLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdbool.h>

/**
 * Instantiate the zero-coordinate Standby / Idle Screen (Root Index 0).
 *
 * @return Root screen lv_obj_t pointer.
 */
lv_obj_t * app_idle_create(void);

/**
 * Dynamically refresh clock, date, and conditional notification pills.
 */
void app_idle_update(void);

/**
 * Open / display the Standby / Idle Screen.
 */
void app_idle_open(void);

/**
 * Check if the Standby / Idle Screen is currently active.
 */
bool app_idle_is_active(void);

/**
 * Wallpaper style profiles for the Standby / Idle screen.
 */
typedef enum {
    WALLPAPER_DARK = 0,
    WALLPAPER_CYBER,
    WALLPAPER_SUNSET,
    WALLPAPER_EMERALD,
    WALLPAPER_COUNT
} idle_wallpaper_t;

/**
 * Set active wallpaper profile on the Idle screen.
 */
void app_idle_set_wallpaper(idle_wallpaper_t wp);

/**
 * Get the currently active wallpaper profile.
 */
idle_wallpaper_t app_idle_get_wallpaper(void);

/**
 * Refresh wallpaper image and scrim opacity on active Idle screen.
 */
void app_idle_refresh_wallpaper(void);

#ifdef __cplusplus
}
#endif

#endif /* APPS_HOME_APP_IDLE_H */
