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

#include "drivers/hal_display.h"
#include "boards/board_config.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_ROWS (CONFIG_DISP_VER_RES / 10) /* 1/10th screen height: 22 rows */
#define BUFFER_PIXELS (CONFIG_DISP_HOR_RES * BUFFER_ROWS)
#define BUFFER_SIZE_BYTES (BUFFER_PIXELS * sizeof(lv_color16_t))

static SDL_Window *s_window = NULL;
static SDL_Renderer *s_renderer = NULL;
static SDL_Texture *s_texture = NULL;
static lv_display_t *s_disp = NULL;

/* Double partial buffers sized to 1/10th screen height */
static uint8_t s_buf1[BUFFER_SIZE_BYTES];
static uint8_t s_buf2[BUFFER_SIZE_BYTES];

static void sdl_display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    if (s_texture && s_renderer) {
        int32_t w = lv_area_get_width(area);
        int32_t h = lv_area_get_height(area);

        SDL_Rect rect;
        rect.x = area->x1;
        rect.y = area->y1;
        rect.w = w;
        rect.h = h;

        /* Pitch is width in bytes: 16-bit RGB565 */
        SDL_UpdateTexture(s_texture, &rect, px_map, (int)(w * sizeof(uint16_t)));

        if (lv_display_flush_is_last(disp)) {
            SDL_RenderClear(s_renderer);
            SDL_RenderCopy(s_renderer, s_texture, NULL, NULL);
            SDL_RenderPresent(s_renderer);
        }
    }

    lv_display_flush_ready(disp);
}

bool hal_display_init(void)
{
    if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
            fprintf(stderr, "[HAL_DISP] SDL_InitSubSystem(VIDEO) failed: %s\n", SDL_GetError());
            return false;
        }
    }

    int scale = CONFIG_SIM_WINDOW_SCALE;
    int win_w = CONFIG_DISP_HOR_RES * scale;
    int win_h = CONFIG_DISP_VER_RES * scale;

    s_window = SDL_CreateWindow(
        "VeebhaOS - 176x220 (MIPS Simulator)",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        win_w,
        win_h,
        SDL_WINDOW_SHOWN
    );

    if (!s_window) {
        fprintf(stderr, "[HAL_DISP] SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    s_renderer = SDL_CreateRenderer(
        s_window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!s_renderer) {
        fprintf(stderr, "[HAL_DISP] SDL_CreateRenderer fallback to software: %s\n", SDL_GetError());
        s_renderer = SDL_CreateRenderer(s_window, -1, SDL_RENDERER_SOFTWARE);
        if (!s_renderer) {
            fprintf(stderr, "[HAL_DISP] SDL_CreateRenderer failed: %s\n", SDL_GetError());
            SDL_DestroyWindow(s_window);
            s_window = NULL;
            return false;
        }
    }

    SDL_RenderSetLogicalSize(s_renderer, CONFIG_DISP_HOR_RES, CONFIG_DISP_VER_RES);

    s_texture = SDL_CreateTexture(
        s_renderer,
        SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STREAMING,
        CONFIG_DISP_HOR_RES,
        CONFIG_DISP_VER_RES
    );

    if (!s_texture) {
        fprintf(stderr, "[HAL_DISP] SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(s_renderer);
        SDL_DestroyWindow(s_window);
        s_renderer = NULL;
        s_window = NULL;
        return false;
    }

    /* Initialize LVGL Display Driver */
    s_disp = lv_display_create(CONFIG_DISP_HOR_RES, CONFIG_DISP_VER_RES);
    if (!s_disp) {
        fprintf(stderr, "[HAL_DISP] lv_display_create failed\n");
        return false;
    }

    lv_display_set_color_format(s_disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(s_disp, sdl_display_flush_cb);
    lv_display_set_buffers(s_disp, s_buf1, s_buf2, sizeof(s_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    return true;
}

void hal_display_deinit(void)
{
    if (s_texture) {
        SDL_DestroyTexture(s_texture);
        s_texture = NULL;
    }
    if (s_renderer) {
        SDL_DestroyRenderer(s_renderer);
        s_renderer = NULL;
    }
    if (s_window) {
        SDL_DestroyWindow(s_window);
        s_window = NULL;
    }
}

void hal_display_set_rotation(hal_disp_rot_t rot)
{
    if (!s_disp) return;

    switch (rot) {
    case HAL_DISP_ROT_90:
        lv_display_set_rotation(s_disp, LV_DISPLAY_ROTATION_90);
        break;
    case HAL_DISP_ROT_180:
        lv_display_set_rotation(s_disp, LV_DISPLAY_ROTATION_180);
        break;
    case HAL_DISP_ROT_270:
        lv_display_set_rotation(s_disp, LV_DISPLAY_ROTATION_270);
        break;
    case HAL_DISP_ROT_0:
    default:
        lv_display_set_rotation(s_disp, LV_DISPLAY_ROTATION_0);
        break;
    }
}

uint16_t hal_display_get_hor_res(void)
{
    return CONFIG_DISP_HOR_RES;
}

uint16_t hal_display_get_ver_res(void)
{
    return CONFIG_DISP_VER_RES;
}

lv_display_t * hal_display_get_lv_display(void)
{
    return s_disp;
}
