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

#include "app_stopwatch.h"
#include "sdk/include/veebha_status_bar.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_overlays.h"
#include "sdk/include/veebha_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "APP_STOPWATCH"

typedef enum {
    STOPWATCH_MODE_SW = 0,
    STOPWATCH_MODE_TIMER
} sw_mode_t;

typedef struct {
    lv_obj_t          *softkey_bar;
    lv_obj_t          *first_item;
    veebha_view_type_t view_type;
    char               title[WIN_MGR_LABEL_MAX];
    os_fullscreen_mode_t fullscreen_mode;
    bool               show_battery_hud;

    sw_mode_t          mode;
    bool               is_running;

    /* Stopwatch state */
    uint32_t           sw_elapsed_ms;

    /* Countdown Timer state */
    uint32_t           timer_duration_s;
    uint32_t           timer_remaining_ms;

    lv_obj_t          *mode_btn_sw;
    lv_obj_t          *mode_lbl_sw;
    lv_obj_t          *mode_btn_tmr;
    lv_obj_t          *mode_lbl_tmr;

    lv_obj_t          *time_lbl;
    lv_obj_t          *hint_lbl;

    lv_timer_t        *tick_timer;
} stopwatch_screen_data_t;

static lv_obj_t *s_sw_active_scr = NULL;

bool app_stopwatch_is_active(void)
{
    return s_sw_active_scr != NULL;
}

static void update_stopwatch_ui(stopwatch_screen_data_t *data);

static void on_sw_tick(lv_timer_t *tmr)
{
    stopwatch_screen_data_t *data = (stopwatch_screen_data_t *)lv_timer_get_user_data(tmr);
    if (!data || !data->is_running) return;

    if (data->mode == STOPWATCH_MODE_SW) {
        data->sw_elapsed_ms += 100;
        update_stopwatch_ui(data);
    } else {
        if (data->timer_remaining_ms >= 100) {
            data->timer_remaining_ms -= 100;
        } else {
            data->timer_remaining_ms = 0;
            data->is_running = false;
            notif_panel_post_alert("Timer Complete", "Countdown finished: 00:00!");
            OS_LOGI(TAG, "Countdown timer expired! Alert posted to notification panel.");
        }
        update_stopwatch_ui(data);
    }
}

static void on_sw_start_pause(void)
{
    if (!s_sw_active_scr) return;
    stopwatch_screen_data_t *data = (stopwatch_screen_data_t *)lv_obj_get_user_data(s_sw_active_scr);
    if (!data) return;

    data->is_running = !data->is_running;
    update_stopwatch_ui(data);
}

static void on_sw_reset(void)
{
    if (!s_sw_active_scr) return;
    stopwatch_screen_data_t *data = (stopwatch_screen_data_t *)lv_obj_get_user_data(s_sw_active_scr);
    if (!data) return;

    data->is_running = false;
    if (data->mode == STOPWATCH_MODE_SW) {
        data->sw_elapsed_ms = 0;
    } else {
        data->timer_remaining_ms = data->timer_duration_s * 1000;
    }
    update_stopwatch_ui(data);
}

static void on_sw_back(void)
{
    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_set_editing(g, false);
    }
    win_mgr_pop();
}

