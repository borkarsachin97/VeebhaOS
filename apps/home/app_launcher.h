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

#ifndef APPS_HOME_APP_LAUNCHER_H
#define APPS_HOME_APP_LAUNCHER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * Instantiate the 3x3 Main Menu Launcher screen.
 *
 * @return Root screen lv_obj_t pointer ready for win_mgr_push().
 */
lv_obj_t * app_launcher_create(void);

/**
 * Open the 3x3 Main Menu Launcher from Standby / Idle.
 */
void app_launcher_open(void);

/**
 * Invalidate cached launcher screen so it re-renders on next launch.
 */
void app_launcher_invalidate(void);

/**
 * Open the built-in Music Player application.
 */
void app_music_open(void);

#ifdef __cplusplus
}
#endif

#endif /* APPS_HOME_APP_LAUNCHER_H */
