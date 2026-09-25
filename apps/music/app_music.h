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

#ifndef APPS_MUSIC_APP_MUSIC_H
#define APPS_MUSIC_APP_MUSIC_H

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *title;
    const char *artist;
    uint16_t duration_sec; /* e.g. 200 for 03:20 */
    const char *duration_str; /* "03:20" */
} music_track_t;

/**
 * Initialize music subsystem, timer, and default tracks.
 */
void app_music_init(void);

/**
 * Open Music application (shows playlist or player).
 */
void app_music_open(void);

/**
 * Open Tier 1: Playlist view.
 */
void app_music_open_playlist(void);

/**
 * Open Tier 2: Walkman Now Playing player view.
 */
void app_music_open_player(void);

/**
 * Adjust music volume (+1 or -1). Range: 0 to 10.
 */
void app_music_adjust_volume(int8_t step);

/**
 * Get current music volume (0 to 10).
 */
uint8_t app_music_get_volume(void);

/**
 * Toggle Play/Pause state.
 */
void app_music_toggle_play(void);

/**
 * Skip to next track.
 */
void app_music_next(void);

/**
 * Skip to previous track.
 */
void app_music_prev(void);

/**
 * Relative seek in seconds (+/-).
 */
void app_music_seek(int16_t seconds);

/**
 * Stop music playback and clear live pill.
 */
void app_music_stop(void);

/**
 * Returns true if music is actively playing.
 */
bool app_music_is_playing(void);

/**
 * Get currently selected track.
 */
const music_track_t * app_music_get_current_track(void);

/**
 * Get current track elapsed seconds.
 */
uint16_t app_music_get_elapsed_sec(void);

/**
 * Select track by index and begin playback.
 */
void app_music_play_index(uint8_t index);

/**
 * Play a specific audio file from storage path and open Now Playing UI.
 *
 * @param filepath Full path or filename (e.g. "/sdcard/Music/01_Blinding_Lights.mp3").
 */
void app_music_play_file(const char *filepath);

typedef enum {
    MUSIC_MODE_PLAYER = 0,
    MUSIC_MODE_FM_RADIO
} music_mode_t;

/**
 * Get active music mode (Player vs FM Radio).
 */
music_mode_t app_music_get_mode(void);

/**
 * Set active music mode.
 */
void app_music_set_mode(music_mode_t mode);

/**
 * Toggle between Player and FM Radio mode.
 */
void app_music_toggle_mode(void);

/**
 * FM Radio fine tune: step in 0.1 MHz (+1 or -1).
 */
void app_music_fm_seek(int8_t step_tenth_mhz);

/**
 * FM Radio preset station selection.
 */
void app_music_fm_next_preset(void);
void app_music_fm_prev_preset(void);

/**
 * Get current FM frequency in tenths of MHz (e.g. 983 = 98.3 MHz).
 */
uint16_t app_music_fm_get_freq(void);

/**
 * Get current FM station name.
 */
const char * app_music_fm_get_station_name(void);

/**
 * Check if FM radio is currently muted.
 */
bool app_music_fm_is_muted(void);

/**
 * Toggle FM radio mute / unmute.
 */
void app_music_fm_toggle_mute(void);

#ifdef __cplusplus
}
#endif

#endif /* APPS_MUSIC_APP_MUSIC_H */