static void update_stopwatch_ui(stopwatch_screen_data_t *data)
{
    if (!data) return;

    /* Mode badges visual state */
    bool is_sw = (data->mode == STOPWATCH_MODE_SW);
    lv_obj_set_style_bg_color(data->mode_btn_sw, is_sw ? theme_get()->accent : theme_get()->card_color, 0);
    lv_obj_set_style_text_color(data->mode_lbl_sw, is_sw ? lv_color_hex(0x000000) : theme_get()->text_primary, 0);

    lv_obj_set_style_bg_color(data->mode_btn_tmr, !is_sw ? theme_get()->accent : theme_get()->card_color, 0);
    lv_obj_set_style_text_color(data->mode_lbl_tmr, !is_sw ? lv_color_hex(0x000000) : theme_get()->text_primary, 0);

    /* Time display */
    char buf[32];
    if (is_sw) {
        uint32_t total_s = data->sw_elapsed_ms / 1000;
        uint32_t tenths = (data->sw_elapsed_ms % 1000) / 100;
        uint32_t m = total_s / 60;
        uint32_t s = total_s % 60;
        snprintf(buf, sizeof(buf), "%02u:%02u.%u", m, s, tenths);
        lv_label_set_text(data->hint_lbl, "LSK: Start | # or >: Timer");
    } else {
        uint32_t total_s = data->timer_remaining_ms / 1000;
        uint32_t m = total_s / 60;
        uint32_t s = total_s % 60;
        snprintf(buf, sizeof(buf), "%02u:%02u", m, s);
        lv_label_set_text(data->hint_lbl, "D-pad: Set Time | #: SW");
    }
    lv_label_set_text(data->time_lbl, buf);

    /* Softkeys */
    const char *lsk = data->is_running ? "Pause" : "Start";
    bool can_reset = is_sw ? (data->sw_elapsed_ms > 0) : (data->timer_remaining_ms != data->timer_duration_s * 1000);

    if (data->is_running || can_reset) {
        softkey_set_actions(lsk, on_sw_start_pause, "Reset", on_sw_reset);
    } else {
        softkey_set_actions(lsk, on_sw_start_pause, "Back", on_sw_back);
    }
}

void app_stopwatch_toggle_mode(void)
{
    if (!s_sw_active_scr) return;
    stopwatch_screen_data_t *data = (stopwatch_screen_data_t *)lv_obj_get_user_data(s_sw_active_scr);
    if (!data) return;

    data->is_running = false;
    data->mode = (data->mode == STOPWATCH_MODE_SW) ? STOPWATCH_MODE_TIMER : STOPWATCH_MODE_SW;
    update_stopwatch_ui(data);
}

static void on_mode_sw_clicked(lv_event_t *e)
{
    (void)e;
    if (!s_sw_active_scr) return;
    stopwatch_screen_data_t *data = (stopwatch_screen_data_t *)lv_obj_get_user_data(s_sw_active_scr);
    if (!data) return;
    if (data->mode != STOPWATCH_MODE_SW) {
        app_stopwatch_toggle_mode();
    }
}

static void on_mode_tmr_clicked(lv_event_t *e)
{
    (void)e;
    if (!s_sw_active_scr) return;
    stopwatch_screen_data_t *data = (stopwatch_screen_data_t *)lv_obj_get_user_data(s_sw_active_scr);
    if (!data) return;
    if (data->mode != STOPWATCH_MODE_TIMER) {
        app_stopwatch_toggle_mode();
    }
}

static void on_sw_key_cb(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    if (key == '#' || key == 35) {
        app_stopwatch_toggle_mode();
    } else if (key == LV_KEY_UP || key == LV_KEY_PREV || key == '2') {
        app_stopwatch_handle_key(VEEBHA_KEY_UP);
    } else if (key == LV_KEY_DOWN || key == LV_KEY_NEXT || key == '8') {
        app_stopwatch_handle_key(VEEBHA_KEY_DOWN);
    } else if (key == LV_KEY_LEFT || key == '4') {
        app_stopwatch_handle_key(VEEBHA_KEY_LEFT);
    } else if (key == LV_KEY_RIGHT || key == '6') {
        app_stopwatch_handle_key(VEEBHA_KEY_RIGHT);
    } else if (key == LV_KEY_ENTER) {
        app_stopwatch_handle_key(VEEBHA_KEY_OK);
    } else if (key >= '0' && key <= '9') {
        app_stopwatch_handle_key((veebha_key_t)(VEEBHA_KEY_NUM_0 + (key - '0')));
    }
}

