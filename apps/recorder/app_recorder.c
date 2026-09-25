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

#include "app_recorder.h"
#include "sdk/vfs/os_vfs.h"
#include "sdk/include/veebha_templates.h"
#include "sdk/include/veebha_status_bar.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_overlays.h"
#include "sdk/include/veebha_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "APP_RECORDER"

typedef struct {
    lv_obj_t          *softkey_bar;
    lv_obj_t          *first_item;
    veebha_view_type_t view_type;
    char               title[WIN_MGR_LABEL_MAX];
    os_fullscreen_mode_t fullscreen_mode;
    bool               show_battery_hud;

    capture_mode_t     mode;
    bool               is_recording;
    uint32_t           record_duration_s;

    lv_obj_t          *mode_badges[CAPTURE_MODE_COUNT];
    lv_obj_t          *mode_labels[CAPTURE_MODE_COUNT];

    /* Dynamic viewport container */
    lv_obj_t          *content_cnt;

    /* Viewfinder / Graphic elements */
    lv_obj_t          *viewfinder_box;
    lv_obj_t          *rec_badge;
    lv_obj_t          *rec_dot;
    lv_obj_t          *timer_lbl;
    lv_obj_t          *status_lbl;
    lv_obj_t          *vu_bars[7];

    lv_timer_t        *sec_timer;
    lv_timer_t        *anim_timer;
} recorder_screen_data_t;

static lv_obj_t *s_active_recorder_scr = NULL;
static capture_mode_t s_current_mode = CAPTURE_MODE_PHOTO;
static uint16_t s_photo_seq = 1;
static uint16_t s_video_seq = 1;
static uint16_t s_voice_seq = 2;

bool app_recorder_is_active(void)
{
    return s_active_recorder_scr != NULL;
}

capture_mode_t app_recorder_get_mode(void)
{
    return s_current_mode;
}

static void update_recorder_ui(recorder_screen_data_t *data);

static void on_rec_sec_tick(lv_timer_t *tmr)
{
    recorder_screen_data_t *data = (recorder_screen_data_t *)lv_timer_get_user_data(tmr);
    if (!data || !data->is_recording) return;

    data->record_duration_s++;
    if (data->timer_lbl && lv_obj_is_valid(data->timer_lbl)) {
        char buf[32];
        uint32_t m = data->record_duration_s / 60;
        uint32_t s = data->record_duration_s % 60;
        snprintf(buf, sizeof(buf), "%02u:%02u", m, s);
        lv_label_set_text(data->timer_lbl, buf);
    }
}

