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
#include "boards/board_config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    lv_obj_t          *softkey_bar;
    lv_obj_t          *first_item;
    veebha_view_type_t view_type;
    char               title[WIN_MGR_LABEL_MAX];
    os_fullscreen_mode_t fullscreen_mode;
    bool               show_battery_hud;
    lv_obj_t          *art_box;
    lv_obj_t          *headline_lbl;
    lv_obj_t          *subline_lbl;
    lv_obj_t          *progress_bar;
    uint8_t            progress;
    bool               is_playing;
    void (*on_play_toggle)(void);
    void (*on_seek)(int8_t step);
    void (*on_back)(void);
    char               lsk_label[WIN_MGR_LABEL_MAX];
    char               rsk_label[WIN_MGR_LABEL_MAX];
} tpl_media_screen_data_t;

typedef struct {
    char    headline[WIN_MGR_LABEL_MAX];
    char    subline[WIN_MGR_LABEL_MAX];
    uint8_t progress;
    bool    is_playing;
    bool    has_session;
    void (*on_play_toggle)(void);
    void (*on_seek)(int8_t step);
} tpl_media_global_session_t;

static tpl_media_global_session_t s_media_session = {
    .has_session = false,
    .is_playing = false,
    .progress = 0
};

static tpl_media_screen_data_t *s_active_media = NULL;

void tpl_media_register_callbacks(void (*on_play_toggle)(void), void (*on_seek)(int8_t step))
{
    s_media_session.on_play_toggle = on_play_toggle;
    s_media_session.on_seek = on_seek;
}

void tpl_media_clear_session(void)
{
    s_media_session.has_session = false;
    s_media_session.is_playing = false;
    s_media_session.progress = 0;
    s_media_session.headline[0] = '\0';
    s_media_session.subline[0] = '\0';
}

static void update_media_softkeys(tpl_media_screen_data_t *data)
{
    if (!data) return;

    const char *lsk = data->is_playing ? "Pause" : "Play";
    if (data->lsk_label[0] != '\0') {
        lsk = data->lsk_label;
    }

    const char *rsk = data->rsk_label[0] ? data->rsk_label : "Back";

    softkey_set_actions(lsk, tpl_media_default_lsk, rsk, tpl_media_default_rsk);
}

static void on_media_screen_delete_cb(lv_event_t *e)
{
    lv_obj_t *scr = lv_event_get_target(e);
    tpl_media_screen_data_t *data = (tpl_media_screen_data_t *)lv_obj_get_user_data(scr);

    if (data) {
        if (s_active_media == data) {
            s_active_media = NULL;
        }
        free(data);
        lv_obj_set_user_data(scr, NULL);
    }
}

void tpl_media_default_lsk(void)
{
    tpl_media_handle_toggle();
}

void tpl_media_default_rsk(void)
{
    if (s_active_media && s_active_media->on_back) {
        s_active_media->on_back();
    } else {
        win_mgr_pop();
    }
}

void tpl_media_set_progress(uint8_t percent)
{
    if (percent > 100) percent = 100;
    s_media_session.progress = percent;

    if (s_active_media) {
        s_active_media->progress = percent;
        if (s_active_media->progress_bar && lv_obj_is_valid(s_active_media->progress_bar)) {
            lv_bar_set_value(s_active_media->progress_bar, percent, LV_ANIM_OFF);
        }
    }
}

uint8_t tpl_media_get_progress(void)
{
    return s_media_session.progress;
}

void tpl_media_set_playing(bool is_playing)
{
    s_media_session.is_playing = is_playing;

    if (s_active_media) {
        s_active_media->is_playing = is_playing;
        update_media_softkeys(s_active_media);
    }
}

bool tpl_media_is_playing(void)
{
    return s_media_session.is_playing;
}

bool tpl_media_has_active_session(void)
{
    return s_media_session.has_session || (s_active_media != NULL);
}

