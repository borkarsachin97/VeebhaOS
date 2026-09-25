/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Low Power Scheme (LPS) & Deep Sleep Driver for RDA8809 / RDA8955 SoC
 */

#ifndef _HAL_LPS_H_
#define _HAL_LPS_H_

#include "cs_types.h"
#include "global_macros.h"

// =============================================================================
//  LPS Sleep Mode Types
// =============================================================================
typedef enum
{
    LPS_MODE_NONE = 0,
    LPS_MODE_LIGHT_SLEEP,       // Clock gating, CPU wait, screen remains ON
    LPS_MODE_DEEP_SLEEP,        // Clock gating, 32kHz slow clock, PMU LDO cut, display OFF, wake on ANY key / Power key
    LPS_MODE_TIMED_SLEEP        // Deep sleep with automatic timer wakeup after specified duration
} HAL_LPS_SLEEP_MODE_T;

// =============================================================================
//  LPS Wakeup Source Identifier
// =============================================================================
typedef enum
{
    LPS_WAKEUP_NONE = 0,
    LPS_WAKEUP_KEYPAD_MATRIX,   // Matrix keypad press
    LPS_WAKEUP_POWER_KEY,       // PMU Power / ON-OFF key
    LPS_WAKEUP_TIMER,           // OS Timer / Countdown alarm
    LPS_WAKEUP_GPADC,           // Battery / Charger voltage change
    LPS_WAKEUP_USB,             // USB cable insertion / activity
    LPS_WAKEUP_OTHER            // Other hardware interrupt source
} HAL_LPS_WAKEUP_SOURCE_T;

// =============================================================================
//  LPS Diagnostics & State Structure
// =============================================================================
typedef struct
{
    BOOL                    is_sleeping;
    HAL_LPS_SLEEP_MODE_T    last_mode;
    HAL_LPS_WAKEUP_SOURCE_T last_wakeup_source;
    UINT32                  last_sleep_ms;
    UINT32                  last_sleep_duration_ms;
    UINT32                  total_sleep_count;
    UINT32                  total_sleep_time_ms;
    UINT32                  auto_sleep_timeout_sec;     // 0 = disabled, 5 = 5s, 10 = 10s, 30 = 30s
    UINT32                  last_activity_ms;
} hal_lps_state_t;

// =============================================================================
//  LPS Driver Public API
// =============================================================================

/**
 * @brief Initialize the Low Power Scheme (LPS) engine and configure wake-up sources.
 */
void hal_LpsInit(void);

/**
 * @brief Enter low-power sleep mode.
 * 
 * @param mode LPS_MODE_LIGHT_SLEEP, LPS_MODE_DEEP_SLEEP, or LPS_MODE_TIMED_SLEEP.
 * @param timeout_ms Duration to sleep in ms (used for LPS_MODE_TIMED_SLEEP).
 * @return HAL_LPS_WAKEUP_SOURCE_T The source that triggered system wakeup.
 */
HAL_LPS_WAKEUP_SOURCE_T hal_LpsEnterSleep(HAL_LPS_SLEEP_MODE_T mode, UINT32 timeout_ms);

/**
 * @brief Register user activity (e.g. key press) to reset the auto-sleep countdown timer.
 */
void hal_LpsResetActivityTimer(void);

/**
 * @brief Set the auto-sleep timeout in seconds (0 to disable).
 */
void hal_LpsSetAutoSleepTimeout(UINT32 timeout_sec);

/**
 * @brief Get the configured auto-sleep timeout in seconds.
 */
UINT32 hal_LpsGetAutoSleepTimeout(void);

/**
 * @brief Check if the auto-sleep timeout has expired.
 */
BOOL hal_LpsIsAutoSleepExpired(void);

/**
 * @brief Get seconds remaining until auto-sleep triggers.
 */
UINT32 hal_LpsGetAutoSleepRemainingSec(void);

/**
 * @brief Retrieve a pointer to the live LPS state & statistics structure.
 */
const hal_lps_state_t* hal_LpsGetState(void);

/**
 * @brief Convert wakeup source enum to human-readable string.
 */
const char* hal_LpsGetWakeupSourceName(HAL_LPS_WAKEUP_SOURCE_T src);

/**
 * @brief Convert sleep mode enum to human-readable string.
 */
const char* hal_LpsGetSleepModeName(HAL_LPS_SLEEP_MODE_T mode);

#endif // _HAL_LPS_H_