static void on_rec_anim_tick(lv_timer_t *tmr)
{
    recorder_screen_data_t *data = (recorder_screen_data_t *)lv_timer_get_user_data(tmr);
    if (!data) return;

    if (data->mode == CAPTURE_MODE_VIDEO && data->is_recording) {
        /* Blink REC red dot */
        if (data->rec_dot && lv_obj_is_valid(data->rec_dot)) {
            static bool blink = false;
            blink = !blink;
            lv_obj_set_style_opa(data->rec_dot, blink ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        }
    } else if (data->mode == CAPTURE_MODE_VOICE && data->is_recording) {
        /* Animate VU bars */
        static const uint8_t vu_patterns[][7] = {
            {  6, 14, 20, 26, 18, 12,  8 },
            { 12, 22, 28, 16, 24, 18, 10 },
            {  8, 16, 24, 30, 22, 14,  6 },
            { 16, 28, 20, 24, 28, 16, 12 },
            { 10, 18, 26, 18, 14, 20,  8 },
        };
        static uint8_t p_step = 0;
        p_step = (p_step + 1) % (sizeof(vu_patterns) / sizeof(vu_patterns[0]));
        for (int i = 0; i < 7; i++) {
            if (data->vu_bars[i] && lv_obj_is_valid(data->vu_bars[i])) {
                lv_obj_set_height(data->vu_bars[i], vu_patterns[p_step][i]);
            }
        }
    }
}

static void on_action_confirm_dialog(void)
{
    tpl_dialog_close();
}

static void on_recorder_action(void)
{
    if (!s_active_recorder_scr) return;
    recorder_screen_data_t *data = (recorder_screen_data_t *)lv_obj_get_user_data(s_active_recorder_scr);
    if (!data) return;

    if (data->mode == CAPTURE_MODE_PHOTO) {
        /* Snap Photo */
        char fname[64];
        snprintf(fname, sizeof(fname), "IMG_%04u.raw", s_photo_seq++);
        OS_LOGI(TAG, "Photo captured and saved to /sdcard/Photos/%s", fname);

        char alert_msg[128];
        snprintf(alert_msg, sizeof(alert_msg), "Saved to /sdcard/Photos/\n%s (78 KB)", fname);
        notif_panel_post_alert("Photo Captured", alert_msg);

        static tpl_dialog_desc_t dlg = {
            .title = "Photo Captured",
            .icon = LV_SYMBOL_IMAGE,
            .message = "Saved to /sdcard/Photos/",
            .lsk_label = "OK",
            .rsk_label = "OK",
            .on_confirm = on_action_confirm_dialog,
            .on_cancel = on_action_confirm_dialog
        };
        tpl_dialog_show(&dlg);
    } else if (data->mode == CAPTURE_MODE_VIDEO) {
        if (!data->is_recording) {
            /* Start Video */
            data->is_recording = true;
            data->record_duration_s = 0;
            OS_LOGI(TAG, "Video recording started");
        } else {
            /* Stop Video */
            data->is_recording = false;
            char vname[64];
            snprintf(vname, sizeof(vname), "VID_%04u.mjpeg", s_video_seq++);
            OS_LOGI(TAG, "Video saved: /sdcard/Videos/%s (Duration: %u sec)", vname, data->record_duration_s);

            char alert_msg[96];
            snprintf(alert_msg, sizeof(alert_msg), "Saved %s (%u sec)", vname, data->record_duration_s);
            notif_panel_post_alert("Video Saved", alert_msg);
        }
    } else if (data->mode == CAPTURE_MODE_VOICE) {
        if (!data->is_recording) {
            /* Start Voice */
            data->is_recording = true;
            data->record_duration_s = 0;
            OS_LOGI(TAG, "Voice recording started (AMR-WB 16kHz)");
        } else {
            /* Stop Voice */
            data->is_recording = false;
            char wname[64];
            snprintf(wname, sizeof(wname), "Voice_%04u.wav", s_voice_seq++);
            OS_LOGI(TAG, "Voice memo saved: /sdcard/Recordings/%s (Duration: %u sec)", wname, data->record_duration_s);

            char alert_msg[96];
            snprintf(alert_msg, sizeof(alert_msg), "Saved %s (%u sec)", wname, data->record_duration_s);
            notif_panel_post_alert("Voice Memo Saved", alert_msg);
        }
    }
    update_recorder_ui(data);
}

static void on_recorder_back(void)
{
    win_mgr_pop();
}

static void update_recorder_ui(recorder_screen_data_t *data)
{
    if (!data) return;

    /* Update mode badges */
    for (int i = 0; i < CAPTURE_MODE_COUNT; i++) {
        if (!data->mode_badges[i] || !data->mode_labels[i]) continue;
        if (i == (int)data->mode) {
            lv_obj_set_style_bg_color(data->mode_badges[i], theme_get()->accent, 0);
            lv_obj_set_style_text_color(data->mode_labels[i], lv_color_hex(0x000000), 0);
        } else {
            lv_obj_set_style_bg_color(data->mode_badges[i], theme_get()->card_color, 0);
            lv_obj_set_style_text_color(data->mode_labels[i], theme_get()->text_primary, 0);
        }
    }

    /* Softkey actions */
    if (data->mode == CAPTURE_MODE_PHOTO) {
        softkey_set_actions("Snap", on_recorder_action, "Back", on_recorder_back);
    } else if (data->mode == CAPTURE_MODE_VIDEO) {
        softkey_set_actions(data->is_recording ? "Stop" : "Record", on_recorder_action, "Back", on_recorder_back);
    } else if (data->mode == CAPTURE_MODE_VOICE) {
        softkey_set_actions(data->is_recording ? "Stop" : "Record", on_recorder_action, "Back", on_recorder_back);
    }
}

void app_recorder_set_mode(capture_mode_t mode)
{
    if (mode >= CAPTURE_MODE_COUNT) return;
    s_current_mode = mode;
    if (s_active_recorder_scr) {
        recorder_screen_data_t *data = (recorder_screen_data_t *)lv_obj_get_user_data(s_active_recorder_scr);
        if (data) {
            data->mode = mode;
            data->is_recording = false;
            data->record_duration_s = 0;
            update_recorder_ui(data);
        }
    }
}

static void on_recorder_key_cb(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    recorder_screen_data_t *data = (recorder_screen_data_t *)lv_event_get_user_data(e);
    if (!data) return;

    if (key == LV_KEY_RIGHT || key == LV_KEY_NEXT || key == '#') {
        capture_mode_t next = (data->mode + 1) % CAPTURE_MODE_COUNT;
        app_recorder_set_mode(next);
    } else if (key == LV_KEY_LEFT || key == LV_KEY_PREV) {
        capture_mode_t prev = (data->mode == 0) ? (CAPTURE_MODE_COUNT - 1) : (data->mode - 1);
        app_recorder_set_mode(prev);
    } else if (key == LV_KEY_ENTER) {
        on_recorder_action();
    }
}

static void on_recorder_delete_cb(lv_event_t *e)
{
    lv_obj_t *scr = lv_event_get_target(e);
    if (scr == s_active_recorder_scr) {
        s_active_recorder_scr = NULL;
    }
    recorder_screen_data_t *data = (recorder_screen_data_t *)lv_obj_get_user_data(scr);
    if (data) {
        if (data->sec_timer) {
            lv_timer_delete(data->sec_timer);
            data->sec_timer = NULL;
        }
        if (data->anim_timer) {
            lv_timer_delete(data->anim_timer);
            data->anim_timer = NULL;
        }
        free(data);
        lv_obj_set_user_data(scr, NULL);
    }
}

static lv_obj_t * app_recorder_create(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, 176, 220);
    lv_obj_set_style_bg_color(screen, theme_get()->bg_color, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    recorder_screen_data_t *data = (recorder_screen_data_t *)calloc(1, sizeof(recorder_screen_data_t));
    if (!data) {
        lv_obj_delete(screen);
        return NULL;
    }

    data->view_type = VEEBHA_VIEW_TYPE_GENERIC;
    strncpy(data->title, "Recorder", sizeof(data->title) - 1);
    data->mode = s_current_mode;
    data->is_recording = false;
    data->record_duration_s = 0;

    lv_obj_set_user_data(screen, data);
    lv_obj_add_event_cb(screen, on_recorder_delete_cb, LV_EVENT_DELETE, NULL);
    s_active_recorder_scr = screen;

    /* 1. Status Bar (18px) */
    status_bar_create(screen, NULL);

    /* 2. Mode Selector Bar (22px) */
    lv_obj_t *mode_bar = lv_obj_create(screen);
    lv_obj_set_size(mode_bar, lv_pct(100), 22);
    lv_obj_set_style_bg_color(mode_bar, theme_get()->card_color, 0);
    lv_obj_set_style_bg_opa(mode_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(mode_bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(mode_bar, theme_is_light_mode() ? lv_color_hex(0xE2E8F0) : lv_color_hex(0x282C35), 0);
    lv_obj_set_style_border_width(mode_bar, 1, 0);
    lv_obj_set_style_pad_all(mode_bar, 0, 0);
    lv_obj_set_flex_flow(mode_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(mode_bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(mode_bar, LV_OBJ_FLAG_SCROLLABLE);

    const char *names[CAPTURE_MODE_COUNT] = { "Photo", "Video", "Voice" };
    for (int i = 0; i < CAPTURE_MODE_COUNT; i++) {
        lv_obj_t *badge = lv_obj_create(mode_bar);
        data->mode_badges[i] = badge;
        lv_obj_set_size(badge, 52, 18);
        lv_obj_set_style_radius(badge, 3, 0);
        lv_obj_set_style_pad_all(badge, 0, 0);
        lv_obj_remove_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *lbl = lv_label_create(badge);
        data->mode_labels[i] = lbl;
        lv_label_set_text(lbl, names[i]);
        lv_obj_center(lbl);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_10, 0);
    }

    /* 3. Elastic Viewport */
    lv_obj_t *content = lv_obj_create(screen);
    data->content_cnt = content;
    lv_obj_set_size(content, lv_pct(100), 0);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 4, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    /* Viewfinder Card (146x116) */
    lv_obj_t *vf = lv_button_create(content);
    data->viewfinder_box = vf;
    data->first_item = vf;
    lv_obj_set_size(vf, 148, 118);
    lv_obj_set_style_bg_color(vf, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_opa(vf, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(vf, theme_get()->accent, 0);
    lv_obj_set_style_border_width(vf, 1, 0);
    lv_obj_set_style_radius(vf, 4, 0);
    lv_obj_set_style_pad_all(vf, 2, 0);
    lv_obj_set_flex_flow(vf, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(vf, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(vf, LV_OBJ_FLAG_SCROLLABLE);

    /* Crosshair Icon / Viewfinder Art */
    lv_obj_t *art_lbl = lv_label_create(vf);
    lv_label_set_text(art_lbl, LV_SYMBOL_IMAGE "\n[ + ]");
    lv_obj_set_style_text_font(art_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(art_lbl, theme_get()->accent, 0);
    lv_obj_set_style_text_align(art_lbl, LV_TEXT_ALIGN_CENTER, 0);

    /* Red REC Dot */
    data->rec_dot = lv_obj_create(vf);
    lv_obj_set_size(data->rec_dot, 8, 8);
    lv_obj_set_style_bg_color(data->rec_dot, lv_color_hex(0xEF4444), 0);
    lv_obj_set_style_bg_opa(data->rec_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(data->rec_dot, 4, 0);
    lv_obj_set_style_border_width(data->rec_dot, 0, 0);
    lv_obj_set_align(data->rec_dot, LV_ALIGN_TOP_LEFT);
    lv_obj_remove_flag(data->rec_dot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(data->rec_dot, LV_OBJ_FLAG_HIDDEN);

    /* Duration Timer Label */
    data->timer_lbl = lv_label_create(content);
    lv_label_set_text(data->timer_lbl, "00:00");
    lv_obj_set_style_text_font(data->timer_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(data->timer_lbl, theme_get()->accent, 0);
    lv_obj_set_style_pad_top(data->timer_lbl, 2, 0);

    /* Audio VU Meter Container */
    lv_obj_t *vu_cnt = lv_obj_create(content);
    lv_obj_set_size(vu_cnt, 70, 20);
    lv_obj_set_style_bg_opa(vu_cnt, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(vu_cnt, 0, 0);
    lv_obj_set_style_pad_all(vu_cnt, 0, 0);
    lv_obj_set_style_pad_column(vu_cnt, 3, 0);
    lv_obj_set_flex_flow(vu_cnt, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(vu_cnt, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(vu_cnt, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < 7; i++) {
        data->vu_bars[i] = lv_obj_create(vu_cnt);
        lv_obj_set_size(data->vu_bars[i], 4, 4);
        lv_obj_set_style_bg_color(data->vu_bars[i], theme_get()->accent, 0);
        lv_obj_set_style_bg_opa(data->vu_bars[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(data->vu_bars[i], 1, 0);
        lv_obj_set_style_border_width(data->vu_bars[i], 0, 0);
        lv_obj_remove_flag(data->vu_bars[i], LV_OBJ_FLAG_SCROLLABLE);
    }

    lv_obj_add_event_cb(vf, on_recorder_key_cb, LV_EVENT_KEY, data);

    lv_group_t *grp = win_mgr_get_group();
    if (grp) {
        lv_group_add_obj(grp, vf);
        lv_group_focus_obj(vf);
    }

    /* 4. Bottom Softkey Bar */
    data->softkey_bar = softkey_bar_create(screen, "Snap", "Back");
    softkey_set_actions("Snap", on_recorder_action, "Back", on_recorder_back);

    /* Timers */
    data->sec_timer = lv_timer_create(on_rec_sec_tick, 1000, data);
    data->anim_timer = lv_timer_create(on_rec_anim_tick, 200, data);

    update_recorder_ui(data);
    return screen;
}

void app_recorder_init(void)
{
    OS_LOGI(TAG, "Media Capture & Recording Suite initialized");
}

void app_recorder_open(void)
{
    lv_obj_t *scr = app_recorder_create();
    if (scr) {
        win_mgr_push(scr, "Snap", on_recorder_action, "Back", on_recorder_back);
        OS_LOGI(TAG, "Media Capture & Recording Studio opened");
    }
}
