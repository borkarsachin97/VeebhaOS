/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * HAL General Purpose Analog-to-Digital Converter (GPADC) Header for RDA8809
 */

#ifndef _HAL_ANA_GPADC_H_
#define _HAL_ANA_GPADC_H_

#include "cs_types.h"

// Invalid / pending measurement value
#define HAL_ANA_GPADC_BAD_VALUE     0xFFFF

// Dedicated Battery GPADC Channel on RDA8809
#define HAL_ANA_GPADC_CHAN_BATTERY  HAL_ANA_GPADC_CHAN_7
#define HAL_ANA_GPADC_CHAN_CHARGER  HAL_ANA_GPADC_CHAN_6

// =============================================================================
// TYPES
// =============================================================================

typedef enum
{
    HAL_ANA_GPADC_CHAN_0 = 0,
    HAL_ANA_GPADC_CHAN_1,
    HAL_ANA_GPADC_CHAN_2,
    HAL_ANA_GPADC_CHAN_3,
    HAL_ANA_GPADC_CHAN_4,
    HAL_ANA_GPADC_CHAN_5,
    HAL_ANA_GPADC_CHAN_6,       // Charger / VBUS
    HAL_ANA_GPADC_CHAN_7,       // Battery (VBAT)
    HAL_ANA_GPADC_CHAN_QTY
} HAL_ANA_GPADC_CHAN_T;

typedef enum
{
    HAL_ANA_GPADC_ATP_122US = 0,
    HAL_ANA_GPADC_ATP_1MS,
    HAL_ANA_GPADC_ATP_10MS,
    HAL_ANA_GPADC_ATP_100MS,
    HAL_ANA_GPADC_ATP_250MS,
    HAL_ANA_GPADC_ATP_500MS,
    HAL_ANA_GPADC_ATP_1S,
    HAL_ANA_GPADC_ATP_2S,
    HAL_ANA_GPADC_ATP_QTY
} HAL_ANA_GPADC_ATP_T;

typedef UINT16 HAL_ANA_GPADC_MV_T;

// =============================================================================
// FUNCTIONS
// =============================================================================

/**
 * @brief Open and activate a GPADC channel for periodic measurement.
 * @param channel Channel to enable (0..7)
 * @param atp Acquisition Time Period
 */
void hal_AnaGpadcOpen(HAL_ANA_GPADC_CHAN_T channel, HAL_ANA_GPADC_ATP_T atp);

/**
 * @brief Close a GPADC channel.
 * @param channel Channel to disable
 */
void hal_AnaGpadcClose(HAL_ANA_GPADC_CHAN_T channel);

/**
 * @brief Read the raw 10-bit count (0..1023) for the specified channel.
 * @param channel Channel index
 * @return Raw ADC value, or HAL_ANA_GPADC_BAD_VALUE if not ready
 */
UINT16 hal_AnaGpadcGetRaw(HAL_ANA_GPADC_CHAN_T channel);

/**
 * @brief Read the voltage present at the GPADC pin in millivolts.
 * @param channel Channel index
 * @return Measured tension in mV, or HAL_ANA_GPADC_BAD_VALUE
 */
HAL_ANA_GPADC_MV_T hal_AnaGpadcGet(HAL_ANA_GPADC_CHAN_T channel);

/**
 * @brief Convert raw ADC count to millivolts using calibrated slope and intercept.
 * @param gpadcVal Raw 10-bit count
 * @return Tension in mV
 */
HAL_ANA_GPADC_MV_T hal_AnaGpadcGpadc2Volt(UINT16 gpadcVal);

/**
 * @brief Set custom two-point calibration data (low = 3.4V point, high = 4.2V point).
 */
void hal_AnaGpadcSetCalibData(UINT32 low, UINT32 high);

/**
 * @brief Read back current calibration data.
 */
void hal_AnaGpadcGetCalibData(UINT32 *low, UINT32 *high);

/**
 * @brief Get the full battery voltage in millivolts (GPADC Ch7 voltage * 3).
 * @return Battery voltage in mV (e.g. 3400..4200 mV), or HAL_ANA_GPADC_BAD_VALUE
 */
UINT16 hal_AnaGpadcGetBatteryVolt(void);

/**
 * @brief Get the charger / VBUS voltage in millivolts (GPADC Ch6 voltage * 6).
 * @return Charger voltage in mV (e.g. ~5000 mV for USB), or HAL_ANA_GPADC_BAD_VALUE
 */
UINT16 hal_AnaGpadcGetChargerVolt(void);

/**
 * @brief Get the estimated battery charge percentage (0..100%).
 * @return Percentage 0..100
 */
UINT8 hal_AnaGpadcGetBatteryPercent(void);

#endif // _HAL_ANA_GPADC_H_