void app_stopwatch_handle_key(veebha_key_t key)
{
    if (!s_sw_active_scr) return;
    stopwatch_screen_data_t *data = (stopwatch_screen_data_t *)lv_obj_get_user_data(s_sw_active_scr);
    if (!data) return;

    if (key == VEEBHA_KEY_HASH || key == '#' || key == 35) {
        app_stopwatch_toggle_mode();
        return;
    } else if (key == VEEBHA_KEY_OK) {
        on_sw_start_pause();
    } else if (data->mode == STOPWATCH_MODE_SW) {
        if (key == VEEBHA_KEY_RIGHT || key == VEEBHA_KEY_LEFT) {
            app_stopwatch_toggle_mode();
            return;
        }
    } else if (data->mode == STOPWATCH_MODE_TIMER && !data->is_running) {
        if (key == VEEBHA_KEY_UP) {
            data->timer_duration_s += 60;
            if (data->timer_duration_s > 3600) data->timer_duration_s = 3600;
            data->timer_remaining_ms = data->timer_duration_s * 1000;
            update_stopwatch_ui(data);
        } else if (key == VEEBHA_KEY_DOWN) {
            if (data->timer_duration_s > 60) data->timer_duration_s -= 60;
            else data->timer_duration_s = 10;
            data->timer_remaining_ms = data->timer_duration_s * 1000;
            update_stopwatch_ui(data);
        } else if (key == VEEBHA_KEY_RIGHT) {
            data->timer_duration_s += 10;
            data->timer_remaining_ms = data->timer_duration_s * 1000;
            update_stopwatch_ui(data);
        } else if (key == VEEBHA_KEY_LEFT) {
            if (data->timer_duration_s > 10) data->timer_duration_s -= 10;
            data->timer_remaining_ms = data->timer_duration_s * 1000;
            update_stopwatch_ui(data);
        } else if (key >= VEEBHA_KEY_NUM_0 && key <= VEEBHA_KEY_NUM_9) {
            uint32_t val = (key - VEEBHA_KEY_NUM_0);
            if (val > 0) {
                data->timer_duration_s = val * 60;
                data->timer_remaining_ms = data->timer_duration_s * 1000;
                update_stopwatch_ui(data);
            }
        }
    }
}

static void on_sw_delete_cb(lv_event_t *e)
{
    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_set_editing(g, false);
    }
    lv_obj_t *scr = lv_event_get_target(e);
    if (scr == s_sw_active_scr) {
        s_sw_active_scr = NULL;
    }
    stopwatch_screen_data_t *data = (stopwatch_screen_data_t *)lv_obj_get_user_data(scr);
    if (data) {
        if (data->tick_timer) {
            lv_timer_delete(data->tick_timer);
            data->tick_timer = NULL;
        }
        free(data);
        lv_obj_set_user_data(scr, NULL);
    }
}

