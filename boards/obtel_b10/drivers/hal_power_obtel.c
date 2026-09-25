/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * OBTEL B10 Hardware Abstraction Layer: Power Driver (RDA PMU)
 *
 * SPDX-License-Identifier: MIT
 */

#include "sdk/hal/hal_power.h"
#include "boards/obtel_b10/include/global_macros.h"
#include <stdint.h>
#include <stdbool.h>

#define RDA_PMU_STATUS          REG32(RDA_BASE_PMU + 0x00)
#define RDA_PMU_BAT_ADC         REG32(RDA_BASE_PMU + 0x04)
#define RDA_PMU_CTRL            REG32(RDA_BASE_PMU + 0x08)
#define RDA_PMU_RESET           REG32(RDA_BASE_PMU + 0x0C)

void hal_power_init(void)
{
    /* Enable Battery Voltage Monitor ADC */
    RDA_PMU_CTRL |= BIT(0);
}

uint8_t hal_power_get_battery_percent(void)
{
    uint32_t mv = hal_power_get_battery_voltage_mv();
    if (mv >= 4200) return 100;
    if (mv <= 3400) return 0;
    return (uint8_t)(((mv - 3400) * 100) / (4200 - 3400));
}

uint16_t hal_power_get_battery_voltage_mv(void)
{
    uint32_t raw_adc = RDA_PMU_BAT_ADC & 0x0FFF;
    /* 12-bit ADC mapped 0..4500mV */
    uint16_t mv = (uint16_t)((raw_adc * 4500) / 4095);
    if (mv < 3000) mv = 3700; /* Nominal fallback */
    return mv;
}

bool hal_power_is_charging(void)
{
    return (RDA_PMU_STATUS & BIT(2)) ? true : false;
}

void hal_power_off(void)
{
    RDA_PMU_CTRL |= BIT(8); /* Trigger PMU hardware power off */
    while (1) { }
}

void hal_power_reboot(void)
{
    RDA_PMU_RESET = 0xA5A50001; /* Watchdog reset trigger */
    while (1) { }
}
