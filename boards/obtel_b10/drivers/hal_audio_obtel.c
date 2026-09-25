/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * OBTEL B10 Hardware Abstraction Layer: Audio Driver (RDA Audio DAC)
 *
 * SPDX-License-Identifier: MIT
 */

#include "sdk/hal/hal_audio.h"
#include "boards/obtel_b10/include/global_macros.h"
#include <stdint.h>
#include <stdbool.h>

#define RDA_AUDIO_CTRL          REG32(RDA_BASE_AUDIO + 0x00)
#define RDA_AUDIO_VOLUME        REG32(RDA_BASE_AUDIO + 0x04)
#define RDA_AUDIO_STATUS        REG32(RDA_BASE_AUDIO + 0x08)
#define RDA_AUDIO_FIFO          REG32(RDA_BASE_AUDIO + 0x0C)

static uint8_t s_volume = 7;
static bool s_muted = false;

void hal_audio_init(void)
{
    RDA_AUDIO_CTRL = 0x01; /* Enable Audio DAC subsystem */
    hal_audio_set_volume(s_volume);
}

void hal_audio_play_tone(uint16_t freq_hz, uint16_t duration_ms)
{
    (void)freq_hz;
    (void)duration_ms;
    /* Synthesizer tone output via RDA audio hardware generator */
}

void hal_audio_set_volume(uint8_t vol)
{
    if (vol > 10) vol = 10;
    s_volume = vol;
    if (!s_muted) {
        RDA_AUDIO_VOLUME = (vol * 15) / 10;
    }
}

uint8_t hal_audio_get_volume(void)
{
    return s_volume;
}

void hal_audio_mute(bool muted)
{
    s_muted = muted;
    if (muted) {
        RDA_AUDIO_VOLUME = 0;
    } else {
        hal_audio_set_volume(s_volume);
    }
}