lv_obj_t * app_stopwatch_create(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, 176, 220);
    lv_obj_set_style_bg_color(screen, theme_get()->bg_color, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    stopwatch_screen_data_t *data = (stopwatch_screen_data_t *)calloc(1, sizeof(stopwatch_screen_data_t));
    if (!data) {
        lv_obj_del(screen);
        return NULL;
    }

    data->view_type = VEEBHA_VIEW_TYPE_GENERIC;
    strncpy(data->title, "Stopwatch", sizeof(data->title) - 1);
    data->mode = STOPWATCH_MODE_SW;
    data->timer_duration_s = 300; /* 5 min default */
    data->timer_remaining_ms = 300000;

    lv_obj_set_user_data(screen, data);
    lv_obj_add_event_cb(screen, on_sw_delete_cb, LV_EVENT_DELETE, NULL);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, on_sw_key_cb, LV_EVENT_KEY, NULL);
    s_sw_active_scr = screen;

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
    lv_label_set_text(hlbl, "STOPWATCH & TIMER");
    lv_obj_set_style_text_color(hlbl, theme_get()->accent, 0);
    lv_obj_set_style_text_font(hlbl, &lv_font_montserrat_12, 0);

    /* 3. Viewport (Elastic Middle) */
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_size(content, lv_pct(100), 0);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 6, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    /* 3a. Mode Switcher Badges */
    lv_obj_t *mode_bar = lv_obj_create(content);
    lv_obj_set_size(mode_bar, lv_pct(100), 22);
    lv_obj_set_style_bg_opa(mode_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mode_bar, 0, 0);
    lv_obj_set_style_pad_all(mode_bar, 0, 0);
    lv_obj_set_flex_flow(mode_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(mode_bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(mode_bar, LV_OBJ_FLAG_SCROLLABLE);

    data->mode_btn_sw = lv_obj_create(mode_bar);
    lv_obj_set_size(data->mode_btn_sw, 75, 20);
    lv_obj_set_style_radius(data->mode_btn_sw, 4, 0);
    lv_obj_set_style_pad_all(data->mode_btn_sw, 0, 0);
    lv_obj_add_flag(data->mode_btn_sw, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(data->mode_btn_sw, on_mode_sw_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_remove_flag(data->mode_btn_sw, LV_OBJ_FLAG_SCROLLABLE);
    data->mode_lbl_sw = lv_label_create(data->mode_btn_sw);
    lv_label_set_text(data->mode_lbl_sw, "Stopwatch");
    lv_obj_center(data->mode_lbl_sw);
    lv_obj_set_style_text_font(data->mode_lbl_sw, &lv_font_montserrat_10, 0);

    data->mode_btn_tmr = lv_obj_create(mode_bar);
    lv_obj_set_size(data->mode_btn_tmr, 75, 20);
    lv_obj_set_style_radius(data->mode_btn_tmr, 4, 0);
    lv_obj_set_style_pad_all(data->mode_btn_tmr, 0, 0);
    lv_obj_add_flag(data->mode_btn_tmr, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(data->mode_btn_tmr, on_mode_tmr_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_remove_flag(data->mode_btn_tmr, LV_OBJ_FLAG_SCROLLABLE);
    data->mode_lbl_tmr = lv_label_create(data->mode_btn_tmr);
    lv_label_set_text(data->mode_lbl_tmr, "Timer");
    lv_obj_center(data->mode_lbl_tmr);
    lv_obj_set_style_text_font(data->mode_lbl_tmr, &lv_font_montserrat_10, 0);

    /* 3b. Digital Clock Card */
    lv_obj_t *card = lv_obj_create(content);
    lv_obj_set_size(card, lv_pct(100), 54);
    lv_obj_set_style_bg_color(card, theme_get()->card_color, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, theme_is_light_mode() ? lv_color_hex(0xE2E8F0) : lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 6, 0);
    lv_obj_set_style_pad_all(card, 4, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    data->time_lbl = lv_label_create(card);
    lv_label_set_text(data->time_lbl, "00:00.0");
    lv_obj_center(data->time_lbl);
    lv_obj_set_style_text_font(data->time_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(data->time_lbl, theme_get()->accent, 0);

    /* 3c. Hint label */
    data->hint_lbl = lv_label_create(content);
    lv_label_set_text(data->hint_lbl, "Press LSK to Start / Pause");
    lv_obj_set_style_text_font(data->hint_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(data->hint_lbl, lv_color_hex(0x8B949E), 0);

    /* 4. Bottom Softkey Bar */
    data->softkey_bar = softkey_bar_create(screen, "Start", "Back");
    softkey_set_actions("Start", on_sw_start_pause, "Back", on_sw_back);

    /* Tick timer (100ms) */
    data->tick_timer = lv_timer_create(on_sw_tick, 100, data);

    update_stopwatch_ui(data);
    return screen;
}

void app_stopwatch_init(void)
{
    OS_LOGI(TAG, "Stopwatch & Timer module initialized");
}

void app_stopwatch_open(void)
{
    lv_obj_t *scr = app_stopwatch_create();
    if (scr) {
        win_mgr_push(scr, "Start", on_sw_start_pause, "Back", on_sw_back);
        lv_group_t *g = win_mgr_get_group();
        if (g) {
            lv_group_add_obj(g, scr);
            lv_group_focus_obj(scr);
            lv_group_set_editing(g, true);
        }
    }
}