const char * tpl_media_get_headline(void)
{
    if (s_media_session.headline[0] != '\0') {
        return s_media_session.headline;
    }
    if (s_active_media && s_active_media->headline_lbl && lv_obj_is_valid(s_active_media->headline_lbl)) {
        return lv_label_get_text(s_active_media->headline_lbl);
    }
    return "";
}

const char * tpl_media_get_subline(void)
{
    if (s_media_session.subline[0] != '\0') {
        return s_media_session.subline;
    }
    if (s_active_media && s_active_media->subline_lbl && lv_obj_is_valid(s_active_media->subline_lbl)) {
        return lv_label_get_text(s_active_media->subline_lbl);
    }
    return "";
}

void tpl_media_set_metadata(const char *headline, const char *subline)
{
    s_media_session.has_session = true;
    if (headline) {
        strncpy(s_media_session.headline, headline, sizeof(s_media_session.headline) - 1);
        s_media_session.headline[sizeof(s_media_session.headline) - 1] = '\0';
    }
    if (subline) {
        strncpy(s_media_session.subline, subline, sizeof(s_media_session.subline) - 1);
        s_media_session.subline[sizeof(s_media_session.subline) - 1] = '\0';
    }

    if (s_active_media) {
        if (headline && s_active_media->headline_lbl && lv_obj_is_valid(s_active_media->headline_lbl)) {
            lv_label_set_text(s_active_media->headline_lbl, headline);
        }
        if (subline && s_active_media->subline_lbl && lv_obj_is_valid(s_active_media->subline_lbl)) {
            lv_label_set_text(s_active_media->subline_lbl, subline);
        }
    }
}

void tpl_media_handle_seek(int8_t step)
{
    int new_prog = (int)s_media_session.progress + step;
    if (new_prog < 0) new_prog = 0;
    if (new_prog > 100) new_prog = 100;

    tpl_media_set_progress((uint8_t)new_prog);

    if (s_media_session.on_seek) {
        s_media_session.on_seek(step);
    } else if (s_active_media && s_active_media->on_seek) {
        s_active_media->on_seek(step);
    }
}

void tpl_media_handle_toggle(void)
{
    s_media_session.is_playing = !s_media_session.is_playing;

    if (s_active_media) {
        s_active_media->is_playing = s_media_session.is_playing;
        update_media_softkeys(s_active_media);
    }

    if (s_media_session.on_play_toggle) {
        s_media_session.on_play_toggle();
    } else if (s_active_media && s_active_media->on_play_toggle) {
        s_active_media->on_play_toggle();
    }
}

