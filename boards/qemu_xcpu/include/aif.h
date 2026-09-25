/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Audio Interface (AIF) Hardware Registers for RDA8809 / RDA8955
 */

#ifndef _AIF_H_
#define _AIF_H_

#include "cs_types.h"
#include "global_macros.h"

// =============================================================================
//  AIF Hardware Register Base & Structure
// =============================================================================
#define REG_AIF_BASE                0x01927000

typedef volatile struct
{
    REG32                          data;                         //0x00000000
    REG32                          ctrl;                         //0x00000004
    REG32                          serial_ctrl;                  //0x00000008
    REG32                          tone;                         //0x0000000C
    REG32                          side_tone;                    //0x00000010
    REG32                          load_pos;                     //0x00000014
} HWP_AIF_T;

#define hwp_aif                     ((HWP_AIF_T*) KSEG1(REG_AIF_BASE))

#define AIF_RX_LOAD_POS(n)          (((n)&0xF)<<0)

// =============================================================================
//  AIF Control Register (ctrl) Bitfields
// =============================================================================
#define AIF_ENABLE                  (1<<0)
#define AIF_ENABLE_ENABLE           (1<<0)
#define AIF_ENABLE_DISABLE          (0<<0)

#define AIF_TX_OFF                  (1<<4)
#define AIF_TX_OFF_TX_ON            (0<<4)
#define AIF_TX_OFF_TX_OFF           (1<<4)

#define AIF_PARALLEL_OUT_SET        (1<<8)
#define AIF_PARALLEL_OUT_SET_SERL   (0<<8)
#define AIF_PARALLEL_OUT_SET_PARA   (1<<8)

#define AIF_PARALLEL_OUT_CLR        (1<<9)
#define AIF_PARALLEL_OUT_CLR_SERL   (0<<9)
#define AIF_PARALLEL_OUT_CLR_PARA   (1<<9)

#define AIF_PARALLEL_IN_SET         (1<<10)
#define AIF_PARALLEL_IN_SET_SERL    (0<<10)
#define AIF_PARALLEL_IN_SET_PARA    (1<<10)

#define AIF_PARALLEL_IN_CLR         (1<<11)
#define AIF_PARALLEL_IN_CLR_SERL    (0<<11)
#define AIF_PARALLEL_IN_CLR_PARA    (1<<11)

#define AIF_TX_STB_MODE             (1<<12)
#define AIF_OUT_UNDERFLOW           (1<<16)
#define AIF_IN_OVERFLOW             (1<<17)

#define AIF_LOOP_BACK               (1<<31)
#define AIF_LOOP_BACK_NORMAL        (0<<31)
#define AIF_LOOP_BACK_LOOPBACK      (1<<31)

// =============================================================================
//  AIF Serial Control Register (serial_ctrl) Bitfields
// =============================================================================
#define AIF_SERIAL_MODE(n)          (((n)&3)<<0)
#define AIF_SERIAL_MODE_I2S_PCM     (0<<0)
#define AIF_SERIAL_MODE_VOICE       (1<<0)
#define AIF_SERIAL_MODE_DAI         (2<<0)

#define AIF_MASTER_MODE             (1<<4)
#define AIF_MASTER_MODE_SLAVE       (0<<4)
#define AIF_MASTER_MODE_MASTER      (1<<4)

#define AIF_TX_MODE(n)              (((n)&3)<<12)
#define AIF_TX_MODE_STEREO_STEREO   (0<<12)
#define AIF_TX_MODE_MONO_STEREO_CHAN_L (1<<12)
#define AIF_TX_MODE_MONO_STEREO_DUPLI (2<<12)
#define AIF_TX_MODE_STEREO_TO_MONO  (3<<12)

// =============================================================================
//  AIF Tone Generator Register (tone) Bitfields
// =============================================================================
#define AIF_ENABLE_H                (1<<0)
#define AIF_ENABLE_H_DISABLE        (0<<0)
#define AIF_ENABLE_H_ENABLE         (1<<0)

#define AIF_TONE_SELECT             (1<<1)
#define AIF_TONE_SELECT_DTMF        (0<<1)
#define AIF_TONE_SELECT_COMFORT_TONE (1<<1)

