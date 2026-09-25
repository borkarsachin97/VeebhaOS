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
#include "veebha_status_bar.h"
#include "veebha_theme.h"
#include "sdk/text/font_fallback.h"
#include "boards/board_config.h"
#include <stdlib.h>
#include <stdio.h>

#define MAX_GRID_CELLS 32

typedef struct {
    lv_obj_t          *softkey_bar;
    lv_obj_t          *first_item;
    veebha_view_type_t view_type;
    char               title[WIN_MGR_LABEL_MAX];
    os_fullscreen_mode_t fullscreen_mode;
    bool               show_battery_hud;
    void (*on_select)(uint16_t index);
    void (*on_back)(void);
    uint16_t           count;
    uint8_t            columns;
    lv_obj_t          *cells[MAX_GRID_CELLS];
    char               item_titles[MAX_GRID_CELLS][WIN_MGR_LABEL_MAX];
    lv_obj_t          *banner_lbl;
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

    lv_group_t *g = win_mgr_get_group();
    if (g && btn) {
        lv_group_focus_obj(btn);
    }

    if (scr) {
        tpl_grid_screen_data_t *data = (tpl_grid_screen_data_t *)lv_obj_get_user_data(scr);
        if (data && data->on_select) {
            data->on_select(idx);
        }
    }
}

static void on_grid_cell_focus_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    uint16_t idx = (uint16_t)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t *scr = lv_obj_get_screen(btn);

    if (scr) {
        tpl_grid_screen_data_t *data = (tpl_grid_screen_data_t *)lv_obj_get_user_data(scr);
        if (data && data->banner_lbl && idx < data->count) {
            lv_label_set_text(data->banner_lbl, data->item_titles[idx]);
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
        if (focused && lv_obj_is_valid(focused)) {
            lv_obj_send_event(focused, LV_EVENT_CLICKED, NULL);
        }
    }
}

