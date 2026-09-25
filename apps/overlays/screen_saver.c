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

#include "screen_saver.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_log.h"
#include "boards/board_config.h"
#include <stdio.h>
#include <stdlib.h>

#define TAG "SCREEN_SAVER"

#define SCREEN_SAVER_IDLE_TIMEOUT_MS 30000

static lv_obj_t   *s_ss_overlay = NULL;
static lv_obj_t   *s_ss_clock_box = NULL;
static lv_obj_t   *s_ss_clock_lbl = NULL;
static lv_obj_t   *s_ss_date_lbl = NULL;
static lv_timer_t *s_ss_drift_timer = NULL;
static uint32_t    s_last_activity_ms = 0;
static bool        s_is_active = false;

static int8_t s_drift_offsets[][2] = {
    {  0,   0 },
    { 10, -15 },
    { -8,  10 },
    { 14,   8 },
    { -12, -8 },
};
static uint8_t s_drift_step = 0;

static void on_drift_timer(lv_timer_t *tmr)
{
    (void)tmr;
    if (!s_is_active || !s_ss_clock_box || !lv_obj_is_valid(s_ss_clock_box)) return;

    s_drift_step = (s_drift_step + 1) % (sizeof(s_drift_offsets) / sizeof(s_drift_offsets[0]));
    lv_obj_set_pos(s_ss_clock_box, s_drift_offsets[s_drift_step][0], s_drift_offsets[s_drift_step][1]);
}

void screen_saver_init(void)
{
    s_last_activity_ms = 0;
    s_is_active = false;
    OS_LOGI(TAG, "Screen Saver engine initialized");
}

void screen_saver_show(void)
{
    if (s_is_active) return;

    lv_obj_t *top = lv_layer_top();
    if (!top) return;

    s_ss_overlay = lv_obj_create(top);
    lv_obj_set_size(s_ss_overlay, CONFIG_DISP_HOR_RES, CONFIG_DISP_VER_RES);
    lv_obj_set_pos(s_ss_overlay, 0, 0);
    lv_obj_set_style_bg_color(s_ss_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_ss_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_ss_overlay, 0, 0);
    lv_obj_set_style_pad_all(s_ss_overlay, 0, 0);
    lv_obj_remove_flag(s_ss_overlay, LV_OBJ_FLAG_SCROLLABLE);

    /* Drifting Clock Container */
    s_ss_clock_box = lv_obj_create(s_ss_overlay);
    lv_obj_set_size(s_ss_clock_box, 140, 70);
    lv_obj_center(s_ss_clock_box);
    lv_obj_set_style_bg_opa(s_ss_clock_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_ss_clock_box, 0, 0);
    lv_obj_set_style_pad_all(s_ss_clock_box, 0, 0);
    lv_obj_set_flex_flow(s_ss_clock_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_ss_clock_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(s_ss_clock_box, LV_OBJ_FLAG_SCROLLABLE);

    /* Clock Label */
    s_ss_clock_lbl = lv_label_create(s_ss_clock_box);
    lv_label_set_text(s_ss_clock_lbl, "12:00");
    lv_obj_set_style_text_color(s_ss_clock_lbl, lv_color_hex(0x64748B), 0); /* Low-glare slate */
    lv_obj_set_style_text_font(s_ss_clock_lbl, &lv_font_montserrat_24, 0);

    /* Date and Battery Label */
    s_ss_date_lbl = lv_label_create(s_ss_clock_box);
    lv_label_set_text(s_ss_date_lbl, "Tue 22 Sep | " LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(s_ss_date_lbl, lv_color_hex(0x475569), 0);
    lv_obj_set_style_text_font(s_ss_date_lbl, &lv_font_montserrat_10, 0);

    /* Drift timer: shift every 3 seconds */
    s_ss_drift_timer = lv_timer_create(on_drift_timer, 3000, NULL);
    s_is_active = true;
    OS_LOGI(TAG, "Low-power Screen Saver active on lv_layer_top");
}

void screen_saver_hide(void)
{
    if (!s_is_active) return;

    if (s_ss_drift_timer) {
        lv_timer_delete(s_ss_drift_timer);
        s_ss_drift_timer = NULL;
    }

    if (s_ss_overlay && lv_obj_is_valid(s_ss_overlay)) {
        lv_obj_delete_async(s_ss_overlay);
        s_ss_overlay = NULL;
        s_ss_clock_box = NULL;
    }

    s_is_active = false;
    OS_LOGI(TAG, "Screen Saver dismissed");
}

bool screen_saver_is_active(void)
{
    return s_is_active;
}

void screen_saver_reset_idle(void)
{
    s_last_activity_ms = 0;
    if (s_is_active) {
        screen_saver_hide();
    }
}

void screen_saver_check_idle(uint32_t now_ms)
{
    if (s_last_activity_ms == 0) {
        s_last_activity_ms = now_ms;
        return;
    }

    if (!s_is_active && (now_ms - s_last_activity_ms >= SCREEN_SAVER_IDLE_TIMEOUT_MS)) {
        screen_saver_show();
    }
}
