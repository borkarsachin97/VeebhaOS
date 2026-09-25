/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Simulator Hardware Abstraction Layer: Power & Battery Subsystem
 *
 * SPDX-License-Identifier: MIT
 */

#include "sdk/hal/hal_power.h"
#include <stdio.h>
#include <stdlib.h>

static uint8_t s_sim_battery_pct = 85;
static uint16_t s_sim_battery_mv = 4120;
static bool s_sim_is_charging = false;

void hal_power_init(void)
{
    s_sim_battery_pct = 85;
    s_sim_battery_mv = 4120;
    s_sim_is_charging = false;
}

uint8_t hal_power_get_battery_pct(void)
{
    return s_sim_battery_pct;
}

uint16_t hal_power_get_battery_mv(void)
{
    return s_sim_battery_mv;
}

bool hal_power_is_charging(void)
{
    return s_sim_is_charging;
}

void hal_power_reboot(void)
{
    printf("[HAL_POWER_SIM] Reboot requested.\n");
}

void hal_power_shutdown(void)
{
    printf("[HAL_POWER_SIM] Shutdown requested.\n");
}
