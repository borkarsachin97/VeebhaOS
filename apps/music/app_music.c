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

#include "app_music.h"
#include "sdk/include/veebha_templates.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_status_bar.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_live_pill.h"
#include "sdk/include/veebha_log.h"
#include "sdk/include/veebha_i18n.h"
#include "boards/board_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "APP_MUSIC"

#define WALKMAN_ORANGE 0xFF6A00
#define WALKMAN_CYAN   0x00E5FF
#define WALKMAN_DARK   0x151210

static const music_track_t s_tracks[] = {
    { .title = "Blinding Lights", .artist = "The Weeknd",     .duration_sec = 200, .duration_str = "03:20" },
    { .title = "Midnight City",   .artist = "M83",            .duration_sec = 243, .duration_str = "04:03" },
    { .title = "Stay",            .artist = "The Kid LAROI",  .duration_sec = 141, .duration_str = "02:21" },
    { .title = "Resonance",       .artist = "HOME",           .duration_sec = 212, .duration_str = "03:32" },
    { .title = "Voice_001.wav",   .artist = "Voice Memo",     .duration_sec = 12,  .duration_str = "00:12" },
};
#define TRACK_COUNT ((uint8_t)(sizeof(s_tracks) / sizeof(s_tracks[0])))
#define PLAYLIST_TRACK_COUNT 4

typedef struct {
    lv_obj_t          *softkey_bar;
    lv_obj_t          *first_item;
    veebha_view_type_t view_type;
    char               title[WIN_MGR_LABEL_MAX];
    os_fullscreen_mode_t fullscreen_mode;
    bool               show_battery_hud;
    lv_obj_t          *screen;
    lv_obj_t          *art_box;
    lv_obj_t          *title_lbl;
    lv_obj_t          *artist_lbl;
    lv_obj_t          *elapsed_lbl;
    lv_obj_t          *duration_lbl;
    lv_obj_t          *scrubber_bar;
    lv_obj_t          *eq_bars[5];
    lv_obj_t          *vol_cnt;
    lv_obj_t          *vol_lbl;
    lv_timer_t        *vol_timer;
} walkman_ui_t;

typedef struct {
    uint16_t freq_tenth; /* e.g. 983 for 98.3 MHz */
    const char *name;
} fm_station_t;

static const fm_station_t s_fm_presets[] = {
    { 911, "Radio City" },
    { 935, "Red FM" },
    { 983, "Radio Mirchi" },
    { 1040, "Fever FM" },
    { 1072, "Classic Hits" }
};
#define FM_PRESET_COUNT 5

static music_mode_t  s_active_mode = MUSIC_MODE_PLAYER;
static uint16_t      s_fm_freq_tenth = 983; /* 98.3 MHz */
static int8_t        s_fm_preset_idx = 2;   /* Radio Mirchi */
static bool          s_fm_is_muted = false;

static uint8_t       s_current_track = 0;
static uint16_t      s_elapsed_sec = 0;
static bool          s_is_playing = false;
static uint8_t       s_volume = 7; /* 0 to 10 */
static walkman_ui_t *s_walkman_ui = NULL;
static lv_timer_t   *s_playback_timer = NULL;
static lv_timer_t   *s_eq_timer = NULL;

static void update_player_ui(void);
static void on_player_lsk(void);
static void on_player_rsk(void);

static void format_time(uint16_t total_sec, char *buf, size_t buf_len)
{
    uint16_t m = total_sec / 60;
    uint16_t s = total_sec % 60;
    snprintf(buf, buf_len, "%02u:%02u", m, s);
}

