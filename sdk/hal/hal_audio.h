/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Hardware Abstraction Layer: Audio DAC / Codec Contract
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef HAL_AUDIO_H
#define HAL_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Initialize audio codec / DMA streaming interface.
 */
void hal_audio_init(void);

/**
 * Push PCM 16-bit audio buffer to hardware DAC.
 *
 * @param samples Pointer to PCM sample array.
 * @param count Number of samples.
 */
void hal_audio_play_pcm(const int16_t *samples, size_t count);

/**
 * Set master audio playback volume.
 *
 * @param volume_pct Volume percentage (0-100).
 */
void hal_audio_set_volume(uint8_t volume_pct);

/**
 * Mute / unmute audio output.
 */
void hal_audio_set_mute(bool mute);

#ifdef __cplusplus
}
#endif

#endif /* HAL_AUDIO_H */
