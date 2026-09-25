/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Dedicated Hardware Backlight HAL Driver Implementation for RDA8809
 * Controls LCD panel illumination via PMU (RDA1203/ISPI) and Baseband PWL.
 */

#include "cs_types.h"
#include "global_macros.h"
#include "hal_ispi.h"
#include "hal_pwm.h"
#include "hal_backlight.h"

extern void os_log_printf(const char *fmt, ...);

// PMU Registers for Backlight
#define RDA_ADDR_LED_SETTING1           0x19
#define RDA_ADDR_LED_SETTING2           0x1A

// LED Setting 1 (0x19)
#define RDA_PMU_DIM_BL_REG              (1 << 0)
#define RDA_PMU_DIM_BL_DR               (1 << 1)

// LED Setting 2 (0x1A)
#define RDA_PMU_BL_OFF_LP               (1 << 1)
#define RDA_PMU_BL_OFF_ACT              (1 << 2)
#define RDA_PMU_BL_OFF_PON              (1 << 3)
#define RDA_PMU_BL_IBIT_LP(n)           (((n) & 0x0F) << 4)
#define RDA_PMU_BL_IBIT_ACT(n)          (((n) & 0x0F) << 8)
#define RDA_PMU_BL_IBIT_PON(n)          (((n) & 0x0F) << 12)

static uint8_t g_backlight_level = 180;
static bool    g_backlight_init_done = false;

// =============================================================================
// hal_BacklightInit
// =============================================================================
void hal_BacklightInit(void)
{
    if (g_backlight_init_done) return;

    hal_IspiInit();
    hal_PwmInit();

    // Enable direct PMU DIM control on Backlight
    uint16_t reg19 = hal_PmuRead(RDA_ADDR_LED_SETTING1);
    reg19 |= RDA_PMU_DIM_BL_DR;
    hal_PmuWrite(RDA_ADDR_LED_SETTING1, reg19);

    g_backlight_init_done = true;
    hal_BacklightSetLevel(g_backlight_level);

    os_log_printf("[BACKLIGHT] Driver initialized (PMU Opal ISPI Reg 0x1A + Baseband PWL, Level: %u/255)\n",
                  g_backlight_level);
}

// =============================================================================
// hal_BacklightSetLevel
// =============================================================================
void hal_BacklightSetLevel(uint8_t level)
{
    g_backlight_level = level;

    // 1. Control Baseband PWL0 and PWL1
    hal_PwlSelLevel(HAL_PWL_0, level);
    hal_PwlSelLevel(HAL_PWL_1, level);

    // 2. Control PMU Backlight Current & Output Driver via ISPI
    uint16_t ibit = (uint16_t)((level * 15 + 127) / 255);
    if (level > 0 && ibit == 0) ibit = 1;
    if (ibit > 15) ibit = 15;

    // 2a. PMU Reg 0x1A (LED_SETTING2)
    uint16_t led2;
    if (level == 0)
    {
        led2 = RDA_PMU_BL_OFF_ACT | RDA_PMU_BL_OFF_PON | RDA_PMU_BL_OFF_LP;
    }
    else
    {
        led2 = RDA_PMU_BL_IBIT_ACT(ibit) | RDA_PMU_BL_IBIT_PON(ibit) | RDA_PMU_BL_IBIT_LP(ibit);
    }
    hal_PmuWrite(RDA_ADDR_LED_SETTING2, led2);

    // 2b. PMU Reg 0x19 (LED_SETTING1)
    uint16_t led1 = RDA_PMU_DIM_BL_DR;
    if (level == 0)
    {
        led1 |= RDA_PMU_DIM_BL_REG; // OFF
    }
    hal_PmuWrite(RDA_ADDR_LED_SETTING1, led1);
}

// =============================================================================
// hal_BacklightGetLevel
// =============================================================================
uint8_t hal_BacklightGetLevel(void)
{
    return g_backlight_level;
}

void hal_BacklightOn(void)
{
    hal_BacklightSetLevel(g_backlight_level ? g_backlight_level : 180);
}

void hal_BacklightOff(void)
{
    hal_BacklightSetLevel(0);
}

void hal_BacklightToggle(void)
{
    if (g_backlight_level > 0)
        hal_BacklightOff();
    else
        hal_BacklightOn();
}
