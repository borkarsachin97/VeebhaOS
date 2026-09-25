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

#include "app_alarm.h"
#include "sdk/include/veebha_status_bar.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_overlays.h"
#include "sdk/include/veebha_log.h"
#include "sdk/storage/os_nvram.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "APP_ALARM"

typedef struct {
    lv_obj_t          *softkey_bar;
    lv_obj_t          *first_item;
    veebha_view_type_t view_type;
    char               title[WIN_MGR_LABEL_MAX];
    os_fullscreen_mode_t fullscreen_mode;
    bool               show_battery_hud;

    uint8_t            hour;    /* 1..12 */
    uint8_t            minute;  /* 0..59 */
    bool               is_pm;
    bool               is_enabled;

    uint8_t            focus_col; /* 0: hour, 1: minute, 2: am_pm */

    lv_obj_t          *hour_lbl;
    lv_obj_t          *minute_lbl;
    lv_obj_t          *ampm_lbl;
    lv_obj_t          *status_lbl;
} alarm_screen_data_t;

static uint8_t s_alarm_hour = 7;
static uint8_t s_alarm_minute = 0;
static bool s_alarm_pm = false;
static bool s_alarm_enabled = false;
static char s_alarm_time_str[16] = "07:00 AM";

bool app_alarm_is_enabled(void)
{
    return s_alarm_enabled;
}

const char * app_alarm_get_time_str(void)
{
    return s_alarm_time_str;
}

static void update_alarm_ui(alarm_screen_data_t *data)
{
    if (!data) return;

    char hbuf[8], mbuf[8];
    snprintf(hbuf, sizeof(hbuf), "%02u", data->hour);
    snprintf(mbuf, sizeof(mbuf), "%02u", data->minute);

    lv_label_set_text(data->hour_lbl, hbuf);
    lv_label_set_text(data->minute_lbl, mbuf);
    lv_label_set_text(data->ampm_lbl, data->is_pm ? "PM" : "AM");

    /* Focus highlight on active column */
    lv_obj_set_style_text_color(data->hour_lbl, (data->focus_col == 0) ? theme_get()->accent : theme_get()->text_primary, 0);
    lv_obj_set_style_text_color(data->minute_lbl, (data->focus_col == 1) ? theme_get()->accent : theme_get()->text_primary, 0);
    lv_obj_set_style_text_color(data->ampm_lbl, (data->focus_col == 2) ? theme_get()->accent : theme_get()->text_primary, 0);

    if (data->is_enabled) {
        lv_label_set_text(data->status_lbl, "Status: ENABLED (Bell On)");
        lv_obj_set_style_text_color(data->status_lbl, theme_get()->accent, 0);
    } else {
        lv_label_set_text(data->status_lbl, "Status: DISABLED (Off)");
        lv_obj_set_style_text_color(data->status_lbl, lv_color_hex(0x8B949E), 0);
    }

    snprintf(s_alarm_time_str, sizeof(s_alarm_time_str), "%02u:%02u %s",
             data->hour, data->minute, data->is_pm ? "PM" : "AM");
    s_alarm_hour = data->hour;
    s_alarm_minute = data->minute;
    s_alarm_pm = data->is_pm;
    s_alarm_enabled = data->is_enabled;
}

static void on_alarm_save_action(void)
{
    lv_obj_t *top = lv_scr_act();
    alarm_screen_data_t *data = (alarm_screen_data_t *)lv_obj_get_user_data(top);
    if (!data) return;

    data->is_enabled = true;
    char msg[64];
    snprintf(msg, sizeof(msg), "Alarm set for %s", s_alarm_time_str);
    notif_panel_post_alert("Alarm Clock", msg);
    OS_LOGI(TAG, "%s", msg);
    update_alarm_ui(data);
    status_bar_set_alarm(data->is_enabled);

    os_nvram_data_t *nv = os_nvram_get();
    if (nv) {
        nv->alarm_hour = data->hour;
        nv->alarm_min = data->minute;
        nv->alarm_pm = data->is_pm;
        nv->alarm_enabled = data->is_enabled;
        os_nvram_save();
    }
}

static void on_alarm_back_action(void)
{
    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_set_editing(g, false);
    }
    win_mgr_pop();
}

static void on_alarm_key_cb(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    lv_obj_t *scr = lv_obj_get_screen(lv_event_get_target(e));
    alarm_screen_data_t *data = (alarm_screen_data_t *)lv_obj_get_user_data(scr);
    if (!data) return;

    if (key == LV_KEY_RIGHT || key == '6') {
        data->focus_col = (data->focus_col + 1) % 3;
        update_alarm_ui(data);
    } else if (key == LV_KEY_LEFT || key == '4') {
        data->focus_col = (data->focus_col + 2) % 3;
        update_alarm_ui(data);
    } else if (key == LV_KEY_UP || key == LV_KEY_PREV || key == '2') {
        if (data->focus_col == 0) {
            data->hour = (data->hour % 12) + 1;
        } else if (data->focus_col == 1) {
            data->minute = (data->minute + 5) % 60;
        } else {
            data->is_pm = !data->is_pm;
        }
        update_alarm_ui(data);
    } else if (key == LV_KEY_DOWN || key == LV_KEY_NEXT || key == '8') {
        if (data->focus_col == 0) {
            data->hour = (data->hour == 1) ? 12 : (data->hour - 1);
        } else if (data->focus_col == 1) {
            data->minute = (data->minute < 5) ? 55 : (data->minute - 5);
        } else {
            data->is_pm = !data->is_pm;
        }
        update_alarm_ui(data);
    }
}

