/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Simulator Hardware Abstraction Layer: Audio DAC Interface
 *
 * SPDX-License-Identifier: MIT
 */

#include "sdk/hal/hal_audio.h"
#include <stdio.h>

static uint8_t s_volume_pct = 80;
static bool s_is_muted = false;

void hal_audio_init(void)
{
    s_volume_pct = 80;
    s_is_muted = false;
}

void hal_audio_play_pcm(const int16_t *samples, size_t count)
{
    (void)samples;
    (void)count;
}

void hal_audio_set_volume(uint8_t volume_pct)
{
    if (volume_pct > 100) volume_pct = 100;
    s_volume_pct = volume_pct;
}

void hal_audio_set_mute(bool mute)
{
    s_is_muted = mute;
}
