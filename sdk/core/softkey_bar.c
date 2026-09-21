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

#include "veebha_softkeys.h"
#include "boards/board_config.h"
#include <stdio.h>
#include <string.h>

static softkey_callback_t s_lsk_cb = NULL;
static softkey_callback_t s_rsk_cb = NULL;
static softkey_callback_t s_rsk_long_cb = NULL;

static lv_obj_t *s_active_bar = NULL;
static lv_obj_t *s_lsk_label = NULL;
static lv_obj_t *s_rsk_label = NULL;

void softkey_bar_set_active_widget(lv_obj_t *bar)
{
    s_active_bar = bar;
    if (bar && lv_obj_is_valid(bar)) {
        uint32_t cnt = lv_obj_get_child_count(bar);
        s_lsk_label = (cnt > 0) ? lv_obj_get_child(bar, 0) : NULL;
        s_rsk_label = (cnt > 1) ? lv_obj_get_child(bar, 1) : NULL;
    } else {
        s_active_bar = NULL;
        s_lsk_label = NULL;
        s_rsk_label = NULL;
    }
}

void softkey_set_actions(const char *lsk_label, softkey_callback_t lsk_cb,
                         const char *rsk_label, softkey_callback_t rsk_cb)
{
    s_lsk_cb = lsk_cb;
    s_rsk_cb = rsk_cb;
    s_rsk_long_cb = NULL;

    if (s_lsk_label) {
        lv_label_set_text(s_lsk_label, lsk_label ? lsk_label : "");
    }
    if (s_rsk_label) {
        lv_label_set_text(s_rsk_label, rsk_label ? rsk_label : "");
    }
}

void softkey_set_rsk_long_action(softkey_callback_t rsk_long_cb)
{
    s_rsk_long_cb = rsk_long_cb;
}

softkey_callback_t softkey_get_rsk_long_action(void)
{
    return s_rsk_long_cb;
}

void softkey_trigger_lsk(void)
{
    if (s_lsk_cb) {
        s_lsk_cb();
    }
}

void softkey_trigger_rsk(void)
{
    if (s_rsk_cb) {
        s_rsk_cb();
    }
}

void softkey_trigger_rsk_long(void)
{
    if (s_rsk_long_cb) {
        s_rsk_long_cb();
    }
}

lv_obj_t * softkey_bar_create(lv_obj_t *parent, const char *lsk_label, const char *rsk_label)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, lv_pct(100), CONFIG_SOFTKEY_BAR_HEIGHT);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x181A20), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(bar, lv_color_hex(0x282C35), 0);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_width(bar, 1, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_pad_hor(bar, 6, 0);
    lv_obj_set_style_pad_ver(bar, 0, 0);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    /* Flex row: space between */
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Child 0: Left Softkey */
    lv_obj_t *lsk = lv_label_create(bar);
    lv_label_set_text(lsk, lsk_label ? lsk_label : "");
    lv_obj_set_style_text_color(lsk, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lsk, &lv_font_montserrat_12, 0);

    /* Child 1: Right Softkey */
    lv_obj_t *rsk = lv_label_create(bar);
    lv_label_set_text(rsk, rsk_label ? rsk_label : "");
    lv_obj_set_style_text_color(rsk, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(rsk, &lv_font_montserrat_12, 0);

    softkey_bar_set_active_widget(bar);
    return bar;
}
