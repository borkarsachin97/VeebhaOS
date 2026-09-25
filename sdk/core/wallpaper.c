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

#include "wallpaper.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/storage/os_nvram.h"
#include "sdk/include/veebha_log.h"
#include "apps/home/app_idle.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TAG "WALLPAPER"

#define WP_WIDTH  176
#define WP_HEIGHT 220

static wallpaper_mode_t s_mode = WALLPAPER_MODE_THEME_SOLID;
static char             s_current_path[64] = DEFAULT_WALLPAPER_PATH;
static lv_color_t       s_solid_color;

/* 16-bit RGB565 Pixel Buffer for 176x220 Image */
static uint16_t         s_pixel_buf[WP_WIDTH * WP_HEIGHT];

/* LVGL Image Resource Descriptor */
static lv_image_dsc_t   s_img_dsc = {
    .header = {
        .magic = LV_IMAGE_HEADER_MAGIC,
        .cf = LV_COLOR_FORMAT_RGB565,
        .flags = 0,
        .w = WP_WIDTH,
        .h = WP_HEIGHT,
        .stride = WP_WIDTH * 2,
    },
    .data_size = WP_WIDTH * WP_HEIGHT * 2,
    .data = (const uint8_t *)s_pixel_buf,
    .reserved = NULL,
};

static uint16_t pack_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static void generate_procedural_wallpaper(const char *filename)
{
    const char *name = filename;
    const char *slash = strrchr(filename, '/');
    if (slash) name = slash + 1;

    for (int y = 0; y < WP_HEIGHT; y++) {
        int ny = (y * 256) / (WP_HEIGHT - 1);
        for (int x = 0; x < WP_WIDTH; x++) {
            int nx = (x * 256) / (WP_WIDTH - 1);
            uint8_t r = 0, g = 0, b = 0;

            if (strcasecmp(name, "Abstract.bmp") == 0) {
                /* Abstract: Deep purple to magenta with geometric waves */
                int t = ny;
                if (t < 0) t = 0;
                if (t > 255) t = 255;
                r = (uint8_t)(45 + (t * 140) / 256);
                g = (uint8_t)(17 + (t * 40) / 256);
                b = (uint8_t)(44 + (t * 120) / 256);
            } else if (strcasecmp(name, "Neon.bmp") == 0) {
                /* Neon: Cyberpunk dark magenta to electric cyan */
                int t = (nx + ny) / 2;
                r = (uint8_t)((27 * (256 - t)) / 256);
                g = (uint8_t)((160 * t) / 256);
                b = (uint8_t)((58 * (256 - t) + 210 * t) / 256);
                /* Subtle grid lines */
                if (x % 22 == 0 || y % 22 == 0) {
                    r = (uint8_t)(r + 30 > 255 ? 255 : r + 30);
                    g = (uint8_t)(g + 45 > 255 ? 255 : g + 45);
                    b = (uint8_t)(b + 45 > 255 ? 255 : b + 45);
                }
            } else if (strcasecmp(name, "Nature.bmp") == 0) {
                /* Nature: Emerald canopy gradient */
                int t = ny;
                r = (uint8_t)(10 + (t * 25) / 256);
                g = (uint8_t)(28 + (t * 90) / 256);
                b = (uint8_t)(20 + (t * 45) / 256);
            } else {
                /* Default / Cyber: Dark navy to deep ocean cyan */
                int t = ny;
                r = (uint8_t)(6 + (t * 15) / 256);
                g = (uint8_t)(20 + (t * 65) / 256);
                b = (uint8_t)(29 + (t * 110) / 256);
            }

            s_pixel_buf[y * WP_WIDTH + x] = pack_rgb565(r, g, b);
        }
    }
}