static void sync_tpl_media(void)
{
    if (s_active_mode == MUSIC_MODE_FM_RADIO) {
        char title_buf[32];
        snprintf(title_buf, sizeof(title_buf), "%u.%u MHz", s_fm_freq_tenth / 10, s_fm_freq_tenth % 10);
        tpl_media_set_metadata(title_buf, app_music_fm_get_station_name());
        uint8_t prog = (uint8_t)(((s_fm_freq_tenth - 875) * 100) / (1080 - 875));
        tpl_media_set_progress(prog);
        tpl_media_set_playing(!s_fm_is_muted);

        if (!s_fm_is_muted) {
            char pill_txt[64];
            snprintf(pill_txt, sizeof(pill_txt), "FM %s (%s)", title_buf, app_music_fm_get_station_name());
            live_pill_publish(LIVE_PILL_PRIO_RADIO, LV_SYMBOL_AUDIO, pill_txt, lv_color_hex(WALKMAN_ORANGE), app_music_open);
        } else {
            live_pill_clear(LIVE_PILL_PRIO_RADIO);
        }
        live_pill_clear(LIVE_PILL_PRIO_MUSIC);
    } else {
        const music_track_t *t = &s_tracks[s_current_track];
        uint8_t prog = 0;
        if (t->duration_sec > 0) {
            prog = (uint8_t)((s_elapsed_sec * 100) / t->duration_sec);
            if (prog > 100) prog = 100;
        }

        tpl_media_set_metadata(t->title, t->artist);
        tpl_media_set_progress(prog);
        tpl_media_set_playing(s_is_playing);

        if (s_is_playing || s_elapsed_sec > 0) {
            live_pill_publish(LIVE_PILL_PRIO_MUSIC, LV_SYMBOL_AUDIO, t->title, lv_color_hex(WALKMAN_CYAN), app_music_open);
        } else {
            live_pill_clear(LIVE_PILL_PRIO_MUSIC);
        }
        live_pill_clear(LIVE_PILL_PRIO_RADIO);
    }
}

static void on_playback_tick(lv_timer_t *timer)
{
    (void)timer;
    if (s_active_mode == MUSIC_MODE_FM_RADIO) return;
    if (!s_is_playing) return;

    const music_track_t *t = &s_tracks[s_current_track];
    s_elapsed_sec++;

    if (s_elapsed_sec >= t->duration_sec) {
        s_elapsed_sec = 0;
        s_current_track = (s_current_track + 1) % TRACK_COUNT;
        OS_LOGI(TAG, "Track completed, auto-advancing to: %s", s_tracks[s_current_track].title);
    }

    sync_tpl_media();

    if (s_walkman_ui && lv_obj_is_valid(s_walkman_ui->screen)) {
        update_player_ui();
    }
}

static void on_eq_tick(lv_timer_t *timer)
{
    (void)timer;
    if (!s_walkman_ui || !lv_obj_is_valid(s_walkman_ui->screen)) return;

    static const uint8_t eq_patterns[][5] = {
        { 4, 12, 16,  8,  5 },
        { 8, 15,  9, 14,  7 },
        { 12, 6, 14, 10, 15 },
        { 6, 14,  7, 16,  9 },
        { 10, 8, 15,  6, 12 },
        { 15, 11, 6, 13,  8 },
    };
    static uint8_t step = 0;

    bool active = (s_active_mode == MUSIC_MODE_FM_RADIO) ? !s_fm_is_muted : s_is_playing;
    if (active) {
        step = (step + 1) % (sizeof(eq_patterns) / sizeof(eq_patterns[0]));
        for (int i = 0; i < 5; i++) {
            if (s_walkman_ui->eq_bars[i] && lv_obj_is_valid(s_walkman_ui->eq_bars[i])) {
                lv_obj_set_height(s_walkman_ui->eq_bars[i], eq_patterns[step][i]);
            }
        }
    } else {
        for (int i = 0; i < 5; i++) {
            if (s_walkman_ui->eq_bars[i] && lv_obj_is_valid(s_walkman_ui->eq_bars[i])) {
                lv_obj_set_height(s_walkman_ui->eq_bars[i], 3);
            }
        }
    }
}

static void on_vol_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (s_walkman_ui && s_walkman_ui->vol_cnt && lv_obj_is_valid(s_walkman_ui->vol_cnt)) {
        lv_obj_add_flag(s_walkman_ui->vol_cnt, LV_OBJ_FLAG_HIDDEN);
    }
}