lv_obj_t * tpl_media_create(const tpl_media_view_t *desc)
{
    if (!desc) return NULL;

    /* 1. Root Screen Container */
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, CONFIG_DISP_HOR_RES, CONFIG_DISP_VER_RES);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x121212), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Allocate screen context */
    tpl_media_screen_data_t *data = (tpl_media_screen_data_t *)calloc(1, sizeof(tpl_media_screen_data_t));
    if (!data) return NULL;

    data->view_type = VEEBHA_VIEW_TYPE_MEDIA;
    strncpy(data->title, desc->title ? desc->title : "Media", sizeof(data->title) - 1);
    data->title[sizeof(data->title) - 1] = '\0';
    data->progress = (desc->progress_percent <= 100) ? desc->progress_percent : 100;
    data->is_playing = desc->is_playing;
    data->on_play_toggle = desc->on_play_toggle;
    data->on_seek = desc->on_seek;
    data->on_back = desc->on_back;

    if (desc->lsk_label) {
        strncpy(data->lsk_label, desc->lsk_label, sizeof(data->lsk_label) - 1);
    }
    if (desc->rsk_label) {
        strncpy(data->rsk_label, desc->rsk_label, sizeof(data->rsk_label) - 1);
    }

    lv_obj_set_user_data(screen, data);
    lv_obj_add_event_cb(screen, on_media_screen_delete_cb, LV_EVENT_DELETE, NULL);

    /* 2. Zone A: Fixed 18px Top Status Bar */
    status_bar_create(screen, desc->title ? desc->title : "Media");

    /* 3. Zone B: Elastic 184px Viewport with 4 Centered Slots */
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_size(content, lv_pct(100), 0);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 4, 0);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    /* Center alignment for player elements */
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLL_ANIMATION);
    lv_obj_set_style_outline_width(content, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(content, 0, LV_STATE_FOCUS_KEY);

    /* Slot 1: Visual Container: 80x80 Artwork / Symbol Box */
    lv_obj_t *art_box = lv_obj_create(content);
    data->art_box = art_box;
    lv_obj_set_size(art_box, 80, 80);
    lv_obj_set_style_bg_color(art_box, lv_color_hex(0x1C2028), 0);
    lv_obj_set_style_bg_opa(art_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(art_box, lv_color_hex(0x282C35), 0);
    lv_obj_set_style_border_width(art_box, 1, 0);
    lv_obj_set_style_radius(art_box, 0, 0);
    lv_obj_set_style_outline_width(art_box, 0, 0);
    lv_obj_remove_flag(art_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(art_box, LV_OBJ_FLAG_SCROLL_ANIMATION);
    lv_obj_set_flex_flow(art_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(art_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *art_icon = lv_label_create(art_box);
    if (desc->art_src) {
        lv_label_set_text(art_icon, (const char *)desc->art_src);
    } else {
        lv_label_set_text(art_icon, LV_SYMBOL_AUDIO);
    }
    lv_obj_set_style_text_color(art_icon, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(art_icon, &lv_font_montserrat_16, 0);

    /* Slot 2: Headline Label (Track Title or Caller Name) */
    lv_obj_t *headline = lv_label_create(content);
    data->headline_lbl = headline;
    lv_label_set_text(headline, desc->headline ? desc->headline : "No Title");
    lv_obj_set_style_text_color(headline, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(headline, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(headline, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(headline, 6, 0);

    /* Slot 3: Subline Label (Artist Name or Duration) */
    lv_obj_t *subline = lv_label_create(content);
    data->subline_lbl = subline;
    lv_label_set_text(subline, desc->subline ? desc->subline : "");
    lv_obj_set_style_text_color(subline, lv_color_hex(0xA0A6B2), 0);
    lv_obj_set_style_text_font(subline, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_align(subline, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(subline, 2, 0);

    /* Slot 4: Styled Progress Bar: 154px width, rectangular */
    lv_obj_t *bar = lv_bar_create(content);
    data->progress_bar = bar;
    lv_obj_set_size(bar, 154, 8);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, data->progress, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x282C35), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x00E5FF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 0, LV_PART_INDICATOR);
    lv_obj_set_style_margin_top(bar, 10, 0);

    if (desc->hide_progress) {
        lv_obj_add_flag(bar, LV_OBJ_FLAG_HIDDEN);
    }

    /* Register content into global group so D-pad focus is preserved */
    data->first_item = content;
    lv_group_t *group = win_mgr_get_group();
    if (group) {
        lv_group_add_obj(group, content);
    }

    /* 4. Zone C: Fixed 20px Bottom Softkey Bar */
    const char *init_lsk = desc->is_playing ? "Pause" : "Play";
    if (desc->lsk_label) init_lsk = desc->lsk_label;
    const char *init_rsk = desc->rsk_label ? desc->rsk_label : "Back";

    lv_obj_t *softkey_bar = softkey_bar_create(screen, init_lsk, init_rsk);
    data->softkey_bar = softkey_bar;

    s_active_media = data;
    softkey_set_actions(init_lsk, tpl_media_default_lsk, init_rsk, tpl_media_default_rsk);

    printf("[MEDIA] Instantiated media view '%s' - '%s' (Progress: %u%%)\n",
           desc->headline ? desc->headline : "", desc->subline ? desc->subline : "", data->progress);
    return screen;
}