void tpl_grid_default_rsk(void)
{
    win_mgr_entry_t *top = win_mgr_get_top();
    if (top && top->screen && lv_obj_is_valid(top->screen)) {
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

    /* 1. Root Screen */
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, CONFIG_DISP_HOR_RES, CONFIG_DISP_VER_RES);
    lv_obj_set_style_bg_color(screen, theme_get()->bg_color, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Allocate screen context */
    tpl_grid_screen_data_t *data = (tpl_grid_screen_data_t *)calloc(1, sizeof(tpl_grid_screen_data_t));
    if (data) {
        data->softkey_bar = NULL;
        data->first_item = NULL;
        data->banner_lbl = NULL;
        data->view_type = VEEBHA_VIEW_TYPE_GRID;
        data->fullscreen_mode = OS_FULLSCREEN_NONE;
        data->show_battery_hud = false;
        strncpy(data->title, desc->title ? desc->title : "Menu", sizeof(data->title) - 1);
        data->title[sizeof(data->title) - 1] = '\0';
        data->on_select = desc->on_select;
        data->on_back = desc->on_back;
        data->count = (desc->count < MAX_GRID_CELLS) ? desc->count : MAX_GRID_CELLS;
        data->columns = cols;
        for (int c = 0; c < MAX_GRID_CELLS; c++) {
            data->cells[c] = NULL;
            data->item_titles[c][0] = '\0';
        }
    }
    lv_obj_set_user_data(screen, data);
    lv_obj_add_event_cb(screen, on_grid_screen_delete_cb, LV_EVENT_DELETE, NULL);

    /* 2. Zone A: Fixed 18px Top Status Bar */
    status_bar_create(screen, NULL);

    /* Dynamic Title Banner (Height: 18px, Width: 100%) */
    lv_obj_t *banner = lv_obj_create(screen);
    lv_obj_set_size(banner, lv_pct(100), 18);
    lv_obj_set_style_bg_color(banner, theme_get()->card_color, 0);
    lv_obj_set_style_bg_opa(banner, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(banner, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(banner, theme_is_light_mode() ? lv_color_hex(0xE2E8F0) : lv_color_hex(0x282C35), 0);
    lv_obj_set_style_border_width(banner, 1, 0);
    lv_obj_set_style_radius(banner, 0, 0);
    lv_obj_set_style_pad_all(banner, 0, 0);
    lv_obj_remove_flag(banner, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(banner, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(banner, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *banner_lbl = lv_label_create(banner);
    if (data) {
        data->banner_lbl = banner_lbl;
    }
    const char *init_title = (desc->count > 0 && desc->items[0].title) ? desc->items[0].title : "";
    lv_label_set_text(banner_lbl, init_title);
    lv_obj_set_style_text_color(banner_lbl, theme_get()->accent, 0);
    lv_obj_set_style_text_font(banner_lbl, veebha_font_get_default(), 0);
    lv_obj_set_style_text_align(banner_lbl, LV_TEXT_ALIGN_CENTER, 0);

    /* 3. Zone B: Elastic Content Viewport with CSS Grid */
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_size(content, lv_pct(100), 0);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_color(content, theme_get()->bg_color, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_radius(content, 0, 0);
    lv_obj_set_style_pad_hor(content, 4, 0);
    lv_obj_set_style_pad_ver(content, 2, 0);
    lv_obj_set_style_pad_row(content, 3, 0);
    lv_obj_set_style_pad_column(content, 3, 0);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);

    uint16_t total = (desc->count < MAX_GRID_CELLS) ? desc->count : MAX_GRID_CELLS;
    uint8_t num_rows = (total + cols - 1) / cols;
    if (num_rows == 0) num_rows = 1;

    static const int32_t col_dsc[] = {
        LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST
    };

    if (total <= 9) {
        /* Standard 3x3 launcher page: non-scrollable fixed layout */
        static const int32_t row_dsc_fit[] = {
            LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST
        };
        lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLL_ANIMATION);
        lv_obj_set_grid_dsc_array(content, col_dsc, row_dsc_fit);
    } else {
        /* Multi-page grid (>9 items): scrollable with fixed row height */
        static int32_t row_dsc_multi[16];
        uint8_t r_cnt = num_rows < 15 ? num_rows : 14;
        for (uint8_t r = 0; r < r_cnt; r++) {
            row_dsc_multi[r] = 48;
        }
        row_dsc_multi[r_cnt] = LV_GRID_TEMPLATE_LAST;
        lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLL_ANIMATION);
        lv_obj_set_grid_dsc_array(content, col_dsc, row_dsc_multi);
    }

    lv_group_t *group = win_mgr_get_group();

    for (uint16_t i = 0; i < total; i++) {
        lv_obj_t *cell = lv_button_create(content);
        lv_obj_remove_flag(cell, LV_OBJ_FLAG_SCROLL_ANIMATION);
        if (i == 0 && data) {
            data->first_item = cell;
        }
        if (data) {
            data->cells[i] = cell;
            strncpy(data->item_titles[i], desc->items[i].title ? desc->items[i].title : "", WIN_MGR_LABEL_MAX - 1);
            data->item_titles[i][WIN_MGR_LABEL_MAX - 1] = '\0';
        }
        lv_obj_set_user_data(cell, (void *)(uintptr_t)i);

        /* Place in Grid Matrix (Column c, Row r) */
        uint8_t c = i % cols;
        uint8_t r = i / cols;
        lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_STRETCH, c, 1,
                                   LV_GRID_ALIGN_STRETCH, r, 1);

        /* Cell Styling: Sharp Rectangular Card with Flex Column Layout */
        lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(cell, 2, 0);
        lv_obj_set_style_radius(cell, 0, 0);
        lv_obj_set_style_outline_width(cell, 0, 0);
        lv_obj_set_style_outline_pad(cell, 0, 0);

        /* Default Styling */
        lv_obj_set_style_bg_color(cell, theme_get()->card_color, 0);
        lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(cell, 0, 0);

        /* Focused Styling: High-contrast accent border & background with sharp 90-deg geometry */
        lv_obj_set_style_bg_color(cell, theme_is_light_mode() ? lv_color_hex(0xD0E8FF) : lv_color_hex(0x1A3555), LV_STATE_FOCUSED);
        lv_obj_set_style_border_color(cell, theme_get()->accent, LV_STATE_FOCUSED);
        lv_obj_set_style_border_width(cell, 2, LV_STATE_FOCUSED);
        lv_obj_set_style_radius(cell, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(cell, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_pad(cell, 0, LV_STATE_FOCUSED);

        lv_obj_set_style_bg_color(cell, theme_is_light_mode() ? lv_color_hex(0xD0E8FF) : lv_color_hex(0x1A3555), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_border_color(cell, theme_get()->accent, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_border_width(cell, 2, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_radius(cell, 0, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(cell, 0, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_pad(cell, 0, LV_STATE_FOCUS_KEY);

        /* Clean Centered Icon Only (scaled to 20px) */
        if (desc->items[i].icon) {
            lv_obj_t *icon_lbl = lv_label_create(cell);
            lv_label_set_text(icon_lbl, (const char *)desc->items[i].icon);
            lv_obj_set_style_text_color(icon_lbl, theme_get()->accent, 0);
            lv_obj_set_style_text_color(icon_lbl, lv_color_hex(0xFFFFFF), LV_STATE_FOCUSED);
            lv_obj_set_style_text_color(icon_lbl, lv_color_hex(0xFFFFFF), LV_STATE_FOCUS_KEY);
            lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_20, 0);
        }

        /* Update Dynamic Title Banner upon receiving focus */
        lv_obj_add_event_cb(cell, on_grid_cell_focus_cb, LV_EVENT_FOCUSED, (void *)(uintptr_t)i);

        /* 4-way Keypad Navigation Event */
        lv_obj_add_event_cb(cell, on_grid_cell_key_cb, LV_EVENT_KEY, (void *)(uintptr_t)i);

        /* Click Event */
        lv_obj_add_event_cb(cell, on_grid_item_clicked, LV_EVENT_CLICKED, (void *)(uintptr_t)i);

        /* Scroll On Focus only if multi-page grid */
        if (total > 9) {
            lv_obj_add_flag(cell, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        }

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
