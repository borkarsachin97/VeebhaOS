/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Hardware Abstraction Layer: Power & Battery Management Contract
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef HAL_POWER_H
#define HAL_POWER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * Initialize PMU / ADC battery monitoring circuitry.
 */
void hal_power_init(void);

/**
 * Read battery state-of-charge percentage (0-100).
 */
uint8_t hal_power_get_battery_pct(void);

/**
 * Read battery voltage in millivolts.
 */
uint16_t hal_power_get_battery_mv(void);

/**
 * Check if external charger (USB / AC) is connected.
 */
bool hal_power_is_charging(void);

/**
 * Request system reboot.
 */
void hal_power_reboot(void);

/**
 * Request system power off / deep sleep.
 */
void hal_power_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_POWER_H */
