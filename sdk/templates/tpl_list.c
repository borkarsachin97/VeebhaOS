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

#include "veebha_templates.h"
#include "veebha_win_mgr.h"
#include "veebha_softkeys.h"
#include "boards/board_config.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    lv_obj_t *softkey_bar;
    lv_obj_t *first_item;
    void (*on_select)(uint16_t index);
    void (*on_back)(void);
} tpl_list_screen_data_t;

static void on_screen_delete_cb(lv_event_t *e)
{
    lv_obj_t *scr = lv_event_get_target(e);
    tpl_list_screen_data_t *data = (tpl_list_screen_data_t *)lv_obj_get_user_data(scr);
    if (data) {
        free(data);
        lv_obj_set_user_data(scr, NULL);
    }
}

static void on_item_clicked(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    uint16_t idx = (uint16_t)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t *scr = lv_obj_get_screen(btn);

    if (scr) {
        tpl_list_screen_data_t *data = (tpl_list_screen_data_t *)lv_obj_get_user_data(scr);
        if (data && data->on_select) {
            data->on_select(idx);
        }
    }
}

void tpl_list_default_lsk(void)
{
    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_obj_t *focused = lv_group_get_focused(g);
        if (focused) {
            lv_obj_send_event(focused, LV_EVENT_CLICKED, NULL);
        }
    }
}

void tpl_list_default_rsk(void)
{
    win_mgr_entry_t *top = win_mgr_get_top();
    if (top && top->screen) {
        tpl_list_screen_data_t *data = (tpl_list_screen_data_t *)lv_obj_get_user_data(top->screen);
        if (data && data->on_back) {
            data->on_back();
            return;
        }
    }
    win_mgr_pop();
}

lv_obj_t * tpl_list_create(const tpl_list_view_t *desc)
{
    if (!desc) return NULL;

    /* 1. Root Screen: Dark Charcoal #121212 */
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x121212), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Allocate screen context */
    tpl_list_screen_data_t *data = (tpl_list_screen_data_t *)malloc(sizeof(tpl_list_screen_data_t));
    if (data) {
        data->softkey_bar = NULL;
        data->first_item = NULL;
        data->on_select = desc->on_select;
        data->on_back = desc->on_back;
    }
    lv_obj_set_user_data(screen, data);
    lv_obj_add_event_cb(screen, on_screen_delete_cb, LV_EVENT_DELETE, NULL);

    /* 2. Zone A: Fixed 18px Top Status Bar / Header */
    lv_obj_t *status_bar = lv_obj_create(screen);
    lv_obj_set_size(status_bar, lv_pct(100), CONFIG_STATUS_BAR_HEIGHT);
    lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x181A20), 0);
    lv_obj_set_style_bg_opa(status_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_set_style_radius(status_bar, 0, 0);
    lv_obj_set_style_pad_hor(status_bar, 4, 0);
    lv_obj_set_style_pad_ver(status_bar, 0, 0);
    lv_obj_remove_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Header Title */
    lv_obj_t *title_lbl = lv_label_create(status_bar);
    lv_label_set_text(title_lbl, desc->title ? desc->title : "VeebhaOS");
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0x00E5FF), 0); /* Cyan accent */
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_12, 0);

    /* Right Indicator: Time & Battery */
    lv_obj_t *right_tray = lv_obj_create(status_bar);
    lv_obj_set_size(right_tray, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(right_tray, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right_tray, 0, 0);
    lv_obj_set_style_pad_all(right_tray, 0, 0);
    lv_obj_set_style_pad_column(right_tray, 4, 0);
    lv_obj_remove_flag(right_tray, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(right_tray, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(right_tray, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *time_lbl = lv_label_create(right_tray);
    lv_label_set_text(time_lbl, "12:00");
    lv_obj_set_style_text_color(time_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(time_lbl, &lv_font_montserrat_12, 0);

    lv_obj_t *bat_lbl = lv_label_create(right_tray);
    lv_label_set_text(bat_lbl, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(bat_lbl, lv_color_hex(0x00E676), 0);
    lv_obj_set_style_text_font(bat_lbl, &lv_font_montserrat_12, 0);

    /* 3. Zone B: Elastic 184px Content Viewport */
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_size(content, lv_pct(100), 0);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_color(content, lv_color_hex(0x121212), 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_radius(content, 0, 0);
    lv_obj_set_style_pad_all(content, 2, 0);
    lv_obj_set_style_pad_row(content, 2, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);

    /* Scroll setup: Center-locked snapping, hidden scrollbars */
    lv_obj_set_scroll_snap_y(content, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);

    /* Add Items */
    lv_group_t *group = win_mgr_get_group();

    for (uint16_t i = 0; i < desc->count; i++) {
        lv_obj_t *btn = lv_button_create(content);
        if (i == 0 && data) {
            data->first_item = btn;
        }
        lv_obj_set_size(btn, lv_pct(100), 28);
        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_hor(btn, 6, 0);
        lv_obj_set_style_pad_ver(btn, 0, 0);
        lv_obj_set_style_radius(btn, 4, 0);

        /* Default Unfocused Styling */
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1A1D24), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(btn, 0, 0);

        /* Focused Styling: High-contrast Cyan accent & blue highlight */
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1A3555), LV_STATE_FOCUSED);
        lv_obj_set_style_border_color(btn, lv_color_hex(0x00E5FF), LV_STATE_FOCUSED);
        lv_obj_set_style_border_width(btn, 1, LV_STATE_FOCUSED);

        /* Optional Icon */
        if (desc->items[i].icon) {
            lv_obj_t *icon_lbl = lv_label_create(btn);
            lv_label_set_text(icon_lbl, (const char *)desc->items[i].icon);
            lv_obj_set_style_text_color(icon_lbl, lv_color_hex(0x00E5FF), 0);
            lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_12, 0);
            lv_obj_set_style_pad_right(icon_lbl, 4, 0);
        }

        /* Title Label */
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, desc->items[i].title ? desc->items[i].title : "");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x00E5FF), LV_STATE_FOCUSED);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
        lv_obj_set_flex_grow(lbl, 1);

        /* Optional Subtext */
        if (desc->items[i].subtext) {
            lv_obj_t *sub = lv_label_create(btn);
            lv_label_set_text(sub, desc->items[i].subtext);
            lv_obj_set_style_text_color(sub, lv_color_hex(0x8A8D93), 0);
            lv_obj_set_style_text_font(sub, &lv_font_montserrat_12, 0);
        }

        /* Click Event Binding */
        lv_obj_add_event_cb(btn, on_item_clicked, LV_EVENT_CLICKED, (void *)(uintptr_t)i);

        /* Register to Keypad Group */
        if (group) {
            lv_group_add_obj(group, btn);
        }
    }

    /* 4. Zone C: Fixed 20px Bottom Softkey Bar */
    const char *lsk = desc->lsk_label ? desc->lsk_label : "Select";
    const char *rsk = desc->rsk_label ? desc->rsk_label : "Back";
    lv_obj_t *bar = softkey_bar_create(screen, lsk, rsk);
    if (data) {
        data->softkey_bar = bar;
    }

    return screen;
}