static void update_player_ui(void)
{
    if (!s_walkman_ui || !lv_obj_is_valid(s_walkman_ui->screen)) return;

    if (s_active_mode == MUSIC_MODE_FM_RADIO) {
        if (s_walkman_ui->title_lbl && lv_obj_is_valid(s_walkman_ui->title_lbl)) {
            char fbuf[32];
            snprintf(fbuf, sizeof(fbuf), "%u.%u MHz", s_fm_freq_tenth / 10, s_fm_freq_tenth % 10);
            lv_label_set_text(s_walkman_ui->title_lbl, fbuf);
        }

        if (s_walkman_ui->artist_lbl && lv_obj_is_valid(s_walkman_ui->artist_lbl)) {
            char abuf[48];
            snprintf(abuf, sizeof(abuf), "%s [Stereo RDS]", app_music_fm_get_station_name());
            lv_label_set_text(s_walkman_ui->artist_lbl, abuf);
        }

        if (s_walkman_ui->elapsed_lbl && lv_obj_is_valid(s_walkman_ui->elapsed_lbl)) {
            lv_label_set_text(s_walkman_ui->elapsed_lbl, "87.5");
        }

        if (s_walkman_ui->duration_lbl && lv_obj_is_valid(s_walkman_ui->duration_lbl)) {
            lv_label_set_text(s_walkman_ui->duration_lbl, "108.0");
        }

        if (s_walkman_ui->scrubber_bar && lv_obj_is_valid(s_walkman_ui->scrubber_bar)) {
            uint8_t prog = (uint8_t)(((s_fm_freq_tenth - 875) * 100) / (1080 - 875));
            lv_bar_set_value(s_walkman_ui->scrubber_bar, prog, LV_ANIM_OFF);
        }

        softkey_set_actions(s_fm_is_muted ? "Unmute" : "Mute", on_player_lsk, "Back", on_player_rsk);
    } else {
        const music_track_t *t = &s_tracks[s_current_track];

        if (s_walkman_ui->title_lbl && lv_obj_is_valid(s_walkman_ui->title_lbl)) {
            lv_label_set_text(s_walkman_ui->title_lbl, t->title);
        }

        if (s_walkman_ui->artist_lbl && lv_obj_is_valid(s_walkman_ui->artist_lbl)) {
            lv_label_set_text(s_walkman_ui->artist_lbl, t->artist);
        }

        if (s_walkman_ui->elapsed_lbl && lv_obj_is_valid(s_walkman_ui->elapsed_lbl)) {
            char time_buf[16];
            format_time(s_elapsed_sec, time_buf, sizeof(time_buf));
            lv_label_set_text(s_walkman_ui->elapsed_lbl, time_buf);
        }

        if (s_walkman_ui->duration_lbl && lv_obj_is_valid(s_walkman_ui->duration_lbl)) {
            lv_label_set_text(s_walkman_ui->duration_lbl, t->duration_str);
        }

        if (s_walkman_ui->scrubber_bar && lv_obj_is_valid(s_walkman_ui->scrubber_bar)) {
            uint8_t prog = 0;
            if (t->duration_sec > 0) {
                prog = (uint8_t)((s_elapsed_sec * 100) / t->duration_sec);
                if (prog > 100) prog = 100;
            }
            lv_bar_set_value(s_walkman_ui->scrubber_bar, prog, LV_ANIM_OFF);
        }

        softkey_set_actions(s_is_playing ? "Pause" : "Play", on_player_lsk, "Back", on_player_rsk);
    }
}

static void on_player_screen_delete_cb(lv_event_t *e)
{
    lv_obj_t *scr = lv_event_get_target(e);
    walkman_ui_t *ui = (walkman_ui_t *)lv_obj_get_user_data(scr);

    if (ui) {
        if (ui->vol_timer) {
            lv_timer_delete(ui->vol_timer);
            ui->vol_timer = NULL;
        }
        if (s_walkman_ui == ui) {
            s_walkman_ui = NULL;
        }
        free(ui);
        lv_obj_set_user_data(scr, NULL);
    }
}

static void on_player_lsk(void)
{
    if (s_active_mode == MUSIC_MODE_FM_RADIO) {
        app_music_fm_toggle_mute();
    } else {
        app_music_toggle_play();
    }
}

static void on_player_rsk(void)
{
    win_mgr_pop();
}

music_mode_t app_music_get_mode(void)
{
    return s_active_mode;
}