#define AIF_DTMF_FREQ_COL(n)        (((n)&3)<<4)
#define AIF_DTMF_FREQ_COL_1209_HZ   (0<<4)
#define AIF_DTMF_FREQ_COL_1336_HZ   (1<<4)
#define AIF_DTMF_FREQ_COL_1477_HZ   (2<<4)
#define AIF_DTMF_FREQ_COL_1633_HZ   (3<<4)

#define AIF_DTMF_FREQ_ROW(n)        (((n)&3)<<6)
#define AIF_DTMF_FREQ_ROW_697_HZ    (0<<6)
#define AIF_DTMF_FREQ_ROW_770_HZ    (1<<6)
#define AIF_DTMF_FREQ_ROW_852_HZ    (2<<6)
#define AIF_DTMF_FREQ_ROW_941_HZ    (3<<6)

#define AIF_COMFORT_FREQ(n)         (((n)&3)<<8)
#define AIF_COMFORT_FREQ_425_HZ     (0<<8)
#define AIF_COMFORT_FREQ_950_HZ     (1<<8)
#define AIF_COMFORT_FREQ_1400_HZ    (2<<8)
#define AIF_COMFORT_FREQ_1800_HZ    (3<<8)

#define AIF_TONE_GAIN(n)            (((n)&3)<<12)
#define AIF_TONE_GAIN_0_DB          (0<<12)
#define AIF_TONE_GAIN_M3_DB         (1<<12)
#define AIF_TONE_GAIN_M9_DB         (2<<12)
#define AIF_TONE_GAIN_M15_DB        (3<<12)

// DTMF Combinations
#define AIF_FREQ_1  (AIF_DTMF_FREQ_ROW_697_HZ | AIF_DTMF_FREQ_COL_1209_HZ)
#define AIF_FREQ_2  (AIF_DTMF_FREQ_ROW_697_HZ | AIF_DTMF_FREQ_COL_1336_HZ)
#define AIF_FREQ_3  (AIF_DTMF_FREQ_ROW_697_HZ | AIF_DTMF_FREQ_COL_1477_HZ)
#define AIF_FREQ_A  (AIF_DTMF_FREQ_ROW_697_HZ | AIF_DTMF_FREQ_COL_1633_HZ)
#define AIF_FREQ_4  (AIF_DTMF_FREQ_ROW_770_HZ | AIF_DTMF_FREQ_COL_1209_HZ)
#define AIF_FREQ_5  (AIF_DTMF_FREQ_ROW_770_HZ | AIF_DTMF_FREQ_COL_1336_HZ)
#define AIF_FREQ_6  (AIF_DTMF_FREQ_ROW_770_HZ | AIF_DTMF_FREQ_COL_1477_HZ)
#define AIF_FREQ_B  (AIF_DTMF_FREQ_ROW_770_HZ | AIF_DTMF_FREQ_COL_1633_HZ)
#define AIF_FREQ_7  (AIF_DTMF_FREQ_ROW_852_HZ | AIF_DTMF_FREQ_COL_1209_HZ)
#define AIF_FREQ_8  (AIF_DTMF_FREQ_ROW_852_HZ | AIF_DTMF_FREQ_COL_1336_HZ)
#define AIF_FREQ_9  (AIF_DTMF_FREQ_ROW_852_HZ | AIF_DTMF_FREQ_COL_1477_HZ)
#define AIF_FREQ_C  (AIF_DTMF_FREQ_ROW_852_HZ | AIF_DTMF_FREQ_COL_1633_HZ)
#define AIF_FREQ_S  (AIF_DTMF_FREQ_ROW_941_HZ | AIF_DTMF_FREQ_COL_1209_HZ)
#define AIF_FREQ_0  (AIF_DTMF_FREQ_ROW_941_HZ | AIF_DTMF_FREQ_COL_1336_HZ)
#define AIF_FREQ_P  (AIF_DTMF_FREQ_ROW_941_HZ | AIF_DTMF_FREQ_COL_1477_HZ)
#define AIF_FREQ_D  (AIF_DTMF_FREQ_ROW_941_HZ | AIF_DTMF_FREQ_COL_1633_HZ)

// Side tone
#define AIF_SIDE_TONE_GAIN(n)       (((n)&15)<<0)

#endif // _AIF_H_
