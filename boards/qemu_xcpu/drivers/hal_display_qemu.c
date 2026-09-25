/*
 * VeebhaOS - QEMU RDA8809 Display Driver
 * Connects LVGL display flush callback to GOUDA 2D DMA Blitter
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "hal_display_qemu.h"
#include "boards/qemu_xcpu/include/hal_gouda.h"
#include "boards/qemu_xcpu/include/lcd_ili9225g.h"
#include "boards/board_config.h"
#include <string.h>

#define DISP_HOR_RES CONFIG_DISP_HOR_RES
#define DISP_VER_RES CONFIG_DISP_VER_RES

/* Double 176x220 16-bit RGB565 framebuffers in PSRAM for DMA ping-pong */
static lv_color_t s_disp_buf1[DISP_HOR_RES * DISP_VER_RES] __attribute__((aligned(8)));
static lv_color_t s_disp_buf2[DISP_HOR_RES * DISP_VER_RES] __attribute__((aligned(8)));
static lv_display_t *s_lv_disp = NULL;

static void qemu_display_rounder_cb(lv_event_t *e)
{
    lv_area_t *area = (lv_area_t *)lv_event_get_param(e);
    if (!area) return;

    /* Align x1 to even column (2-pixel / 4-byte boundary) */
    area->x1 &= ~1;

    /* Ensure width is even (x2 is odd) */
    int32_t w = (area->x2 - area->x1 + 1 + 1) & ~1;
    if (area->x1 + w > DISP_HOR_RES) {
        w = DISP_HOR_RES - area->x1;
    }
    area->x2 = area->x1 + w - 1;

    /* Boundary clamping */
    if (area->x2 >= DISP_HOR_RES) area->x2 = DISP_HOR_RES - 1;
    if (area->y2 >= DISP_VER_RES) area->y2 = DISP_VER_RES - 1;
}

static void qemu_display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint16_t w = area->x2 - area->x1 + 1;
    uint16_t h = area->y2 - area->y1 + 1;

    const lcd_panel_t *panel = lcd_ili9225g_get_panel();
    if (panel && panel->set_window) {
        panel->set_window(area->x1, area->y1, area->x2, area->y2);
    }

    /* Asynchronous Gouda DMA Blit: hardware streams in background */
    hal_GoudaBlitRoi((const uint16_t *)px_map, w, h, area->x1, area->y1, w, h, true);

    /* Signal LVGL immediately so CPU renders the next block in the other buffer */
    lv_display_flush_ready(disp);
}

void hal_display_qemu_init(void)
{
    s_lv_disp = lv_display_create(DISP_HOR_RES, DISP_VER_RES);
    lv_display_set_buffers(s_lv_disp, s_disp_buf1, s_disp_buf2, sizeof(s_disp_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(s_lv_disp, qemu_display_flush_cb);
    lv_display_add_event_cb(s_lv_disp, qemu_display_rounder_cb, LV_EVENT_INVALIDATE_AREA, NULL);
}

lv_display_t * hal_display_qemu_get_disp(void)
{
    return s_lv_disp;
}