void app_music_set_mode(music_mode_t mode)
{
    s_active_mode = mode;
    if (s_active_mode == MUSIC_MODE_FM_RADIO) {
        if (s_is_playing) {
            s_is_playing = false;
        }
    }
    sync_tpl_media();
    if (s_walkman_ui && lv_obj_is_valid(s_walkman_ui->screen)) {
        update_player_ui();
    }
}

void app_music_toggle_mode(void)
{
    app_music_set_mode(s_active_mode == MUSIC_MODE_PLAYER ? MUSIC_MODE_FM_RADIO : MUSIC_MODE_PLAYER);
}

void app_music_fm_seek(int8_t step_tenth_mhz)
{
    int next = (int)s_fm_freq_tenth + step_tenth_mhz;
    if (next < 875) next = 1080;
    else if (next > 1080) next = 875;
    s_fm_freq_tenth = (uint16_t)next;

    s_fm_preset_idx = -1;
    for (int i = 0; i < FM_PRESET_COUNT; i++) {
        if (s_fm_presets[i].freq_tenth == s_fm_freq_tenth) {
            s_fm_preset_idx = i;
            break;
        }
    }
    sync_tpl_media();
    if (s_walkman_ui && lv_obj_is_valid(s_walkman_ui->screen)) {
        update_player_ui();
    }
}

void app_music_fm_next_preset(void)
{
    if (s_fm_preset_idx < 0) {
        s_fm_preset_idx = 0;
    } else {
        s_fm_preset_idx = (s_fm_preset_idx + 1) % FM_PRESET_COUNT;
    }
    s_fm_freq_tenth = s_fm_presets[s_fm_preset_idx].freq_tenth;
    sync_tpl_media();
    if (s_walkman_ui && lv_obj_is_valid(s_walkman_ui->screen)) {
        update_player_ui();
    }
}

void app_music_fm_prev_preset(void)
{
    if (s_fm_preset_idx <= 0) {
        s_fm_preset_idx = FM_PRESET_COUNT - 1;
    } else {
        s_fm_preset_idx--;
    }
    s_fm_freq_tenth = s_fm_presets[s_fm_preset_idx].freq_tenth;
    sync_tpl_media();
    if (s_walkman_ui && lv_obj_is_valid(s_walkman_ui->screen)) {
        update_player_ui();
    }
}

uint16_t app_music_fm_get_freq(void)
{
    return s_fm_freq_tenth;
}

const char * app_music_fm_get_station_name(void)
{
    if (s_fm_preset_idx >= 0 && s_fm_preset_idx < FM_PRESET_COUNT) {
        return s_fm_presets[s_fm_preset_idx].name;
    }
    return "Custom Station";
}

bool app_music_fm_is_muted(void)
{
    return s_fm_is_muted;
}

void app_music_fm_toggle_mute(void)
{
    s_fm_is_muted = !s_fm_is_muted;
    sync_tpl_media();
    if (s_walkman_ui && lv_obj_is_valid(s_walkman_ui->screen)) {
        update_player_ui();
    }
}

static void on_media_seek_cb(int8_t step)
{
    app_music_seek((int16_t)step);
}

void app_music_init(void)
{
    s_current_track = 0;
    s_elapsed_sec = 0;
    s_is_playing = false;
    s_volume = 7;

    tpl_media_register_callbacks(app_music_toggle_play, on_media_seek_cb);

    if (!s_playback_timer) {
        s_playback_timer = lv_timer_create(on_playback_tick, 1000, NULL);
    }
    if (!s_eq_timer) {
        s_eq_timer = lv_timer_create(on_eq_tick, 150, NULL);
    }

    sync_tpl_media();
    OS_LOGI(TAG, "Music player suite initialized");
}

