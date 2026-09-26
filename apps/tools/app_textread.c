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

#include "app_textread.h"
#include "sdk/include/veebha_status_bar.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_templates.h"
#include "sdk/include/veebha_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "APP_TEXTREAD"

typedef struct {
    lv_obj_t          *softkey_bar;
    lv_obj_t          *first_item;
    veebha_view_type_t view_type;
    char               title[WIN_MGR_LABEL_MAX];
    os_fullscreen_mode_t fullscreen_mode;
    bool               show_battery_hud;
    bool               keep_alive;

    char               filename[32];
    lv_obj_t          *scroll_box;
    lv_obj_t          *content_lbl;
} textread_screen_data_t;

static const char *s_default_readme =
    "=== VeebhaOS Document Viewer ===\n\n"
    "Welcome to VeebhaOS v4.3!\n"
    "Target: RDA8809 / Linux Simulator\n"
    "Display: 176x220 RGB565\n\n"
    "Architecture Features:\n"
    "- Zero-Coordinate UI Templates\n"
    "- 3x3 Modular App Launcher\n"
    "- Asynchronous Event Dispatcher\n"
    "- Virtual File System (/sdcard)\n"
    "- Walkman Music & Media Hub\n"
    "- Keypad Calculator & Tools Hub\n\n"
    "Keyboard Navigation:\n"
    "- Arrow Keys: D-pad navigation\n"
    "- F1 / LSK: Menu / Options\n"
    "- F2 / RSK: Back / Clear\n"
    "- C key (Hold): Quick Settings\n"
    "- * key (Hold): Task Switcher\n\n"
    "Enjoy developing on VeebhaOS!";

static void on_textread_back(void)
{
    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_set_editing(g, false);
    }
    win_mgr_pop();
}

static void on_textread_dialog_close(void)
{
    tpl_dialog_close();
}

static void on_textread_info(void)
{
    static tpl_dialog_desc_t dlg = {
        .title = "Doc Info",
        .icon = LV_SYMBOL_FILE,
        .message = "Format: Plain Text (UTF-8)\nEncoding: ANSI / ASCII\nStatus: Read-Only",
        .lsk_label = "OK",
        .rsk_label = "Back",
        .on_confirm = on_textread_dialog_close,
        .on_cancel = on_textread_dialog_close
    };
    tpl_dialog_show(&dlg);
}

static void on_textread_key_cb(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *scr = lv_obj_get_screen(target);
    textread_screen_data_t *data = (textread_screen_data_t *)lv_obj_get_user_data(scr);
    if (!data || !data->scroll_box) return;

    if (key == LV_KEY_UP || key == LV_KEY_PREV || key == '2') {
        lv_obj_scroll_by_bounded(data->scroll_box, 0, 24, LV_ANIM_OFF);
    } else if (key == LV_KEY_DOWN || key == LV_KEY_NEXT || key == '8') {
        lv_obj_scroll_by_bounded(data->scroll_box, 0, -24, LV_ANIM_OFF);
    } else if (key == '0' || key == LV_KEY_ESC || key == LV_KEY_BACKSPACE) {
        on_textread_back();
    }
}

static void on_textread_delete_cb(lv_event_t *e)
{
    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_set_editing(g, false);
    }
    lv_obj_t *scr = lv_event_get_target(e);
    textread_screen_data_t *data = (textread_screen_data_t *)lv_obj_get_user_data(scr);
    if (data) {
        free(data);
        lv_obj_set_user_data(scr, NULL);
    }
}

