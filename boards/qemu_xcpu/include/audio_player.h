/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Audio Player Header for FreeRTOS RDA8809
 * High-performance double-buffered streaming of 16-bit PCM WAV and MP3 files
 */

#ifndef _AUDIO_PLAYER_H_
#define _AUDIO_PLAYER_H_

#include "cs_types.h"

typedef enum
{
    PLAYER_STATE_STOPPED = 0,
    PLAYER_STATE_PLAYING,
    PLAYER_STATE_PAUSED
} PLAYER_STATE_T;

typedef enum
{
    PLAYER_FORMAT_NONE = 0,
    PLAYER_FORMAT_WAV,
    PLAYER_FORMAT_MP3
} PLAYER_FORMAT_T;

typedef struct
{
    PLAYER_STATE_T   state;
    PLAYER_FORMAT_T  format;
    char             filename[64];
    UINT32           sample_rate;
    UINT16           channels;
    UINT16           bits_per_sample;
    UINT32           bitrate_kbps;
    UINT32           total_seconds;
    UINT32           elapsed_seconds;
    UINT32           bytes_read;
    UINT32           total_file_bytes;
    UINT32           underrun_count;
    UINT32           swap_count;
    UINT8            volume;
} audio_player_status_t;

// Public APIs
void                  audio_player_init(void);
BOOL                  audio_player_play(const char *filepath);
void                  audio_player_pause(void);
void                  audio_player_resume(void);
void                  audio_player_stop(void);
void                  audio_player_toggle_pause(void);
void                  audio_player_set_volume(UINT8 vol); // 0 (min) to 15 (max)
UINT8                 audio_player_get_volume(void);
void                  audio_player_volume_up(void);
void                  audio_player_volume_down(void);
audio_player_status_t audio_player_get_status(void);
BOOL                  audio_player_is_active(void);

#endif // _AUDIO_PLAYER_H_