void app_music_adjust_volume(int8_t step)
{
    int new_vol = (int)s_volume + step;
    if (new_vol < 0) new_vol = 0;
    if (new_vol > 10) new_vol = 10;
    s_volume = (uint8_t)new_vol;

    OS_LOGI(TAG, "Volume adjusted: %u/10", s_volume);

    if (s_walkman_ui && lv_obj_is_valid(s_walkman_ui->screen)) {
        if (s_walkman_ui->vol_lbl && lv_obj_is_valid(s_walkman_ui->vol_lbl)) {
            char buf[16];
            snprintf(buf, sizeof(buf), "Vol: %u/10", s_volume);
            lv_label_set_text(s_walkman_ui->vol_lbl, buf);
        }
        if (s_walkman_ui->vol_cnt && lv_obj_is_valid(s_walkman_ui->vol_cnt)) {
            lv_obj_clear_flag(s_walkman_ui->vol_cnt, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_walkman_ui->vol_timer) {
            lv_timer_reset(s_walkman_ui->vol_timer);
        }
    }
}

uint8_t app_music_get_volume(void)
{
    return s_volume;
}

void app_music_toggle_play(void)
{
    s_is_playing = !s_is_playing;
    OS_LOGI(TAG, "Playback toggle: %s", s_is_playing ? "PLAYING" : "PAUSED");

    sync_tpl_media();

    if (s_walkman_ui && lv_obj_is_valid(s_walkman_ui->screen)) {
        update_player_ui();
        on_eq_tick(NULL);
    }
}

void app_music_next(void)
{
    s_current_track = (s_current_track + 1) % TRACK_COUNT;
    s_elapsed_sec = 0;
    sync_tpl_media();
    if (s_walkman_ui && lv_obj_is_valid(s_walkman_ui->screen)) {
        update_player_ui();
    }
}

void app_music_prev(void)
{
    if (s_current_track == 0) {
        s_current_track = TRACK_COUNT - 1;
    } else {
        s_current_track--;
    }
    s_elapsed_sec = 0;
    sync_tpl_media();
    if (s_walkman_ui && lv_obj_is_valid(s_walkman_ui->screen)) {
        update_player_ui();
    }
}

void app_music_seek(int16_t seconds)
{
    const music_track_t *t = &s_tracks[s_current_track];
    int new_sec = (int)s_elapsed_sec + seconds;
    if (new_sec < 0) new_sec = 0;
    if (new_sec >= t->duration_sec) new_sec = t->duration_sec - 1;
    s_elapsed_sec = (uint16_t)new_sec;

    sync_tpl_media();
    if (s_walkman_ui && lv_obj_is_valid(s_walkman_ui->screen)) {
        update_player_ui();
    }
}

void app_music_stop(void)
{
    s_is_playing = false;
    s_elapsed_sec = 0;
    live_pill_clear(LIVE_PILL_PRIO_MUSIC);
    live_pill_clear(LIVE_PILL_PRIO_RADIO);
    sync_tpl_media();
    if (s_walkman_ui && lv_obj_is_valid(s_walkman_ui->screen)) {
        update_player_ui();
        on_eq_tick(NULL);
    }
    OS_LOGI(TAG, "Music playback stopped");
}

bool app_music_is_playing(void)
{
    return s_is_playing;
}

const music_track_t * app_music_get_current_track(void)
{
    return &s_tracks[s_current_track];
}

uint16_t app_music_get_elapsed_sec(void)
{
    return s_elapsed_sec;
}

void app_music_play_index(uint8_t index)
{
    if (index >= TRACK_COUNT) return;
    s_current_track = index;
    s_elapsed_sec = 0;
    s_is_playing = true;
    sync_tpl_media();
}

void app_music_play_file(const char *filepath)
{
    if (!filepath) return;

    uint8_t target_idx = 0;
    if (strstr(filepath, "Blinding_Lights") || strstr(filepath, "Blinding Lights")) {
        target_idx = 0;
    } else if (strstr(filepath, "Midnight_City") || strstr(filepath, "Midnight City")) {
        target_idx = 1;
    } else if (strstr(filepath, "Stay")) {
        target_idx = 2;
    } else if (strstr(filepath, "Resonance")) {
        target_idx = 3;
    } else if (strstr(filepath, "Voice_001") || strstr(filepath, ".wav")) {
        target_idx = 4;
    }

    OS_LOGI(TAG, "Playing audio file '%s' -> track index %u ('%s')", filepath, target_idx, s_tracks[target_idx].title);
    app_music_play_index(target_idx);
    app_music_open_player();
}

void app_music_open_player(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    if (!screen) return;

    walkman_ui_t *ui = (walkman_ui_t *)calloc(1, sizeof(walkman_ui_t));
    if (!ui) {
        lv_obj_delete(screen);
        return;
    }

    ui->screen = screen;
    ui->view_type = VEEBHA_VIEW_TYPE_MEDIA;
    strncpy(ui->title, "Walkman", sizeof(ui->title) - 1);

    lv_obj_set_size(screen, CONFIG_DISP_HOR_RES, CONFIG_DISP_VER_RES);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101216), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    /* 1. Status Bar (18px) */
    status_bar_create(screen, "Walkman");

    /* 2. Middle Viewport (Elastic ~182px) */
    lv_obj_t *viewport = lv_obj_create(screen);
    lv_obj_set_size(viewport, CONFIG_DISP_HOR_RES, 0);
    lv_obj_set_flex_grow(viewport, 1);
    lv_obj_set_style_bg_opa(viewport, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(viewport, 0, 0);
    lv_obj_set_style_pad_hor(viewport, 4, 0);
    lv_obj_set_style_pad_ver(viewport, 2, 0);
    lv_obj_set_flex_flow(viewport, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(viewport, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(viewport, LV_OBJ_FLAG_SCROLLABLE);

    /* 2a. Artwork Squircle Card (54x54) */
    lv_obj_t *art_box = lv_obj_create(viewport);
    ui->art_box = art_box;
    lv_obj_set_size(art_box, 54, 54);
    lv_obj_set_style_bg_color(art_box, lv_color_hex(WALKMAN_DARK), 0);
    lv_obj_set_style_bg_opa(art_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(art_box, lv_color_hex(WALKMAN_ORANGE), 0);
    lv_obj_set_style_border_width(art_box, 2, 0);
    lv_obj_set_style_radius(art_box, 10, 0);
    lv_obj_set_style_pad_all(art_box, 2, 0);
    lv_obj_remove_flag(art_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(art_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(art_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Walkman Logo / Icon inside artwork */
    lv_obj_t *art_icon = lv_label_create(art_box);
    lv_label_set_text(art_icon, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_color(art_icon, lv_color_hex(WALKMAN_ORANGE), 0);
    lv_obj_set_style_text_font(art_icon, &lv_font_montserrat_14, 0);

    lv_obj_t *w_lbl = lv_label_create(art_box);
    lv_label_set_text(w_lbl, "WALKMAN");
    lv_obj_set_style_text_color(w_lbl, lv_color_hex(0xFFA726), 0);
    lv_obj_set_style_text_font(w_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_pad_top(w_lbl, 1, 0);

    /* 2b. Track Title */
    ui->title_lbl = lv_label_create(viewport);
    lv_label_set_text(ui->title_lbl, s_tracks[s_current_track].title);
    lv_obj_set_style_text_color(ui->title_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(ui->title_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(ui->title_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(ui->title_lbl, 160);
    lv_label_set_long_mode(ui->title_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_pad_top(ui->title_lbl, 4, 0);

    /* 2c. Artist Subtitle */
    ui->artist_lbl = lv_label_create(viewport);
    lv_label_set_text(ui->artist_lbl, s_tracks[s_current_track].artist);
    lv_obj_set_style_text_color(ui->artist_lbl, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_style_text_font(ui->artist_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_align(ui->artist_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(ui->artist_lbl, 160);
    lv_label_set_long_mode(ui->artist_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_pad_top(ui->artist_lbl, 1, 0);

    /* 2d. Dual Timestamps Flanking 4px Scrubber */
    lv_obj_t *scrub_row = lv_obj_create(viewport);
    lv_obj_set_size(scrub_row, 160, 16);
    lv_obj_set_style_bg_opa(scrub_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(scrub_row, 0, 0);
    lv_obj_set_style_pad_all(scrub_row, 0, 0);
    lv_obj_set_flex_flow(scrub_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(scrub_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(scrub_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_top(scrub_row, 4, 0);

    ui->elapsed_lbl = lv_label_create(scrub_row);
    lv_label_set_text(ui->elapsed_lbl, "00:00");
    lv_obj_set_style_text_color(ui->elapsed_lbl, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_style_text_font(ui->elapsed_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_width(ui->elapsed_lbl, 32);
    lv_obj_set_style_text_align(ui->elapsed_lbl, LV_TEXT_ALIGN_LEFT, 0);

    ui->scrubber_bar = lv_bar_create(scrub_row);
    lv_obj_set_size(ui->scrubber_bar, 86, 4);
    lv_bar_set_range(ui->scrubber_bar, 0, 100);
    lv_bar_set_value(ui->scrubber_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ui->scrubber_bar, lv_color_hex(0x282C35), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui->scrubber_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(ui->scrubber_bar, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ui->scrubber_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui->scrubber_bar, lv_color_hex(WALKMAN_CYAN), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(ui->scrubber_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(ui->scrubber_bar, 2, LV_PART_INDICATOR);

    ui->duration_lbl = lv_label_create(scrub_row);
    lv_label_set_text(ui->duration_lbl, s_tracks[s_current_track].duration_str);
    lv_obj_set_style_text_color(ui->duration_lbl, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_style_text_font(ui->duration_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_width(ui->duration_lbl, 32);
    lv_obj_set_style_text_align(ui->duration_lbl, LV_TEXT_ALIGN_RIGHT, 0);

    /* 2e. 5-Bar Animated Equalizer */
    lv_obj_t *eq_cnt = lv_obj_create(viewport);
    lv_obj_set_size(eq_cnt, 50, 18);
    lv_obj_set_style_bg_opa(eq_cnt, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(eq_cnt, 0, 0);
    lv_obj_set_style_pad_all(eq_cnt, 0, 0);
    lv_obj_set_style_pad_column(eq_cnt, 3, 0);
    lv_obj_set_flex_flow(eq_cnt, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(eq_cnt, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(eq_cnt, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_top(eq_cnt, 4, 0);

    for (int i = 0; i < 5; i++) {
        ui->eq_bars[i] = lv_obj_create(eq_cnt);
        lv_obj_set_size(ui->eq_bars[i], 4, 2);
        lv_obj_set_style_bg_color(ui->eq_bars[i], lv_color_hex(WALKMAN_ORANGE), 0);
        lv_obj_set_style_bg_opa(ui->eq_bars[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(ui->eq_bars[i], 1, 0);
        lv_obj_set_style_border_width(ui->eq_bars[i], 0, 0);
        lv_obj_set_style_pad_all(ui->eq_bars[i], 0, 0);
        lv_obj_remove_flag(ui->eq_bars[i], LV_OBJ_FLAG_SCROLLABLE);
    }

    /* 2f. Floating Volume OSD Pill */
    ui->vol_cnt = lv_obj_create(viewport);
    lv_obj_set_size(ui->vol_cnt, LV_SIZE_CONTENT, 18);
    lv_obj_set_style_bg_color(ui->vol_cnt, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_opa(ui->vol_cnt, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ui->vol_cnt, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_border_width(ui->vol_cnt, 1, 0);
    lv_obj_set_style_radius(ui->vol_cnt, 9, 0);
    lv_obj_set_style_pad_hor(ui->vol_cnt, 8, 0);
    lv_obj_set_style_pad_ver(ui->vol_cnt, 1, 0);
    lv_obj_remove_flag(ui->vol_cnt, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui->vol_cnt, LV_OBJ_FLAG_HIDDEN);

    ui->vol_lbl = lv_label_create(ui->vol_cnt);
    char vbuf[16];
    snprintf(vbuf, sizeof(vbuf), "Vol: %u/10", s_volume);
    lv_label_set_text(ui->vol_lbl, vbuf);
    lv_obj_set_style_text_color(ui->vol_lbl, lv_color_hex(0x38BDF8), 0);
    lv_obj_set_style_text_font(ui->vol_lbl, &lv_font_montserrat_10, 0);

    ui->vol_timer = lv_timer_create(on_vol_timer_cb, 1500, NULL);

    /* Dummy focus anchor for keypad */
    lv_obj_t *focus_anchor = lv_obj_create(viewport);
    lv_obj_set_size(focus_anchor, 1, 1);
    lv_obj_set_style_bg_opa(focus_anchor, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_anchor, 0, 0);
    lv_obj_add_flag(focus_anchor, LV_OBJ_FLAG_CLICKABLE);
    ui->first_item = focus_anchor;

    lv_group_t *grp = win_mgr_get_group();
    if (grp) {
        lv_group_add_obj(grp, focus_anchor);
    }

    /* 3. Softkey Bar (20px) */
    ui->softkey_bar = softkey_bar_create(screen, s_is_playing ? "Pause" : "Play", "Back");
    softkey_bar_set_active_widget(ui->softkey_bar);
    softkey_set_actions(s_is_playing ? "Pause" : "Play", on_player_lsk, "Back", on_player_rsk);

    s_walkman_ui = ui;
    lv_obj_set_user_data(screen, ui);
    lv_obj_add_event_cb(screen, on_player_screen_delete_cb, LV_EVENT_DELETE, NULL);

    update_player_ui();

    win_mgr_push(screen, s_is_playing ? "Pause" : "Play", on_player_lsk, "Back", on_player_rsk);
    OS_LOGI(TAG, "Walkman Now Playing UI opened for track %u: '%s'", s_current_track, s_tracks[s_current_track].title);
}

static void on_playlist_select(uint16_t index)
{
    /* If index 0 is [ Now Playing ] */
    if (s_is_playing && index == 0) {
        app_music_set_mode(MUSIC_MODE_PLAYER);
        app_music_open_player();
        return;
    }

    uint8_t fm_idx = s_is_playing ? (PLAYLIST_TRACK_COUNT + 1) : PLAYLIST_TRACK_COUNT;
    if (index == fm_idx) {
        app_music_set_mode(MUSIC_MODE_FM_RADIO);
        app_music_open_player();
        return;
    }

    uint8_t track_idx = index;
    if (s_is_playing) {
        track_idx = (uint8_t)(index - 1);
    }

    if (track_idx < TRACK_COUNT) {
        app_music_set_mode(MUSIC_MODE_PLAYER);
        app_music_play_index(track_idx);
        app_music_open_player();
    }
}

static void on_playlist_back(void)
{
    win_mgr_pop();
}

void app_music_open_playlist(void)
{
    static tpl_list_item_t list_items[7];
    uint16_t item_count = 0;

    /* Build formatted subtexts */
    static char subtexts[PLAYLIST_TRACK_COUNT][48];
    for (uint8_t i = 0; i < PLAYLIST_TRACK_COUNT; i++) {
        snprintf(subtexts[i], sizeof(subtexts[i]), "%s [%s]", s_tracks[i].artist, s_tracks[i].duration_str);
    }

    if (s_is_playing) {
        list_items[0].icon = LV_SYMBOL_AUDIO;
        list_items[0].title = "[ Now Playing ]";
        list_items[0].subtext = s_tracks[s_current_track].title;
        item_count++;

        for (uint8_t i = 0; i < PLAYLIST_TRACK_COUNT; i++) {
            list_items[item_count].icon = LV_SYMBOL_AUDIO;
            list_items[item_count].title = s_tracks[i].title;
            list_items[item_count].subtext = subtexts[i];
            item_count++;
        }
    } else {
        for (uint8_t i = 0; i < PLAYLIST_TRACK_COUNT; i++) {
            list_items[i].icon = LV_SYMBOL_AUDIO;
            list_items[i].title = s_tracks[i].title;
            list_items[i].subtext = subtexts[i];
            item_count++;
        }
    }

    list_items[item_count].icon = LV_SYMBOL_REFRESH;
    list_items[item_count].title = "[ Switch to FM Radio ]";
    list_items[item_count].subtext = "87.5 - 108.0 MHz Tuner";
    item_count++;

    tpl_list_view_t view_desc = {
        .title = "Music Tracks",
        .items = list_items,
        .count = item_count,
        .on_select = on_playlist_select,
        .on_back = on_playlist_back,
        .lsk_label = "Select",
        .rsk_label = "Back"
    };

    lv_obj_t *scr = tpl_list_create(&view_desc);
    if (scr) {
        win_mgr_push(scr, "Select", tpl_list_default_lsk, "Back", on_playlist_back);
        OS_LOGI(TAG, "Music playlist view opened with %u items", item_count);
    }
}

void app_music_open(void)
{
    if (s_is_playing) {
        app_music_open_player();
    } else {
        app_music_open_playlist();
    }
}