lv_obj_t * app_textread_create_for_file(const char *filepath)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, 176, 220);
    lv_obj_set_style_bg_color(screen, theme_get()->bg_color, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    textread_screen_data_t *data = (textread_screen_data_t *)calloc(1, sizeof(textread_screen_data_t));
    if (!data) {
        lv_obj_del(screen);
        return NULL;
    }

    data->view_type = VEEBHA_VIEW_TYPE_GENERIC;
    strncpy(data->title, "Reader", sizeof(data->title) - 1);
    data->fullscreen_mode = OS_FULLSCREEN_FULL;
    data->show_battery_hud = true;
    if (filepath) {
        const char *slash = strrchr(filepath, '/');
        strncpy(data->filename, slash ? slash + 1 : filepath, sizeof(data->filename) - 1);
    } else {
        strncpy(data->filename, "ReadMe.txt", sizeof(data->filename) - 1);
    }

    lv_obj_set_user_data(screen, data);
    lv_obj_add_event_cb(screen, on_textread_delete_cb, LV_EVENT_DELETE, NULL);

    /* 1. Zone A: Status Bar (will be hidden by win_mgr in full mode) */
    status_bar_create(screen, NULL);

    /* 2. Header Strip (18px) - hide in full screen mode */
    lv_obj_t *hdr = lv_obj_create(screen);
    lv_obj_set_size(hdr, lv_pct(100), 18);
    lv_obj_set_style_bg_color(hdr, theme_get()->card_color, 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(hdr, theme_is_light_mode() ? lv_color_hex(0xE2E8F0) : lv_color_hex(0x282C35), 0);
    lv_obj_set_style_border_width(hdr, 1, 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(hdr, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *hlbl = lv_label_create(hdr);
    char hbuf[48];
    snprintf(hbuf, sizeof(hbuf), "DOC: %s", data->filename);
    lv_label_set_text(hlbl, hbuf);
    lv_obj_set_style_text_color(hlbl, theme_get()->accent, 0);
    lv_obj_set_style_text_font(hlbl, &lv_font_montserrat_10, 0);

    /* 3. Viewport: Scrollable Text Container */
    lv_obj_t *scroll_box = lv_obj_create(screen);
    data->scroll_box = scroll_box;
    data->first_item = scroll_box;
    lv_obj_set_size(scroll_box, lv_pct(100), 0);
    lv_obj_set_flex_grow(scroll_box, 1);
    lv_obj_set_style_bg_color(scroll_box, theme_get()->bg_color, 0);
    lv_obj_set_style_bg_opa(scroll_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(scroll_box, 0, 0);
    lv_obj_set_style_radius(scroll_box, 0, 0);
    lv_obj_set_style_pad_all(scroll_box, 6, 0);
    lv_obj_add_flag(scroll_box, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_scrollbar_mode(scroll_box, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_event_cb(scroll_box, on_textread_key_cb, LV_EVENT_KEY, NULL);

    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, on_textread_key_cb, LV_EVENT_KEY, NULL);

    lv_obj_t *content_lbl = lv_label_create(scroll_box);
    data->content_lbl = content_lbl;
    lv_label_set_text(content_lbl, s_default_readme);
    lv_obj_set_width(content_lbl, 160);
    lv_label_set_long_mode(content_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(content_lbl, theme_get()->text_primary, 0);
    lv_obj_set_style_text_font(content_lbl, &lv_font_montserrat_10, 0);

    lv_group_t *grp = win_mgr_get_group();
    if (grp) {
        lv_group_add_obj(grp, scroll_box);
    }
    data->first_item = scroll_box;

    /* 4. Bottom Softkey Bar */
    data->softkey_bar = softkey_bar_create(screen, "Info", "Back");
    softkey_set_actions("Info", on_textread_info, "Back", on_textread_back);

    return screen;
}

void app_textread_init(void)
{
    OS_LOGI(TAG, "Text Reader module initialized");
}

void app_textread_open_file(const char *filepath)
{
    lv_obj_t *scr = app_textread_create_for_file(filepath);
    if (scr) {
        win_mgr_push(scr, "Info", on_textread_info, "Back", on_textread_back);
        win_mgr_set_fullscreen_mode(scr, OS_FULLSCREEN_FULL, true);
        lv_group_t *g = win_mgr_get_group();
        if (g) {
            textread_screen_data_t *d = (textread_screen_data_t *)lv_obj_get_user_data(scr);
            if (d && d->scroll_box) {
                lv_group_focus_obj(d->scroll_box);
            }
            lv_group_set_editing(g, true);
        }
    }
}

void app_textread_open(void)
{
    app_textread_open_file("/sdcard/Documents/ReadMe.txt");
}
