/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Simulator Hardware Abstraction Layer: Display Driver
 *
 * SPDX-License-Identifier: MIT
 */

#include "sdk/hal/hal_display.h"
#include "drivers/hal_display.h"
#include "boards/board_config.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define BUFFER_ROWS (CONFIG_DISP_VER_RES / 10) /* 1/10th screen height: 22 rows */
#define BUFFER_PIXELS (CONFIG_DISP_HOR_RES * BUFFER_ROWS)
#define BUFFER_SIZE_BYTES (BUFFER_PIXELS * sizeof(lv_color16_t))

static SDL_Window *s_window = NULL;
static SDL_Renderer *s_renderer = NULL;
static SDL_Texture *s_texture = NULL;
static lv_display_t *s_disp = NULL;

/* Full-frame shadow buffer for thread-safe cross-thread frame presentation */
static uint16_t s_framebuffer[CONFIG_DISP_HOR_RES * CONFIG_DISP_VER_RES];
static volatile bool s_new_frame_ready = false;

/* Double partial buffers sized to 1/10th screen height for LVGL */
static uint8_t s_buf1[BUFFER_SIZE_BYTES];
static uint8_t s_buf2[BUFFER_SIZE_BYTES];

static void sdl_display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    if (px_map) {
        int32_t w = lv_area_get_width(area);
        int32_t h = lv_area_get_height(area);
        const uint16_t *src = (const uint16_t *)px_map;

        for (int32_t y = 0; y < h; y++) {
            int32_t dst_y = area->y1 + y;
            if (dst_y >= 0 && dst_y < CONFIG_DISP_VER_RES) {
                int32_t dst_x = area->x1;
                if (dst_x >= 0 && (dst_x + w) <= CONFIG_DISP_HOR_RES) {
                    memcpy(&s_framebuffer[dst_y * CONFIG_DISP_HOR_RES + dst_x],
                           &src[y * w],
                           w * sizeof(uint16_t));
                }
            }
        }

        if (lv_display_flush_is_last(disp)) {
            s_new_frame_ready = true;
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

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); /* Nearest-neighbor for crisp pixel rendering */

    int scale = CONFIG_SIM_WINDOW_SCALE;
    if (scale < 1) scale = 1;
    int win_w = CONFIG_DISP_HOR_RES * scale;
    int win_h = CONFIG_DISP_VER_RES * scale;

    s_window = SDL_CreateWindow(
        "VeebhaOS - 176x220 (FreeRTOS Simulator)",
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

    /* Clear initial frame to pure black */
    memset(s_framebuffer, 0, sizeof(s_framebuffer));
    SDL_UpdateTexture(s_texture, NULL, s_framebuffer, (int)(CONFIG_DISP_HOR_RES * sizeof(uint16_t)));
    SDL_RenderClear(s_renderer);
    SDL_RenderCopy(s_renderer, s_texture, NULL, NULL);
    SDL_RenderPresent(s_renderer);

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

bool hal_display_sim_has_new_frame(void)
{
    return s_new_frame_ready;
}

void hal_display_sim_present(void)
{
    if (s_texture && s_renderer) {
        s_new_frame_ready = false;
        SDL_UpdateTexture(s_texture, NULL, s_framebuffer, (int)(CONFIG_DISP_HOR_RES * sizeof(uint16_t)));
        SDL_RenderClear(s_renderer);
        SDL_RenderCopy(s_renderer, s_texture, NULL, NULL);
        SDL_RenderPresent(s_renderer);
    }
}

void hal_display_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t *color_p)
{
    if (color_p) {
        int32_t w = x2 - x1 + 1;
        int32_t h = y2 - y1 + 1;
        for (int32_t y = 0; y < h; y++) {
            int32_t dst_y = y1 + y;
            if (dst_y >= 0 && dst_y < CONFIG_DISP_VER_RES) {
                if (x1 >= 0 && (x1 + w) <= CONFIG_DISP_HOR_RES) {
                    memcpy(&s_framebuffer[dst_y * CONFIG_DISP_HOR_RES + x1],
                           &color_p[y * w],
                           w * sizeof(uint16_t));
                }
            }
        }
        s_new_frame_ready = true;
    }
}

void hal_display_set_backlight(uint8_t brightness_pct)
{
    (void)brightness_pct;
}

bool hal_display_save_screenshot(const char *filename)
{
    if (!filename) return false;

    SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(
        (void *)s_framebuffer,
        CONFIG_DISP_HOR_RES,
        CONFIG_DISP_VER_RES,
        16,
        (int)(CONFIG_DISP_HOR_RES * sizeof(uint16_t)),
        0xF800, /* Red mask for RGB565 */
        0x07E0, /* Green mask for RGB565 */
        0x001F, /* Blue mask for RGB565 */
        0       /* Alpha mask */
    );

    if (!surface) {
        fprintf(stderr, "[HAL_DISP] SDL_CreateRGBSurfaceFrom failed: %s\n", SDL_GetError());
        return false;
    }

    if (SDL_SaveBMP(surface, filename) != 0) {
        fprintf(stderr, "[HAL_DISP] SDL_SaveBMP to '%s' failed: %s\n", filename, SDL_GetError());
        SDL_FreeSurface(surface);
        return false;
    }

    SDL_FreeSurface(surface);
    printf("[HAL_DISP] Screenshot saved to '%s' (Direct Framebuffer: %dx%d RGB565)\n",
           filename, CONFIG_DISP_HOR_RES, CONFIG_DISP_VER_RES);
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
