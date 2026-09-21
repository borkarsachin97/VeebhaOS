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

#define MAX_GRID_CELLS 32

typedef struct {
    lv_obj_t          *softkey_bar;
    lv_obj_t          *first_item;
    veebha_view_type_t view_type;
    void (*on_select)(uint16_t index);
    void (*on_back)(void);
    uint16_t           count;
    uint8_t            columns;
    lv_obj_t          *cells[MAX_GRID_CELLS];
} tpl_grid_screen_data_t;

static void on_grid_screen_delete_cb(lv_event_t *e)
{
    lv_obj_t *scr = lv_event_get_target(e);
    tpl_grid_screen_data_t *data = (tpl_grid_screen_data_t *)lv_obj_get_user_data(scr);
    if (data) {
        free(data);
        lv_obj_set_user_data(scr, NULL);
    }
}

static void on_grid_item_clicked(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    uint16_t idx = (uint16_t)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t *scr = lv_obj_get_screen(btn);

    if (scr) {
        tpl_grid_screen_data_t *data = (tpl_grid_screen_data_t *)lv_obj_get_user_data(scr);
        if (data && data->on_select) {
            data->on_select(idx);
        }
    }
}

static void on_grid_cell_key_cb(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    uint16_t idx = (uint16_t)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *scr = lv_obj_get_screen(btn);
    tpl_grid_screen_data_t *data = (tpl_grid_screen_data_t *)lv_obj_get_user_data(scr);

    if (!data || data->count == 0) return;

    uint16_t count = data->count;
    uint8_t cols = data->columns ? data->columns : 3;
    uint16_t target = idx;

    if (key == LV_KEY_RIGHT) {
        /* Pressing Right on column 3 wraps to column 1 of next row */
        target = (idx + 1) % count;
        if (data->cells[target]) {
            lv_group_focus_obj(data->cells[target]);
        }
    } else if (key == LV_KEY_LEFT) {
        /* Pressing Left on column 1 wraps to column 3 of previous row */
        target = (idx + count - 1) % count;
        if (data->cells[target]) {
            lv_group_focus_obj(data->cells[target]);
        }
    } else if (key == LV_KEY_DOWN) {
        /* Pressing Down moves to same column in next row; wraps to top row */
        if (idx + cols < count) {
            target = idx + cols;
        } else {
            target = idx % cols;
        }
        if (data->cells[target]) {
            lv_group_focus_obj(data->cells[target]);
        }
    } else if (key == LV_KEY_UP) {
        /* Pressing Up moves to same column in previous row; wraps to bottom row */
        if (idx >= cols) {
            target = idx - cols;
        } else {
            uint16_t b = idx;
            while (b + cols < count) b += cols;
            target = b;
        }
        if (data->cells[target]) {
            lv_group_focus_obj(data->cells[target]);
        }
    }
}

void tpl_grid_default_lsk(void)
{
    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_obj_t *focused = lv_group_get_focused(g);
        if (focused) {
            lv_obj_send_event(focused, LV_EVENT_CLICKED, NULL);
        }
    }
}

void tpl_grid_default_rsk(void)
{
    win_mgr_entry_t *top = win_mgr_get_top();
    if (top && top->screen) {
        tpl_grid_screen_data_t *data = (tpl_grid_screen_data_t *)lv_obj_get_user_data(top->screen);
        if (data && data->on_back) {
            data->on_back();
            return;
        }
    }
    win_mgr_pop();
}

