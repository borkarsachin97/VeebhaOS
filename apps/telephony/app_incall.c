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

#include "apps/telephony/app_incall.h"
#include "apps/home/app_idle.h"
#include "sdk/include/veebha_live_pill.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_status_bar.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_log.h"
#include "boards/board_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "APP_INCALL"
#define DTMF_MAX_LEN 24

typedef struct {
    lv_obj_t          *softkey_bar;
    lv_obj_t          *first_item;
    veebha_view_type_t view_type;
    char               title[WIN_MGR_LABEL_MAX];
    os_fullscreen_mode_t fullscreen_mode;
    bool               show_battery_hud;

    lv_obj_t          *status_hdr_lbl;
    lv_obj_t          *caller_name_lbl;
    lv_obj_t          *caller_num_lbl;
    lv_obj_t          *duration_lbl;
    lv_obj_t          *dtmf_card;
    lv_obj_t          *dtmf_lbl;
    lv_obj_t          *mute_badge;
    lv_obj_t          *speaker_badge;
} incall_ui_t;

/* Global Call Session State */
static incall_state_t s_call_state = INCALL_STATE_IDLE;
static call_type_t    s_call_type = CALL_TYPE_OUTGOING;
static char           s_caller_name[32] = {0};
static char           s_caller_number[24] = {0};
static char           s_dtmf_buf[DTMF_MAX_LEN + 1] = {0};
static uint32_t       s_call_seconds = 0;
static char           s_duration_str[16] = "Calling...";
static bool           s_is_muted = false;
static bool           s_is_speaker = false;

static lv_timer_t    *s_call_timer = NULL;
static lv_obj_t      *s_incall_screen = NULL;
static incall_ui_t   *s_incall_ui = NULL;

