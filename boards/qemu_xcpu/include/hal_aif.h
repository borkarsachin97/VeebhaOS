/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Audio Interface (AIF) Driver Header
 */

#ifndef _HAL_AIF_H_
#define _HAL_AIF_H_

#include "cs_types.h"
#include "global_macros.h"

// =============================================================================
//  Tone Type Enumeration
// =============================================================================
typedef enum
{
    HAL_AIF_DTMF_0 = 0,
    HAL_AIF_DTMF_1,
    HAL_AIF_DTMF_2,
    HAL_AIF_DTMF_3,
    HAL_AIF_DTMF_4,
    HAL_AIF_DTMF_5,
    HAL_AIF_DTMF_6,
    HAL_AIF_DTMF_7,
    HAL_AIF_DTMF_8,
    HAL_AIF_DTMF_9,
    HAL_AIF_DTMF_S,     // '*' (Star)
    HAL_AIF_DTMF_P,     // '#' (Pound)
    HAL_AIF_DTMF_A,
    HAL_AIF_DTMF_B,
    HAL_AIF_DTMF_C,
    HAL_AIF_DTMF_D,
    HAL_AIF_COMFORT_425,
    HAL_AIF_COMFORT_950,
    HAL_AIF_COMFORT_1400,
    HAL_AIF_COMFORT_1800,
    HAL_AIF_TONE_QTY
} HAL_AIF_TONE_TYPE_T;

// =============================================================================
//  Tone Attenuation Enumeration
// =============================================================================
typedef enum
{
    HAL_AIF_TONE_0DB = 0,
    HAL_AIF_TONE_M3DB,
    HAL_AIF_TONE_M9DB,
    HAL_AIF_TONE_M15DB
} HAL_AIF_TONE_ATTENUATION_T;

// =============================================================================
//  Driver APIs
// =============================================================================
void        hal_AifInit(void);
void        hal_AifOpen(void);
void        hal_AifClose(void);
void        hal_AifTone(HAL_AIF_TONE_TYPE_T tone, HAL_AIF_TONE_ATTENUATION_T attenuation, BOOL start);
void        hal_AifSetSideTone(UINT32 vol);
const char* hal_AifGetToneName(HAL_AIF_TONE_TYPE_T tone);

// =============================================================================
//  AIF DMA Audio Streaming APIs
// =============================================================================
typedef void (*HAL_AIF_DMA_CALLBACK_T)(void);

void        hal_AifConfigureAudio(UINT32 sample_rate, BOOL is_stereo, BOOL is_16bit);
BOOL        hal_AifPlayDma(void *buffer, UINT32 total_bytes, HAL_AIF_DMA_CALLBACK_T half_cb, HAL_AIF_DMA_CALLBACK_T end_cb);
void        hal_AifStopDma(void);
void        hal_AifPauseDma(BOOL pause);
void        hal_AifDmaIrqHandler(void);
BOOL        hal_AifIsDmaPlaying(void);
UINT32      hal_AifGetDmaSwapCount(void);
void        hal_AifUpdateTxMode(BOOL is_stereo);

#endif // _HAL_AIF_H_
