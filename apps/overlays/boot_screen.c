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

#include "boot_screen.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_log.h"
#include "boards/board_config.h"
#include <stdio.h>
#include <stdlib.h>

#define TAG "BOOT_SCREEN"

typedef struct {
    lv_obj_t           *screen;
    lv_obj_t           *bar;
    lv_obj_t           *status_lbl;
    uint8_t             progress;
    boot_complete_cb_t  on_complete;
    lv_timer_t         *timer;
} boot_screen_t;

static boot_screen_t *s_boot = NULL;

bool boot_screen_is_active(void)
{
    return s_boot != NULL;
}

static const char *s_boot_stages[] = {
    "Initializing Hardware HAL...",
    "Mounting Virtual File System...",
    "Starting System Event Queue...",
    "Loading Desktop Shell...",
    "Welcome to VeebhaOS"
};

static void on_boot_timer(lv_timer_t *tmr)
{
    if (!s_boot) {
        lv_timer_delete(tmr);
        return;
    }

    s_boot->progress += 25;
    if (s_boot->progress > 100) s_boot->progress = 100;

    if (s_boot->bar && lv_obj_is_valid(s_boot->bar)) {
        lv_bar_set_value(s_boot->bar, s_boot->progress, LV_ANIM_OFF);
    }

    uint8_t stage_idx = s_boot->progress / 25;
    if (stage_idx >= 5) stage_idx = 4;

    if (s_boot->status_lbl && lv_obj_is_valid(s_boot->status_lbl)) {
        lv_label_set_text(s_boot->status_lbl, s_boot_stages[stage_idx]);
    }

    if (s_boot->progress >= 100) {
        OS_LOGI(TAG, "VeebhaOS boot sequence complete -> transitioning to shell");
        boot_complete_cb_t cb = s_boot->on_complete;
        lv_obj_t *boot_scr = s_boot->screen;

        if (s_boot->timer) {
            lv_timer_delete(s_boot->timer);
            s_boot->timer = NULL;
        }

        free(s_boot);
        s_boot = NULL;

        if (cb) {
            cb();
        }

        if (boot_scr && lv_obj_is_valid(boot_scr)) {
            lv_obj_delete_async(boot_scr);
        }
    }
}

void boot_screen_start(boot_complete_cb_t on_complete)
{
    boot_screen_t *b = (boot_screen_t *)calloc(1, sizeof(boot_screen_t));
    if (!b) return;

    b->on_complete = on_complete;
    b->progress = 0;

    lv_obj_t *scr = lv_obj_create(NULL);
    b->screen = scr;
    lv_obj_set_size(scr, CONFIG_DISP_HOR_RES, CONFIG_DISP_VER_RES);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0A0F1D), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* 1. Brand Logo Container */
    lv_obj_t *logo_box = lv_obj_create(scr);
    lv_obj_set_size(logo_box, 140, 60);
    lv_obj_set_style_bg_opa(logo_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(logo_box, 0, 0);
    lv_obj_set_style_pad_all(logo_box, 0, 0);
    lv_obj_set_flex_flow(logo_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(logo_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(logo_box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(logo_box);
    lv_label_set_text(title, "VEEBHA OS");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);

    lv_obj_t *sub = lv_label_create(logo_box);
    lv_label_set_text(sub, "Feature Phone OS");
    lv_obj_set_style_text_color(sub, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_10, 0);

    /* 2. Progress Bar */
    b->bar = lv_bar_create(scr);
    lv_obj_set_size(b->bar, 120, 4);
    lv_bar_set_range(b->bar, 0, 100);
    lv_bar_set_value(b->bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(b->bar, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(b->bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(b->bar, 2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(b->bar, lv_color_hex(0x00E5FF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(b->bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(b->bar, 2, LV_PART_INDICATOR);
    lv_obj_set_style_margin_top(b->bar, 14, 0);

    /* 3. Status Diagnostic Text */
    b->status_lbl = lv_label_create(scr);
    lv_label_set_text(b->status_lbl, "Starting VeebhaOS...");
    lv_obj_set_style_text_color(b->status_lbl, lv_color_hex(0x64748B), 0);
    lv_obj_set_style_text_font(b->status_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_margin_top(b->status_lbl, 8, 0);

    s_boot = b;
    lv_scr_load(scr);

    /* Progress timer: tick every 80ms */
    b->timer = lv_timer_create(on_boot_timer, 80, b);
    OS_LOGI(TAG, "VeebhaOS animated boot screen started");
}
