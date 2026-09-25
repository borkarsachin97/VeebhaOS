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

#ifndef SDK_CORE_WALLPAPER_H
#define SDK_CORE_WALLPAPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "third_party/lvgl/lvgl.h"
#include <stdint.h>
#include <stdbool.h>

#define WALLPAPER_DIR          "/sdcard/Wallpapers"
#define DEFAULT_WALLPAPER_PATH "/sdcard/Wallpapers/default.bmp"

typedef enum {
    WALLPAPER_MODE_THEME_SOLID = 0,
    WALLPAPER_MODE_IMAGE_BMP   = 1,
} wallpaper_mode_t;

/**
 * Initialize wallpaper subsystem, restoring settings from NVRAM.
 */
void wallpaper_init(void);

/**
 * Set active wallpaper to a solid theme color.
 *
 * @param color LVGL solid color to apply.
 */
void wallpaper_set_solid(lv_color_t color);

/**
 * Set active wallpaper to a BMP image at the given VFS path.
 *
 * @param vfs_path VFS path to BMP file (e.g. "/sdcard/Wallpapers/default.bmp").
 * @return true if successfully loaded/parsed, false otherwise.
 */
bool wallpaper_set_image(const char *vfs_path);

/**
 * Retrieve the current wallpaper VFS path.
 *
 * @return String pointer to active path.
 */
const char * wallpaper_get_current_path(void);

/**
 * Retrieve the active wallpaper mode.
 *
 * @return Active wallpaper_mode_t (Solid or Image).
 */
wallpaper_mode_t wallpaper_get_mode(void);

/**
 * Retrieve the LVGL image descriptor for the active wallpaper image.
 *
 * @return Pointer to internal lv_image_dsc_t structure.
 */
const lv_image_dsc_t * wallpaper_get_img_dsc(void);

/**
 * Retrieve the active solid color (if in solid mode).
 */
lv_color_t wallpaper_get_solid_color(void);

#ifdef __cplusplus
}
#endif

#endif /* SDK_CORE_WALLPAPER_H */
