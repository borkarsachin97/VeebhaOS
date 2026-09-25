/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * HAL PWM, PWL, PWT, and LPG Driver Header for RDA8809
 */

#ifndef _HAL_PWM_H_
#define _HAL_PWM_H_

#include "cs_types.h"
#include "pwm.h"

// =============================================================================
// TYPES & ENUMS
// =============================================================================

typedef enum
{
    HAL_PWL_NONE = 0,
    HAL_PWL_0,
    HAL_PWL_1,
    HAL_PWL_QTY
} HAL_PWL_ID_T;

typedef enum
{
    LPG_PER_125 = 0,    // 125 ms
    LPG_PER_250,        // 250 ms
    LPG_PER_500,        // 500 ms
    LPG_PER_750,        // 750 ms
    LPG_PER_1000,       // 1.0 s
    LPG_PER_1250,       // 1.25 s
    LPG_PER_1500,       // 1.5 s
    LPG_PER_1750,       // 1.75 s
    LPG_PER_QTY
} HAL_LPG_PERIOD_T;

typedef enum
{
    LPG_ON_01 = 1,      // 15.6 ms
    LPG_ON_02,          // 31.2 ms
    LPG_ON_03,          // 46.9 ms
    LPG_ON_04,          // 62.5 ms
    LPG_ON_05,          // 78.1 ms
    LPG_ON_06,          // 93.7 ms
    LPG_ON_07,          // 110 ms
    LPG_ON_08,          // 125 ms
    LPG_ON_09,          // 141 ms
    LPG_ON_10,          // 156 ms
    LPG_ON_11,          // 172 ms
    LPG_ON_12,          // 187 ms
    LPG_ON_13,          // 203 ms
    LPG_ON_14,          // 219 ms
    LPG_ON_15,          // 234 ms
    LPG_ON_QTY
} HAL_LPG_ON_T;

// =============================================================================
// FUNCTIONS
// =============================================================================

/**
 * @brief Initialize PWM hardware controller, clear resets, and enable clocks.
 */
void hal_PwmInit(void);

/**
 * @brief Set the duty cycle for the specified PWL channel.
 * @param id HAL_PWL_0 or HAL_PWL_1
 * @param level 0 = off, 255 = fully on, intermediate values = duty cycle (level / 255)
 */
void hal_PwlSelLevel(HAL_PWL_ID_T id, UINT8 level);

/**
 * @brief Configure glowing / breathing pulse mode on PWL0.
 * @param levelMin Minimum luminosity
 * @param levelMax Maximum luminosity
 * @param pulsePeriod Pulse period counter
 * @param pulse TRUE to enable pulsing, FALSE for static min level
 */
void hal_PwlGlow(UINT8 levelMin, UINT8 levelMax, UINT8 pulsePeriod, BOOL pulse);

/**
 * @brief Output a square wave tone on the buzzer / PWT pin.
 * @param noteFreq Desired tone frequency in Hz (min 152 Hz)
 * @param level Tone level / volume (0..100)
 */
void hal_BuzzStart(UINT16 noteFreq, UINT16 level);

/**
 * @brief Stop tone output on the buzzer / PWT pin.
 */
void hal_BuzzStop(void);

/**
 * @brief Start Light Pulse Generator (LPG) for status LED blinking.
 * @param period LPG repetition period
 * @param onTime LPG active on duration
 */
void hal_LpgStart(HAL_LPG_PERIOD_T period, HAL_LPG_ON_T onTime);

/**
 * @brief Stop Light Pulse Generator (LPG).
 */
void hal_LpgStop(void);

/**
 * @brief Set system backlight brightness level across both PWL and PMU LED drivers.
 * @param level Brightness level (0..255)
 */
void hal_PwmSetBacklight(UINT8 level);

/**
 * @brief Get current backlight brightness level (0..255).
 */
UINT8 hal_PwmGetBacklight(void);

#endif // _HAL_PWM_H_
