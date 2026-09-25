/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Dedicated Hardware Backlight HAL Driver Header for RDA8809
 * Controls LCD panel illumination via internal PMU (RDA1203/ISPI) and Baseband PWL.
 */

#ifndef _HAL_BACKLIGHT_H_
#define _HAL_BACKLIGHT_H_

#include "cs_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
//  CONSTANTS & PRESETS
// =============================================================================
#define BACKLIGHT_LEVEL_OFF         0
#define BACKLIGHT_LEVEL_MIN         10
#define BACKLIGHT_LEVEL_LOW         64
#define BACKLIGHT_LEVEL_MED         128
#define BACKLIGHT_LEVEL_HIGH        192
#define BACKLIGHT_LEVEL_MAX         255

// =============================================================================
//  PUBLIC BACKLIGHT API
// =============================================================================

/**
 * @brief Initialize the Backlight subsystem (PMU LDO, charge pump, bias, and PWL).
 */
void hal_BacklightInit(void);

/**
 * @brief Set the display backlight brightness.
 * @param level Brightness level from 0 (OFF) to 255 (Maximum).
 */
void hal_BacklightSetLevel(uint8_t level);

/**
 * @brief Get the current display backlight brightness level (0..255).
 * @return Current level.
 */
uint8_t hal_BacklightGetLevel(void);

/**
 * @brief Step backlight brightness up or down by a relative delta.
 * @param step Positive or negative step delta.
 */
void hal_BacklightStep(int16_t step);

/**
 * @brief Enable or disable the internal LCD backlight Charge Pump boost circuit.
 * @param enable true to enable boost (~4.5V-5V), false to disable.
 */
void hal_BacklightEnableChargePump(bool enable);

/**
 * @brief Check if the Charge Pump boost is currently enabled.
 * @return true if enabled, false otherwise.
 */
bool hal_BacklightIsChargePumpEnabled(void);

#ifdef __cplusplus
}
#endif

#endif // _HAL_BACKLIGHT_H_