static void update_incall_ui(void)
{
    if (!s_incall_ui || !s_incall_screen || !lv_obj_is_valid(s_incall_screen)) return;

    /* 1. Subheader status */
    if (s_incall_ui->status_hdr_lbl && lv_obj_is_valid(s_incall_ui->status_hdr_lbl)) {
        if (s_call_state == INCALL_STATE_CALLING) {
            lv_label_set_text(s_incall_ui->status_hdr_lbl, (s_call_type == CALL_TYPE_INCOMING) ? "INCOMING CALL" : "CALLING...");
            lv_obj_set_style_text_color(s_incall_ui->status_hdr_lbl, theme_get()->accent, 0);
        } else if (s_call_state == INCALL_STATE_CONNECTED) {
            lv_label_set_text(s_incall_ui->status_hdr_lbl, "CONNECTED");
            lv_obj_set_style_text_color(s_incall_ui->status_hdr_lbl, lv_color_hex(0x00E676), 0);
        } else {
            lv_label_set_text(s_incall_ui->status_hdr_lbl, "CALL ENDED");
            lv_obj_set_style_text_color(s_incall_ui->status_hdr_lbl, lv_color_hex(0xEF4444), 0);
        }
    }

    /* 2. Caller Info */
    if (s_incall_ui->caller_name_lbl && lv_obj_is_valid(s_incall_ui->caller_name_lbl)) {
        lv_label_set_text(s_incall_ui->caller_name_lbl, s_caller_name[0] ? s_caller_name : s_caller_number);
    }

    if (s_incall_ui->caller_num_lbl && lv_obj_is_valid(s_incall_ui->caller_num_lbl)) {
        char type_buf[48];
        snprintf(type_buf, sizeof(type_buf), "Mobile | %s", s_caller_number);
        lv_label_set_text(s_incall_ui->caller_num_lbl, type_buf);
    }

    /* 3. Duration Label */
    if (s_incall_ui->duration_lbl && lv_obj_is_valid(s_incall_ui->duration_lbl)) {
        if (s_call_state == INCALL_STATE_CALLING) {
            lv_label_set_text(s_incall_ui->duration_lbl, (s_call_type == CALL_TYPE_INCOMING) ? "Incoming..." : "Calling...");
        } else {
            uint32_t mins = s_call_seconds / 60;
            uint32_t secs = s_call_seconds % 60;
            snprintf(s_duration_str, sizeof(s_duration_str), "%02u:%02u", mins, secs);
            lv_label_set_text(s_incall_ui->duration_lbl, s_duration_str);
        }
    }

    /* 4. DTMF Strip */
    if (s_incall_ui->dtmf_card && lv_obj_is_valid(s_incall_ui->dtmf_card)) {
        if (s_dtmf_buf[0] != '\0') {
            char dbuf[48];
            snprintf(dbuf, sizeof(dbuf), "DTMF: %s", s_dtmf_buf);
            if (s_incall_ui->dtmf_lbl && lv_obj_is_valid(s_incall_ui->dtmf_lbl)) {
                lv_label_set_text(s_incall_ui->dtmf_lbl, dbuf);
            }
            lv_obj_clear_flag(s_incall_ui->dtmf_card, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_incall_ui->dtmf_card, LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* 5. Audio State Badges */
    if (s_incall_ui->mute_badge && lv_obj_is_valid(s_incall_ui->mute_badge)) {
        if (s_is_muted) {
            lv_obj_clear_flag(s_incall_ui->mute_badge, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_incall_ui->mute_badge, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (s_incall_ui->speaker_badge && lv_obj_is_valid(s_incall_ui->speaker_badge)) {
        if (s_is_speaker) {
            lv_obj_clear_flag(s_incall_ui->speaker_badge, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_incall_ui->speaker_badge, LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* 6. Softkeys */
    if (s_call_state != INCALL_STATE_ENDED) {
        softkey_set_actions(s_is_muted ? "Unmute" : "Mute", app_incall_toggle_mute, "End", app_incall_end);
    } else {
        softkey_set_actions("", NULL, "", NULL);
    }
}

static void on_call_timer_tick(lv_timer_t *timer)
{
    (void)timer;
    if (s_call_state == INCALL_STATE_IDLE || s_call_state == INCALL_STATE_ENDED) return;

    if (s_call_state == INCALL_STATE_CALLING) {
        s_call_state = INCALL_STATE_CONNECTED;
        s_call_seconds = 0;
        snprintf(s_duration_str, sizeof(s_duration_str), "00:00");
    } else {
        s_call_seconds++;
        uint32_t mins = s_call_seconds / 60;
        uint32_t secs = s_call_seconds % 60;
        snprintf(s_duration_str, sizeof(s_duration_str), "%02u:%02u", mins, secs);
    }

    update_incall_ui();
    char pill_buf[48];
    snprintf(pill_buf, sizeof(pill_buf), "In Call (%s)", s_duration_str);
    live_pill_publish(LIVE_PILL_PRIO_CALL, LV_SYMBOL_CALL, pill_buf, lv_color_hex(0x00E676), app_incall_show);
    app_idle_update();
}

static void on_incall_screen_deleted(lv_event_t *e)
{
    (void)e;
    s_incall_screen = NULL;
    if (s_incall_ui) {
        free(s_incall_ui);
        s_incall_ui = NULL;
    }
    OS_LOGD(TAG, "In-Call screen destroyed (Call active: %d)", (int)app_incall_is_active());
}

static lv_obj_t * create_incall_screen(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    if (!scr) return NULL;

    incall_ui_t *ui = (incall_ui_t *)calloc(1, sizeof(incall_ui_t));
    if (!ui) {
        lv_obj_delete(scr);
        return NULL;
    }

    s_incall_screen = scr;
    s_incall_ui = ui;

    ui->view_type = VEEBHA_VIEW_TYPE_MEDIA;
    strncpy(ui->title, "In Call", sizeof(ui->title) - 1);
    ui->fullscreen_mode = OS_FULLSCREEN_NONE;
    ui->show_battery_hud = false;

    lv_obj_set_size(scr, CONFIG_DISP_HOR_RES, CONFIG_DISP_VER_RES);
    lv_obj_set_style_bg_color(scr, theme_get()->bg_color, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_user_data(scr, ui);
    lv_obj_add_event_cb(scr, on_incall_screen_deleted, LV_EVENT_DELETE, NULL);

    /* 1. Zone A: Fixed 18px Top Status Bar */
    status_bar_create(scr, "Call");

    /* 2. Sub-Screen Header Strip (18px) */
    lv_obj_t *subhdr = lv_obj_create(scr);
    lv_obj_set_size(subhdr, lv_pct(100), 18);
    lv_obj_set_style_bg_color(subhdr, theme_get()->card_color, 0);
    lv_obj_set_style_bg_opa(subhdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(subhdr, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(subhdr, theme_is_light_mode() ? lv_color_hex(0xE2E8F0) : lv_color_hex(0x282C35), 0);
    lv_obj_set_style_border_width(subhdr, 1, 0);
    lv_obj_set_style_pad_all(subhdr, 0, 0);
    lv_obj_set_flex_flow(subhdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(subhdr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(subhdr, LV_OBJ_FLAG_SCROLLABLE);

    ui->status_hdr_lbl = lv_label_create(subhdr);
    lv_label_set_text(ui->status_hdr_lbl, "CALLING...");
    lv_obj_set_style_text_color(ui->status_hdr_lbl, theme_get()->accent, 0);
    lv_obj_set_style_text_font(ui->status_hdr_lbl, &lv_font_montserrat_12, 0);

    /* 3. Zone B: Viewport (184px) */
    lv_obj_t *viewport = lv_obj_create(scr);
    lv_obj_set_size(viewport, lv_pct(100), 0);
    lv_obj_set_flex_grow(viewport, 1);
    lv_obj_set_style_bg_opa(viewport, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(viewport, 0, 0);
    lv_obj_set_style_pad_hor(viewport, 8, 0);
    lv_obj_set_style_pad_ver(viewport, 4, 0);
    lv_obj_set_flex_flow(viewport, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(viewport, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(viewport, LV_OBJ_FLAG_SCROLLABLE);

    /* Call Avatar Icon Badge */
    lv_obj_t *avatar_box = lv_obj_create(viewport);
    lv_obj_set_size(avatar_box, 36, 36);
    lv_obj_set_style_bg_color(avatar_box, lv_color_hex(0x0F291E), 0);
    lv_obj_set_style_bg_opa(avatar_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(avatar_box, lv_color_hex(0x10B981), 0);
    lv_obj_set_style_border_width(avatar_box, 1, 0);
    lv_obj_set_style_radius(avatar_box, 18, 0);
    lv_obj_set_style_pad_all(avatar_box, 0, 0);
    lv_obj_remove_flag(avatar_box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *call_icon = lv_label_create(avatar_box);
    lv_label_set_text(call_icon, LV_SYMBOL_CALL);
    lv_obj_set_style_text_color(call_icon, lv_color_hex(0x34D399), 0);
    lv_obj_set_style_text_font(call_icon, &lv_font_montserrat_16, 0);
    lv_obj_center(call_icon);

    /* Caller Name & Type Container */
    lv_obj_t *info_box = lv_obj_create(viewport);
    lv_obj_set_size(info_box, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(info_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(info_box, 0, 0);
    lv_obj_set_style_pad_all(info_box, 0, 0);
    lv_obj_set_flex_flow(info_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(info_box, LV_OBJ_FLAG_SCROLLABLE);

    ui->caller_name_lbl = lv_label_create(info_box);
    lv_obj_set_width(ui->caller_name_lbl, lv_pct(100));
    lv_obj_set_style_text_align(ui->caller_name_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(ui->caller_name_lbl, LV_LABEL_LONG_DOT);
    lv_label_set_text(ui->caller_name_lbl, s_caller_name[0] ? s_caller_name : s_caller_number);
    lv_obj_set_style_text_color(ui->caller_name_lbl, theme_get()->text_primary, 0);
    lv_obj_set_style_text_font(ui->caller_name_lbl, &lv_font_montserrat_14, 0);

    ui->caller_num_lbl = lv_label_create(info_box);
    lv_obj_set_width(ui->caller_num_lbl, lv_pct(100));
    lv_obj_set_style_text_align(ui->caller_num_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(ui->caller_num_lbl, LV_LABEL_LONG_DOT);
    lv_label_set_text(ui->caller_num_lbl, s_caller_number);
    lv_obj_set_style_text_color(ui->caller_num_lbl, theme_get()->text_muted, 0);
    lv_obj_set_style_text_font(ui->caller_num_lbl, &lv_font_montserrat_10, 0);

    /* Duration Counter Label */
    ui->duration_lbl = lv_label_create(viewport);
    lv_obj_set_width(ui->duration_lbl, lv_pct(100));
    lv_obj_set_style_text_align(ui->duration_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(ui->duration_lbl, s_duration_str);
    lv_obj_set_style_text_color(ui->duration_lbl, theme_get()->accent, 0);
    lv_obj_set_style_text_font(ui->duration_lbl, &lv_font_montserrat_16, 0);

    /* DTMF Entry Strip */
    ui->dtmf_card = lv_obj_create(viewport);
    lv_obj_set_size(ui->dtmf_card, 150, 20);
    lv_obj_set_style_bg_color(ui->dtmf_card, theme_get()->card_color, 0);
    lv_obj_set_style_bg_opa(ui->dtmf_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ui->dtmf_card, theme_get()->accent, 0);
    lv_obj_set_style_border_width(ui->dtmf_card, 1, 0);
    lv_obj_set_style_radius(ui->dtmf_card, 4, 0);
    lv_obj_set_style_pad_hor(ui->dtmf_card, 6, 0);
    lv_obj_set_style_pad_ver(ui->dtmf_card, 1, 0);
    lv_obj_remove_flag(ui->dtmf_card, LV_OBJ_FLAG_SCROLLABLE);

    ui->dtmf_lbl = lv_label_create(ui->dtmf_card);
    lv_label_set_text(ui->dtmf_lbl, "DTMF: ");
    lv_obj_set_style_text_color(ui->dtmf_lbl, theme_get()->accent, 0);
    lv_obj_set_style_text_font(ui->dtmf_lbl, &lv_font_montserrat_10, 0);
    lv_obj_center(ui->dtmf_lbl);
    lv_obj_add_flag(ui->dtmf_card, LV_OBJ_FLAG_HIDDEN);

    /* Audio State Badges Row */
    lv_obj_t *badge_row = lv_obj_create(viewport);
    lv_obj_set_size(badge_row, 150, 18);
    lv_obj_set_style_bg_opa(badge_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(badge_row, 0, 0);
    lv_obj_set_style_pad_all(badge_row, 0, 0);
    lv_obj_set_flex_flow(badge_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(badge_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(badge_row, LV_OBJ_FLAG_SCROLLABLE);

    ui->mute_badge = lv_label_create(badge_row);
    lv_label_set_text(ui->mute_badge, LV_SYMBOL_MUTE " Muted  ");
    lv_obj_set_style_text_color(ui->mute_badge, lv_color_hex(0xFF9800), 0);
    lv_obj_set_style_text_font(ui->mute_badge, &lv_font_montserrat_10, 0);
    lv_obj_add_flag(ui->mute_badge, LV_OBJ_FLAG_HIDDEN);

    ui->speaker_badge = lv_label_create(badge_row);
    lv_label_set_text(ui->speaker_badge, LV_SYMBOL_VOLUME_MAX " Speaker");
    lv_obj_set_style_text_color(ui->speaker_badge, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(ui->speaker_badge, &lv_font_montserrat_10, 0);
    lv_obj_add_flag(ui->speaker_badge, LV_OBJ_FLAG_HIDDEN);

    /* Focus Anchor */
    lv_obj_t *anchor = lv_obj_create(viewport);
    lv_obj_set_size(anchor, 1, 1);
    lv_obj_set_style_bg_opa(anchor, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(anchor, 0, 0);
    lv_obj_add_flag(anchor, LV_OBJ_FLAG_CLICKABLE);
    ui->first_item = anchor;

    /* 4. Zone C: Bottom 20px Softkey Bar */
    ui->softkey_bar = softkey_bar_create(scr, "Mute", "End");
    softkey_set_actions("Mute", app_incall_toggle_mute, "End", app_incall_end);

    update_incall_ui();
    return scr;
}

void app_incall_init(void)
{
    s_call_state = INCALL_STATE_IDLE;
    s_call_seconds = 0;
    s_is_muted = false;
    s_is_speaker = false;
    s_dtmf_buf[0] = '\0';
    snprintf(s_duration_str, sizeof(s_duration_str), "Calling...");
    OS_LOGI(TAG, "In-Call Telephony engine initialized");
}

void app_incall_start(const char *name, const char *number, call_type_t type)
{
    if (!number || number[0] == '\0') return;

    /* Clean previous session */
    if (s_call_timer) {
        lv_timer_delete(s_call_timer);
        s_call_timer = NULL;
    }

    const contact_record_t *contact = name ? NULL : telephony_find_contact_by_number(number);
    const char *display_name = name ? name : (contact ? contact->name : "Unknown");

    strncpy(s_caller_name, display_name, sizeof(s_caller_name) - 1);
    s_caller_name[sizeof(s_caller_name) - 1] = '\0';
    strncpy(s_caller_number, number, sizeof(s_caller_number) - 1);
    s_caller_number[sizeof(s_caller_number) - 1] = '\0';

    s_call_type = type;
    s_call_state = INCALL_STATE_CALLING;
    s_call_seconds = 0;
    s_is_muted = false;
    s_is_speaker = false;
    s_dtmf_buf[0] = '\0';
    snprintf(s_duration_str, sizeof(s_duration_str), (type == CALL_TYPE_INCOMING) ? "Incoming..." : "Calling...");

    /* Start timer to tick duration */
    s_call_timer = lv_timer_create(on_call_timer_tick, 1000, NULL);

    lv_obj_t *scr = create_incall_screen();
    if (scr) {
        win_mgr_push(scr, "Mute", app_incall_toggle_mute, "End", app_incall_end);
    }

    char pill_buf[48];
    snprintf(pill_buf, sizeof(pill_buf), "In Call (%s)", s_duration_str);
    live_pill_publish(LIVE_PILL_PRIO_CALL, LV_SYMBOL_CALL, pill_buf, lv_color_hex(0x00E676), app_incall_show);

    app_idle_update();
    OS_LOGI(TAG, "Call started with '%s' (%s), Type: %d", s_caller_name, s_caller_number, (int)type);
}

void app_incall_end(void)
{
    if (s_call_state == INCALL_STATE_IDLE) return;

    OS_LOGI(TAG, "Call ended with '%s' (Duration: %u s)", s_caller_name, s_call_seconds);

    s_call_state = INCALL_STATE_ENDED;

    if (s_call_timer) {
        lv_timer_delete(s_call_timer);
        s_call_timer = NULL;
    }

    /* Record call log */
    telephony_add_call_log(s_caller_name, s_caller_number, s_call_type, s_call_seconds, "Just now");
    telephony_store_save();

    live_pill_clear(LIVE_PILL_PRIO_CALL);

    update_incall_ui();
    app_idle_update();

    s_call_state = INCALL_STATE_IDLE;
    s_call_seconds = 0;
    s_dtmf_buf[0] = '\0';

    /* Reset window manager to standby / idle screen */
    win_mgr_reset_to_home();
}

bool app_incall_is_active(void)
{
    return (s_call_state == INCALL_STATE_CALLING || s_call_state == INCALL_STATE_CONNECTED);
}

bool app_incall_is_foreground(void)
{
    if (!s_incall_screen || !lv_obj_is_valid(s_incall_screen)) return false;
    win_mgr_entry_t *top = win_mgr_get_top();
    return (top && top->screen == s_incall_screen);
}

uint32_t app_incall_get_duration_sec(void)
{
    return s_call_seconds;
}

const char * app_incall_get_duration_str(void)
{
    return s_duration_str;
}

const char * app_incall_get_name(void)
{
    return s_caller_name;
}

const char * app_incall_get_number(void)
{
    return s_caller_number;
}

void app_incall_show(void)
{
    if (!app_incall_is_active()) return;

    if (app_incall_is_foreground()) return;

    if (s_incall_screen && lv_obj_is_valid(s_incall_screen)) {
        win_mgr_push(s_incall_screen, s_is_muted ? "Unmute" : "Mute", app_incall_toggle_mute, "End", app_incall_end);
        return;
    }

    lv_obj_t *scr = create_incall_screen();
    if (scr) {
        win_mgr_push(scr, s_is_muted ? "Unmute" : "Mute", app_incall_toggle_mute, "End", app_incall_end);
    }
}

void app_incall_handle_key(veebha_key_t key)
{
    if (!app_incall_is_active()) return;

    char digit = '\0';
    if (key >= VEEBHA_KEY_NUM_0 && key <= VEEBHA_KEY_NUM_9) {
        digit = '0' + (key - VEEBHA_KEY_NUM_0);
    } else if (key == VEEBHA_KEY_STAR) {
        digit = '*';
    } else if (key == VEEBHA_KEY_HASH) {
        digit = '#';
    }

    if (digit != '\0') {
        size_t len = strlen(s_dtmf_buf);
        if (len < DTMF_MAX_LEN) {
            s_dtmf_buf[len] = digit;
            s_dtmf_buf[len + 1] = '\0';
            update_incall_ui();
            OS_LOGI(TAG, "DTMF digit sent: '%c' (Buffer: %s)", digit, s_dtmf_buf);
        }
    } else if (key == VEEBHA_KEY_OK) {
        app_incall_toggle_speaker();
    }
}

void app_incall_toggle_mute(void)
{
    s_is_muted = !s_is_muted;
    update_incall_ui();
    OS_LOGI(TAG, "Microphone %s", s_is_muted ? "MUTED" : "UNMUTED");
}

void app_incall_toggle_speaker(void)
{
    s_is_speaker = !s_is_speaker;
    update_incall_ui();
    OS_LOGI(TAG, "Speakerphone %s", s_is_speaker ? "ENABLED" : "DISABLED");
}