static void on_alarm_delete_cb(lv_event_t *e)
{
    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_set_editing(g, false);
    }
    lv_obj_t *scr = lv_event_get_target(e);
    alarm_screen_data_t *data = (alarm_screen_data_t *)lv_obj_get_user_data(scr);
    if (data) {
        free(data);
        lv_obj_set_user_data(scr, NULL);
    }
}

lv_obj_t * app_alarm_create(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, 176, 220);
    lv_obj_set_style_bg_color(screen, theme_get()->bg_color, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    alarm_screen_data_t *data = (alarm_screen_data_t *)calloc(1, sizeof(alarm_screen_data_t));
    if (!data) {
        lv_obj_del(screen);
        return NULL;
    }

    data->view_type = VEEBHA_VIEW_TYPE_GENERIC;
    strncpy(data->title, "Alarm", sizeof(data->title) - 1);
    data->hour = s_alarm_hour;
    data->minute = s_alarm_minute;
    data->is_pm = s_alarm_pm;
    data->is_enabled = s_alarm_enabled;

    lv_obj_set_user_data(screen, data);
    lv_obj_add_event_cb(screen, on_alarm_delete_cb, LV_EVENT_DELETE, NULL);

    /* 1. Zone A: Fixed 18px Top Status Bar */
    status_bar_create(screen, NULL);

    /* 2. Header Strip (18px) */
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

    lv_obj_t *hlbl = lv_label_create(hdr);
    lv_label_set_text(hlbl, "ALARM CLOCK");
    lv_obj_set_style_text_color(hlbl, theme_get()->accent, 0);
    lv_obj_set_style_text_font(hlbl, &lv_font_montserrat_12, 0);

    /* 3. Viewport */
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_size(content, lv_pct(100), 0);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 8, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    /* Time Picker Card */
    lv_obj_t *card = lv_button_create(content);
    data->first_item = card;
    lv_obj_set_size(card, lv_pct(100), 56);
    lv_obj_set_style_bg_color(card, theme_get()->card_color, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, theme_is_light_mode() ? lv_color_hex(0xE2E8F0) : lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 6, 0);
    lv_obj_set_style_pad_all(card, 4, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Highlight border when focused */
    lv_obj_set_style_border_color(card, theme_get()->accent, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(card, 2, LV_STATE_FOCUSED);

    data->hour_lbl = lv_label_create(card);
    lv_label_set_text(data->hour_lbl, "07");
    lv_obj_set_style_text_font(data->hour_lbl, &lv_font_montserrat_16, 0);

    lv_obj_t *colon = lv_label_create(card);
    lv_label_set_text(colon, ":");
    lv_obj_set_style_text_font(colon, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(colon, theme_get()->text_primary, 0);

    data->minute_lbl = lv_label_create(card);
    lv_label_set_text(data->minute_lbl, "00");
    lv_obj_set_style_text_font(data->minute_lbl, &lv_font_montserrat_16, 0);

    lv_obj_t *sp = lv_label_create(card);
    lv_label_set_text(sp, " ");

    data->ampm_lbl = lv_label_create(card);
    lv_label_set_text(data->ampm_lbl, "AM");
    lv_obj_set_style_text_font(data->ampm_lbl, &lv_font_montserrat_14, 0);

    lv_obj_add_event_cb(card, on_alarm_key_cb, LV_EVENT_KEY, NULL);

    lv_group_t *grp = win_mgr_get_group();
    if (grp) {
        lv_group_add_obj(grp, card);
    }
    data->first_item = card;

    /* Status Label */
    data->status_lbl = lv_label_create(content);
    lv_label_set_text(data->status_lbl, "Status: ENABLED (Bell On)");
    lv_obj_set_style_text_font(data->status_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(data->status_lbl, theme_get()->accent, 0);

    /* Hint Label */
    lv_obj_t *hint = lv_label_create(content);
    lv_label_set_text(hint, "Left/Right: Select\nUp/Down: Adjust");
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);

    /* 4. Bottom Softkey Bar */
    data->softkey_bar = softkey_bar_create(screen, "Save", "Back");
    softkey_set_actions("Save", on_alarm_save_action, "Back", on_alarm_back_action);

    update_alarm_ui(data);
    return screen;
}

void app_alarm_init(void)
{
    os_nvram_data_t *nv = os_nvram_get();
    if (nv) {
        if (nv->alarm_hour >= 1 && nv->alarm_hour <= 12) s_alarm_hour = nv->alarm_hour;
        if (nv->alarm_min < 60) s_alarm_minute = nv->alarm_min;
        s_alarm_pm = nv->alarm_pm;
        s_alarm_enabled = nv->alarm_enabled;
        snprintf(s_alarm_time_str, sizeof(s_alarm_time_str), "%02u:%02u %s",
                 s_alarm_hour, s_alarm_minute, s_alarm_pm ? "PM" : "AM");
        status_bar_set_alarm(s_alarm_enabled);
    }
    OS_LOGI(TAG, "Alarm Clock module initialized (%s, enabled=%d)", s_alarm_time_str, (int)s_alarm_enabled);
}

void app_alarm_open(void)
{
    lv_obj_t *scr = app_alarm_create();
    if (scr) {
        win_mgr_push(scr, "Save", on_alarm_save_action, "Back", on_alarm_back_action);
        lv_group_t *g = win_mgr_get_group();
        if (g) {
            alarm_screen_data_t *d = (alarm_screen_data_t *)lv_obj_get_user_data(scr);
            if (d && d->first_item) {
                lv_group_focus_obj(d->first_item);
            }
            lv_group_set_editing(g, true);
        }
    }
}
