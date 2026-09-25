/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Analog Baseband (ABB) Audio Codec & Speaker PA Driver Header
 */

#ifndef _HAL_ANA_ABB_H_
#define _HAL_ANA_ABB_H_

#include "cs_types.h"
#include "global_macros.h"

// =============================================================================
//  ABB Codec Registers (CS_ABB via SPI3)
// =============================================================================
#define ABB_REG_CODEC_SIM_INTERFACE         0x01
#define ABB_REG_CODEC_SETTING_1             0x02
#define ABB_REG_CODEC_SETTING_3             0x03
#define ABB_REG_USB_CONTROL                 0x04
#define ABB_REG_CODEC_MIC_SETTING           0x05
#define ABB_REG_CODEC_LINEIN_SETTING        0x08
#define ABB_REG_CODEC_LDO_SETTING1          0x09
#define ABB_REG_CODEC_LDO_SETTING2          0x0A
#define ABB_REG_CODEC_MISC_SETTING          0x0B
#define ABB_REG_CODEC_MODE_SEL              0x0C
#define ABB_REG_CODEC_CALIB_SETTING         0x0D
#define ABB_REG_CODEC_POWER_CTRL            0x0E
#define ABB_REG_CODEC_SOFT_RESET            0x0F
#define ABB_REG_CODEC_CLOCK_CLASS_K_PA      0x10
#define ABB_REG_CODEC_CLOCK_CODEC           0x11
#define ABB_REG_CODEC_HP_DETECT_12H         0x12
#define ABB_REG_CODEC_PLL_13H               0x13
#define ABB_REG_CODEC_PLL_14H               0x14
#define ABB_REG_CODEC_SDM2_FREQ_HIGH        0x15
#define ABB_REG_CODEC_SDM2_FREQ_LOW         0x16
#define ABB_REG_CODEC_SDM2_DITHER           0x17
#define ABB_REG_CODEC_RESET_CTRL            0x1F
#define ABB_REG_CODEC_GAINRAMP_CTRL         0x27
#define ABB_REG_CODEC_FM_MODE               0x28
#define ABB_REG_CODEC_DIG_EN                0x29
#define ABB_REG_CODEC_DIG_DAC_GAIN          0x2A
#define ABB_REG_CODEC_DIG_MIC_GAIN          0x2B
#define ABB_REG_CODEC_DIG_FREQ_SAMPLE_SEL   0x2C
#define ABB_REG_CODEC_DIG_FREQ_SAMPLE_DIV   0x2D
#define ABB_REG_CODEC_HP_DETECT_3DH         0x3D
#define ABB_REG_CODEC_HP_DETECT_44H         0x44
#define ABB_REG_CODEC_HP_DETECT_45H         0x45
#define ABB_REG_CODEC_HP_DETECT_46H         0x46
#define ABB_REG_CODEC_HP_DETECT_47H         0x47

#define ABB_NOTCH20_BYPASS                  (1 << 13)
#define ABB_EARPHONE_DET_BYPASS             (1 << 12)

// =============================================================================
//  ABB Register 0x09 (CODEC_LDO_SETTING1) Profiles
//  Bit 15 is ABB_PD_ALL_DR (1 = Power-down all analog stages).
//  0x0000: Power-up all DACs/opamps & enable audio output (Headphone & Speaker).
//  0x8000: Power-down all analog codec drivers (used for Mute/Close).
// =============================================================================
#define ABB_LDO_SETTING1_HEADPHONE          0x0000
#define ABB_LDO_SETTING1_SPEAKER            0x0000
#define ABB_LDO_SETTING1_POWER_DOWN         0x8000

// Clean Digital Codec Enable bitmask for RDA8809 (NO clock invert):
// ABB_DIG_S_DWA_EN (0x800) | ABB_DIG_MASH_EN_ADC (0x200) | ABB_DIG_PADET_CLK_INV (0x020) | ABB_DIG_S_CODEC_EN (0x004) = 0x0A24
#define ABB_DIG_EN_AUDIO_DEFAULT            0x0A24
#define ABB_DIG_EN_VOICE_DEFAULT            0x0A2C

