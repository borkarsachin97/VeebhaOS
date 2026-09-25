/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Integrated RDA8809 FM Tuner Driver Header
 */

#ifndef _HAL_FM_H_
#define _HAL_FM_H_

#include "cs_types.h"

#define FM_FREQ_MIN_KHZ         87000   // 87.0 MHz
#define FM_FREQ_MAX_KHZ         108000  // 108.0 MHz
#define FM_FREQ_STEP_KHZ        100     // 100 kHz (0.1 MHz)
#define FM_FREQ_DEFAULT_KHZ     98300   // 98.3 MHz default

typedef struct {
    UINT32 freqKHz;
    UINT8  rssi;
    BOOL   is_stereo;
    BOOL   is_valid_station;
    BOOL   is_opened;
    UINT8  volume;
} fm_status_t;

void   hal_FmInit(void);
BOOL   hal_FmOpen(UINT32 initialFreqKHz);
void   hal_FmClose(void);
BOOL   hal_FmIsOpen(void);

BOOL   hal_FmTune(UINT32 freqKHz);
BOOL   hal_FmStepFreq(INT32 stepKHz);
BOOL   hal_FmSeek(BOOL seekUp);

BOOL   hal_FmGetStatus(fm_status_t *status);
UINT32 hal_FmGetFreq(void);
UINT8  hal_FmGetRssi(void);
BOOL   hal_FmIsStereo(void);
BOOL   hal_FmIsValidStation(void);
void   hal_FmSetVolume(UINT8 vol);
UINT8  hal_FmGetVolume(void);

void   hal_FmTunePreset(UINT8 presetNum); // 1 to 9
UINT32 hal_FmGetPresetFreq(UINT8 presetNum);

void   hal_FmDumpRegisters(void);
BOOL   hal_FmScanBuses(void);
void   hal_FmPollStatus(void);

#endif // _HAL_FM_H_