static bool load_bmp_file(const char *path)
{
#if defined(CONFIG_SIMULATOR) && !defined(CONFIG_IS_RAMRUN_ONLY)
    FILE *f = fopen(path, "rb");
    if (!f) {
        /* Also check relative path if path starts with /sdcard/ */
        if (strncmp(path, "/sdcard/", 8) == 0) {
            char rel_path[128];
            snprintf(rel_path, sizeof(rel_path), "./sdcard/%s", path + 8);
            f = fopen(rel_path, "rb");
            if (!f) {
                snprintf(rel_path, sizeof(rel_path), "./build/sdcard/%s", path + 8);
                f = fopen(rel_path, "rb");
            }
        }
    }
#else
    FILE *f = NULL;
#endif

    if (!f) {
        /* File not on physical disk; fallback to built-in procedural VFS image generator */
        generate_procedural_wallpaper(path);
        return true;
    }

    uint8_t header[54];
    if (fread(header, 1, 54, f) != 54) {
        fclose(f);
        return false;
    }

    if (header[0] != 'B' || header[1] != 'M') {
        fclose(f);
        return false;
    }

    uint32_t data_offset = header[10] | (header[11] << 8) | (header[12] << 16) | (header[13] << 24);
    int32_t width = header[18] | (header[19] << 8) | (header[20] << 16) | (header[21] << 24);
    int32_t height = header[22] | (header[23] << 8) | (header[24] << 16) | (header[25] << 24);
    uint16_t bpp = header[28] | (header[29] << 8);

    bool flip_y = true;
    if (height < 0) {
        height = -height;
        flip_y = false;
    }

    if (width <= 0 || height <= 0 || (bpp != 16 && bpp != 24 && bpp != 32)) {
        fclose(f);
        return false;
    }

    fseek(f, data_offset, SEEK_SET);

    size_t row_stride = ((width * bpp + 31) / 32) * 4;
    uint8_t *row_buf = (uint8_t *)malloc(row_stride);
    if (!row_buf) {
        fclose(f);
        return false;
    }

    for (int y = 0; y < height && y < WP_HEIGHT; y++) {
        if (fread(row_buf, 1, row_stride, f) != row_stride) break;

        int target_y = flip_y ? (height - 1 - y) : y;
        if (target_y >= WP_HEIGHT) continue;

        for (int x = 0; x < width && x < WP_WIDTH; x++) {
            uint16_t pixel = 0;
            if (bpp == 24) {
                uint8_t b = row_buf[x * 3 + 0];
                uint8_t g = row_buf[x * 3 + 1];
                uint8_t r = row_buf[x * 3 + 2];
                pixel = pack_rgb565(r, g, b);
            } else if (bpp == 32) {
                uint8_t b = row_buf[x * 4 + 0];
                uint8_t g = row_buf[x * 4 + 1];
                uint8_t r = row_buf[x * 4 + 2];
                pixel = pack_rgb565(r, g, b);
            } else if (bpp == 16) {
                pixel = row_buf[x * 2] | (row_buf[x * 2 + 1] << 8);
            }
            s_pixel_buf[target_y * WP_WIDTH + x] = pixel;
        }
    }

    free(row_buf);
    fclose(f);
    return true;
}

void wallpaper_init(void)
{
    s_solid_color = theme_get()->bg_color;

    os_nvram_data_t *nv = os_nvram_get();
    if (nv && nv->wallpaper_mode == (uint8_t)WALLPAPER_MODE_IMAGE_BMP && nv->wallpaper_path[0] != '\0') {
        wallpaper_set_image(nv->wallpaper_path);
    } else {
        wallpaper_set_solid(s_solid_color);
    }

    OS_LOGI(TAG, "Wallpaper Engine initialized (Mode: %s, Path: %s)",
            (s_mode == WALLPAPER_MODE_IMAGE_BMP) ? "IMAGE_BMP" : "THEME_SOLID",
            s_current_path);
}

void wallpaper_set_solid(lv_color_t color)
{
    s_mode = WALLPAPER_MODE_THEME_SOLID;
    s_solid_color = color;
    app_idle_refresh_wallpaper();
    OS_LOGI(TAG, "Set wallpaper mode to THEME_SOLID (0x%06X)",
            (unsigned int)lv_color_to_u32(color) & 0xFFFFFF);
}

bool wallpaper_set_image(const char *vfs_path)
{
    if (!vfs_path || vfs_path[0] == '\0') {
        vfs_path = DEFAULT_WALLPAPER_PATH;
    }

    if (!load_bmp_file(vfs_path)) {
        OS_LOGW(TAG, "Failed to load BMP wallpaper: %s", vfs_path);
        return false;
    }

    s_mode = WALLPAPER_MODE_IMAGE_BMP;
    strncpy(s_current_path, vfs_path, sizeof(s_current_path) - 1);
    s_current_path[sizeof(s_current_path) - 1] = '\0';

    app_idle_refresh_wallpaper();

    OS_LOGI(TAG, "Set wallpaper image: %s", s_current_path);
    return true;
}

const char * wallpaper_get_current_path(void)
{
    return s_current_path;
}

wallpaper_mode_t wallpaper_get_mode(void)
{
    return s_mode;
}

const lv_image_dsc_t * wallpaper_get_img_dsc(void)
{
    return &s_img_dsc;
}

lv_color_t wallpaper_get_solid_color(void)
{
    return s_solid_color;
}