// Digital registers for RDA8809E / RDA8955 (offset 0x100)
#define ABB_REG_CODEC_DIG_EN_8809E          0x129
#define ABB_REG_CODEC_DIG_DAC_GAIN_8809E    0x12A
#define ABB_REG_CODEC_DIG_FREQ_SAMPLE_SEL_8809E 0x12C
#define ABB_REG_CODEC_DIG_FREQ_SAMPLE_DIV_8809E 0x12D

// =============================================================================
//  PMU LDO & Speaker PA Registers (CS_PMU via SPI3)
// =============================================================================
#define PMU_REG_LDO_SETTINGS                0x02
#define PMU_REG_LDO_ACTIVE_SETTING1         0x03
#define PMU_REG_LDO_ACTIVE_SETTING5         0x07
#define PMU_REG_LDO_LP_SETTING1             0x08
#define PMU_REG_SPEAKER_PA_SETTING1         0x27
#define PMU_REG_THERMAL_CALIBRATION         0x36
#define PMU_REG_SPEAKER_PA_SETTING2         0x40
#define PMU_REG_SPEAKER_PA_SETTING3         0x41
#define PMU_REG_SPEAKER_PA_SETTING4         0x42

#define PMU_REG_HP_DETECT_SETTING           0x11
#define RDA_PMU_HP_OUT_MASK                 (1 << 0)
#define RDA_PMU_HP_IN_MASK                  (1 << 1)
#define RDA_PMU_HP_OUT_CLEAR                (1 << 2)
#define RDA_PMU_HP_IN_CLEAR                 (1 << 3)
#define RDA_PMU_HP_OUT                      (1 << 4)
#define RDA_PMU_HP_IN                       (1 << 5)
#define RDA_PMU_HP_DETECT                   (1 << 6)

#define RDA_PMU_DOUBLE_MODE_EN_CLG          (1 << 7)

// =============================================================================
//  Audio Routing Modes
// =============================================================================
typedef enum {
    HAL_AUD_ROUTE_AUTO = 0,     // Automatically switch to Headphone when jack inserted, else Speaker
    HAL_AUD_ROUTE_SPEAKER,      // Force Loudspeaker (Class-D Double BTL)
    HAL_AUD_ROUTE_HEADPHONE     // Force 3.5mm Headphone Jack (Stereo Headphone PA)
} HAL_AUD_ROUTE_T;

// =============================================================================
//  Driver APIs
// =============================================================================
void   hal_AbbOpen(void);
void   hal_AbbClose(void);
BOOL   hal_AbbIsOpen(void);
void   hal_AbbSetVolume(UINT8 vol); // 0 (min) to 15 (max)
UINT8  hal_AbbGetVolume(void);
void   hal_AbbSetMute(BOOL mute);
void   hal_AbbSpeakerPaEnable(BOOL enable);
void   hal_AbbSetSpeakerClassD(BOOL classD);
BOOL   hal_AbbGetSpeakerClassD(void);
void   hal_AbbEnableTestTone1k(BOOL en);
void   hal_AbbSetSampleRate(UINT32 sample_rate);

BOOL   hal_AudIsHeadphoneDetected(void);
UINT16 hal_AudGetHeadphoneStatus(UINT16 *pmu_hp_reg, UINT16 *gpadc_mv);
void   hal_AudSetRoute(HAL_AUD_ROUTE_T route);
HAL_AUD_ROUTE_T hal_AudGetRoute(void);
BOOL   hal_AudIsHeadphoneActive(void);
void   hal_AbbApplyAudioRoute(void);

UINT16 hal_AbbRead(UINT16 reg);
void   hal_AbbWrite(UINT16 reg, UINT16 val);
UINT16 hal_PmuRead(UINT16 reg);
void   hal_PmuWrite(UINT16 reg, UINT16 val);

void   hal_AbbAudioDumpDiagnostics(void);
void   hal_AbbSetAudioRouteFm(BOOL isFm);
BOOL   hal_AbbGetAudioRouteFm(void);

#endif // _HAL_ANA_ABB_H_
