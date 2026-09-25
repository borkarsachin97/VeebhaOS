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
#include <string.h>

typedef struct {
    lv_obj_t          *softkey_bar;
    lv_obj_t          *first_item;
    veebha_view_type_t view_type;
    char               title[WIN_MGR_LABEL_MAX];
    os_fullscreen_mode_t fullscreen_mode;
    bool               show_battery_hud;
    bool               keep_alive;
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

static lv_style_t s_list_btn_style;
static lv_style_t s_list_btn_focused_style;
static lv_style_t s_list_lbl_style;
static lv_style_t s_list_lbl_focused_style;
static bool s_styles_initialized = false;
static os_theme_id_t s_last_palette = (os_theme_id_t)0xFF;
static bool s_last_light_mode = false;

static void ensure_list_styles(void)
{
    os_theme_id_t tid = theme_get_palette();
    bool is_light = theme_is_light_mode();
    if (s_styles_initialized && s_last_palette == tid && s_last_light_mode == is_light) {
        return;
    }
    s_last_palette = tid;
    s_last_light_mode = is_light;

    bool is_mono = (tid == THEME_HIGH_CONTRAST_BW);
    lv_color_t sep_color = is_mono ? lv_color_hex(0x444444) : (is_light ? lv_color_hex(0xE0E0E0) : lv_color_hex(0x222630));
    lv_color_t focus_bg = is_mono ? lv_color_hex(0xFFFFFF) : (is_light ? lv_color_hex(0xD0E8FF) : lv_color_hex(0x1B3555));
    lv_color_t text_col = is_mono ? lv_color_hex(0xFFFFFF) : theme_get()->text_primary;
    lv_color_t focus_text = is_mono ? lv_color_hex(0x000000) : theme_get()->accent;

    if (s_styles_initialized) {
        lv_style_reset(&s_list_btn_style);
        lv_style_reset(&s_list_btn_focused_style);
        lv_style_reset(&s_list_lbl_style);
        lv_style_reset(&s_list_lbl_focused_style);
    }

    /* Unfocused Button: Transparent, zero pixel fill, 1px bottom separator line */
    lv_style_init(&s_list_btn_style);
    lv_style_set_bg_opa(&s_list_btn_style, LV_OPA_TRANSP);
    lv_style_set_border_side(&s_list_btn_style, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_border_width(&s_list_btn_style, 1);
    lv_style_set_border_color(&s_list_btn_style, sep_color);
    lv_style_set_radius(&s_list_btn_style, 0);
    lv_style_set_shadow_width(&s_list_btn_style, 0);
    lv_style_set_outline_width(&s_list_btn_style, 0);
    lv_style_set_pad_hor(&s_list_btn_style, 6);
    lv_style_set_pad_ver(&s_list_btn_style, 2);
    lv_style_set_pad_row(&s_list_btn_style, 0);

    /* Focused Button: Highlight fill, crisp accent border */
    lv_style_init(&s_list_btn_focused_style);
    lv_style_set_bg_opa(&s_list_btn_focused_style, LV_OPA_COVER);
    lv_style_set_bg_color(&s_list_btn_focused_style, focus_bg);
    lv_style_set_border_side(&s_list_btn_focused_style, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_border_width(&s_list_btn_focused_style, 1);
    lv_style_set_border_color(&s_list_btn_focused_style, theme_get()->accent);
    lv_style_set_radius(&s_list_btn_focused_style, 0);
    lv_style_set_shadow_width(&s_list_btn_focused_style, 0);
    lv_style_set_outline_width(&s_list_btn_focused_style, 0);

    /* Labels */
    lv_style_init(&s_list_lbl_style);
    lv_style_set_text_color(&s_list_lbl_style, text_col);
    lv_style_set_text_font(&s_list_lbl_style, veebha_font_get_default());

    lv_style_init(&s_list_lbl_focused_style);
    lv_style_set_text_color(&s_list_lbl_focused_style, focus_text);

    s_styles_initialized = true;
}

static void on_item_clicked(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    uint16_t idx = (uint16_t)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t *scr = lv_obj_get_screen(btn);

    lv_group_t *g = win_mgr_get_group();
    if (g && btn) {
        lv_group_focus_obj(btn);
    }

    if (scr) {
        tpl_list_screen_data_t *data = (tpl_list_screen_data_t *)lv_obj_get_user_data(scr);
        if (data && data->on_select) {
            data->on_select(idx);
        }
    }
}

static void on_list_key_cb(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    lv_group_t *g = win_mgr_get_group();
    if (!g) return;

    if (key == LV_KEY_RIGHT || key == LV_KEY_DOWN) {
        lv_group_focus_next(g);
    } else if (key == LV_KEY_LEFT || key == LV_KEY_UP) {
        lv_group_focus_prev(g);
    }
}

void tpl_list_default_lsk(void)
{
    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_obj_t *focused = lv_group_get_focused(g);
        if (focused && lv_obj_is_valid(focused)) {
            lv_obj_send_event(focused, LV_EVENT_CLICKED, NULL);
        }
    }
}

void tpl_list_default_rsk(void)
{
    win_mgr_entry_t *top = win_mgr_get_top();
    if (top && top->screen && lv_obj_is_valid(top->screen)) {
        win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)lv_obj_get_user_data(top->screen);
        if (hdr && hdr->view_type == VEEBHA_VIEW_TYPE_LIST) {
            tpl_list_screen_data_t *data = (tpl_list_screen_data_t *)hdr;
            if (data->on_back) {
                data->on_back();
                return;
            }
        }
    }
    win_mgr_pop();
}

lv_obj_t * tpl_list_create(const tpl_list_view_t *desc)
{
    if (!desc) return NULL;

    /* 1. Root Screen */
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, CONFIG_DISP_HOR_RES, CONFIG_DISP_VER_RES);
    lv_obj_set_style_bg_color(screen, theme_get()->bg_color, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    /* Allocate screen context */
    tpl_list_screen_data_t *data = (tpl_list_screen_data_t *)calloc(1, sizeof(tpl_list_screen_data_t));
    if (data) {
        data->softkey_bar = NULL;
        data->first_item = NULL;
        data->view_type = VEEBHA_VIEW_TYPE_LIST;
        data->fullscreen_mode = OS_FULLSCREEN_NONE;
        data->show_battery_hud = false;
        data->keep_alive = desc->keep_alive;
        strncpy(data->title, desc->title ? desc->title : "List", sizeof(data->title) - 1);
        data->title[sizeof(data->title) - 1] = '\0';
        data->on_select = desc->on_select;
        data->on_back = desc->on_back;
    }
    lv_obj_set_user_data(screen, data);
    lv_obj_add_event_cb(screen, on_screen_delete_cb, LV_EVENT_DELETE, NULL);

    /* 2. Zone A: Fixed 18px Top Status Bar */
    lv_obj_t *sb = status_bar_create(screen, NULL);
    if (sb) {
        lv_obj_align(sb, LV_ALIGN_TOP_MID, 0, 0);
    }

    /* Dedicated Sub-Screen Header Strip (Height: 18px, Width: 176px at y=18) */
    lv_obj_t *subhdr = lv_obj_create(screen);
    lv_obj_set_size(subhdr, CONFIG_DISP_HOR_RES, 18);
    lv_obj_set_pos(subhdr, 0, 18);
    lv_obj_set_style_bg_color(subhdr, theme_get()->card_color, 0);
    lv_obj_set_style_bg_opa(subhdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(subhdr, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(subhdr, theme_is_light_mode() ? lv_color_hex(0xE2E8F0) : lv_color_hex(0x282C35), 0);
    lv_obj_set_style_border_width(subhdr, 1, 0);
    lv_obj_set_style_radius(subhdr, 0, 0);
    lv_obj_set_style_outline_width(subhdr, 0, 0);
    lv_obj_set_style_pad_all(subhdr, 0, 0);
    lv_obj_remove_flag(subhdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *hdr_lbl = lv_label_create(subhdr);
    lv_label_set_text(hdr_lbl, desc->title ? desc->title : "MENU");
    lv_obj_set_style_text_color(hdr_lbl, theme_get()->accent, 0);
    lv_obj_set_style_text_font(hdr_lbl, veebha_font_get_default(), 0);
    lv_obj_align(hdr_lbl, LV_ALIGN_LEFT_MID, 6, 0);

    /* 3. Zone B: Content Viewport (Height: 164px at y=36) */
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_size(content, CONFIG_DISP_HOR_RES, 164);
    lv_obj_set_pos(content, 0, 36);
    lv_obj_set_style_bg_color(content, theme_get()->bg_color, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_radius(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_set_style_pad_row(content, 0, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    /* Disable center scroll snap completely; rely on LV_OBJ_FLAG_SCROLL_ON_FOCUS */
    lv_obj_set_scroll_snap_y(content, LV_SCROLL_SNAP_NONE);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLL_ANIMATION);

    /* Add Items */
    lv_group_t *group = win_mgr_get_group();
    ensure_list_styles();

    for (uint16_t i = 0; i < desc->count; i++) {
        lv_obj_t *btn = lv_button_create(content);
        lv_obj_remove_style_all(btn);
        lv_obj_add_style(btn, &s_list_btn_style, 0);
        lv_obj_add_style(btn, &s_list_btn_focused_style, LV_STATE_FOCUSED);
        lv_obj_add_style(btn, &s_list_btn_focused_style, LV_STATE_FOCUS_KEY);
        lv_obj_set_size(btn, CONFIG_DISP_HOR_RES, 22);
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLL_ANIMATION);
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

        if (i == 0 && data) {
            data->first_item = btn;
        }
        lv_obj_set_user_data(btn, (void *)(uintptr_t)i);

        /* Title Label: Directly inside button */
        lv_obj_t *lbl = lv_label_create(btn);
        lv_obj_remove_style_all(lbl);
        lv_obj_add_style(lbl, &s_list_lbl_style, 0);
        lv_obj_add_style(lbl, &s_list_lbl_focused_style, LV_STATE_FOCUSED);
        lv_obj_add_style(lbl, &s_list_lbl_focused_style, LV_STATE_FOCUS_KEY);
        lv_label_set_text(lbl, desc->items[i].title ? desc->items[i].title : "");
        lv_obj_set_width(lbl, 164);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 6, 0);

        /* Scroll on focus to ensure off-screen rows scroll into view automatically */
        lv_obj_add_flag(btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

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
    if (bar) {
        lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    }
    if (data) {
        data->softkey_bar = bar;
    }

    return screen;
}