lv_obj_t * tpl_grid_create(const tpl_grid_view_t *desc)
{
    if (!desc) return NULL;

    uint8_t cols = desc->columns ? desc->columns : 3;

    /* 1. Root Screen: Dark Charcoal #121212 */
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x121212), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Allocate screen context */
    tpl_grid_screen_data_t *data = (tpl_grid_screen_data_t *)malloc(sizeof(tpl_grid_screen_data_t));
    if (data) {
        data->softkey_bar = NULL;
        data->first_item = NULL;
        data->view_type = VEEBHA_VIEW_TYPE_GRID;
        data->on_select = desc->on_select;
        data->on_back = desc->on_back;
        data->count = (desc->count < MAX_GRID_CELLS) ? desc->count : MAX_GRID_CELLS;
        data->columns = cols;
        for (int c = 0; c < MAX_GRID_CELLS; c++) data->cells[c] = NULL;
    }
    lv_obj_set_user_data(screen, data);
    lv_obj_add_event_cb(screen, on_grid_screen_delete_cb, LV_EVENT_DELETE, NULL);

    /* 2. Zone A: Fixed 18px Top Status Bar */
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

    lv_obj_t *title_lbl = lv_label_create(status_bar);
    lv_label_set_text(title_lbl, desc->title ? desc->title : "Menu");
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_12, 0);

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

    /* 3. Zone B: Elastic 184px Content Viewport with CSS Grid */
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_size(content, lv_pct(100), 0);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_color(content, lv_color_hex(0x121212), 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_radius(content, 0, 0);
    lv_obj_set_style_pad_all(content, 4, 0);
    lv_obj_set_style_pad_row(content, 4, 0);
    lv_obj_set_style_pad_column(content, 4, 0);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);

    /* 3 Equal Fractional Columns */
    static const int32_t col_dsc[] = {
        LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST
    };
    static const int32_t row_dsc[] = {
        LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST
    };
    lv_obj_set_grid_dsc_array(content, col_dsc, row_dsc);

    lv_group_t *group = win_mgr_get_group();
    uint16_t total = (desc->count < MAX_GRID_CELLS) ? desc->count : MAX_GRID_CELLS;

    for (uint16_t i = 0; i < total; i++) {
        lv_obj_t *cell = lv_button_create(content);
        if (i == 0 && data) {
            data->first_item = cell;
        }
        if (data) {
            data->cells[i] = cell;
        }
        lv_obj_set_user_data(cell, (void *)(uintptr_t)i);

        /* Place in Grid Matrix (Column c, Row r) */
        uint8_t c = i % cols;
        uint8_t r = i / cols;
        lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_STRETCH, c, 1,
                                   LV_GRID_ALIGN_STRETCH, r, 1);

        /* Cell Styling: Squircle Card with Flex Column Layout */
        lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(cell, 2, 0);
        lv_obj_set_style_radius(cell, 8, 0);

        /* Default Styling */
        lv_obj_set_style_bg_color(cell, lv_color_hex(0x1C2028), 0);
        lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(cell, 0, 0);

        /* Focused Styling: High-contrast Cyan border & accent background */
        lv_obj_set_style_bg_color(cell, lv_color_hex(0x1A3555), LV_STATE_FOCUSED);
        lv_obj_set_style_border_color(cell, lv_color_hex(0x00E5FF), LV_STATE_FOCUSED);
        lv_obj_set_style_border_width(cell, 2, LV_STATE_FOCUSED);

        /* Icon Label */
        if (desc->items[i].icon) {
            lv_obj_t *icon_lbl = lv_label_create(cell);
            lv_label_set_text(icon_lbl, (const char *)desc->items[i].icon);
            lv_obj_set_style_text_color(icon_lbl, lv_color_hex(0x00E5FF), 0);
            lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_16, 0);
        }

        /* Title Label */
        lv_obj_t *lbl = lv_label_create(cell);
        lv_label_set_text(lbl, desc->items[i].title ? desc->items[i].title : "");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x00E5FF), LV_STATE_FOCUSED);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);

        /* 4-way Keypad Navigation Event */
        lv_obj_add_event_cb(cell, on_grid_cell_key_cb, LV_EVENT_KEY, (void *)(uintptr_t)i);

        /* Click Event */
        lv_obj_add_event_cb(cell, on_grid_item_clicked, LV_EVENT_CLICKED, (void *)(uintptr_t)i);

        /* Scroll On Focus */
        lv_obj_add_flag(cell, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

        /* Register to Keypad Group */
        if (group) {
            lv_group_add_obj(group, cell);
        }
    }

    /* 4. Zone C: Fixed 20px Bottom Softkey Bar */
    const char *lsk = desc->lsk_label ? desc->lsk_label : "OK";
    const char *rsk = desc->rsk_label ? desc->rsk_label : "Back";
    lv_obj_t *bar = softkey_bar_create(screen, lsk, rsk);
    if (data) {
        data->softkey_bar = bar;
    }

    return screen;
}
