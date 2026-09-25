/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * HAL PWM, PWL, PWT, and LPG Driver Implementation for RDA8809
 */

#include "cs_types.h"
#include "global_macros.h"
#include "sys_ctrl.h"
#include "pwm.h"
#include "hal_pwm.h"
#include "hal_ispi.h"
#include "hal_pmd.h"

// Base divider: 13 MHz / 10 = 1.3 MHz PWM Base Frequency
#define HAL_PWM_BASE_DIVIDER    10
#define HAL_PWM_FREQ            (13000000 / HAL_PWM_BASE_DIVIDER)

// PMU Register Offsets for Backlight & Power on RDA8809
#define RDA_ADDR_LDO_ACTIVE_SETTING2 0x04
#define RDA_ADDR_LED_SETTING1        0x19
#define RDA_ADDR_LED_SETTING2        0x1A
#define RDA_ADDR_THERMAL_CALIBRATION 0x36

// Bitfield definitions
#define RDA_PMU_RGB_LED_OFF          (1 << 0)
#define RDA_PMU_DIM_BL_REG           (1 << 0)
#define RDA_PMU_DIM_BL_DR            (1 << 1)
#define RDA_PMU_BL_OFF_ACT           (1 << 2)
#define RDA_PMU_BL_IBIT_LP(n)        (((n) & 0xF) << 4)
#define RDA_PMU_BL_IBIT_ACT(n)       (((n) & 0xF) << 8)
#define RDA_PMU_BL_IBIT_PON(n)       (((n) & 0xF) << 12)

static UINT8 g_backlight_level = 0;
static BOOL  g_pwm_initialized = FALSE;

// =============================================================================
// hal_PwmInit
// =============================================================================
void hal_PwmInit(void)
{
    if (g_pwm_initialized) return;

    // 1. Enable PWM clock and clear reset in sysCtrl
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_UNLOCK;
    
    // Clear PWM reset
    hwp_sysCtrl->Sys_Rst_Clr = SYS_CTRL_CLR_RST_PWM;
    
    // Enable divided PWM system clock
    hwp_sysCtrl->Clk_Sys_Enable = SYS_CTRL_ENABLE_SYSD_PWM;

    // Setup PWM clock divider for ~1.3 MHz base clock from 78 MHz system clock:
    // divider = (78M / 1.3M) - 1 = 59
    hwp_sysCtrl->Cfg_Clk_PWM = 59;

    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_LOCK;

    // 2. Initialize hardware registers to known off state
    hwp_pwm->PWT_Config  = 0;
    hwp_pwm->LPG_Config  = 0;
    hwp_pwm->PWL0_Config = PWM_PWL0_SET_OE | PWM_PWL0_FORCE_L;
    hwp_pwm->PWL1_Config = PWM_PWL1_SET_OE | PWM_PWL1_FORCE_L;

    g_pwm_initialized = TRUE;
}

// =============================================================================
// hal_PwlSelLevel
// =============================================================================
void hal_PwlSelLevel(HAL_PWL_ID_T id, UINT8 level)
{
    hal_PwmInit();

    UINT32 val;

    if (id == HAL_PWL_0)
    {
        val = PWM_PWL0_SET_OE; // Always assert output enable
        if (level == 0)
        {
            val |= PWM_PWL0_FORCE_L;
        }
        else if (level == 0xFF)
        {
            val |= PWM_PWL0_FORCE_H;
        }
        else
        {
            val |= (PWM_PWL0_EN_H | PWM_PWL_MIN(level));
        }
        hwp_pwm->PWL0_Config = val;
    }
    else if (id == HAL_PWL_1)
    {
        val = PWM_PWL1_SET_OE; // Always assert output enable
        if (level == 0)
        {
            val |= PWM_PWL1_FORCE_L;
        }
        else if (level == 0xFF)
        {
            val |= PWM_PWL1_FORCE_H;
        }
        else
        {
            val |= (PWM_PWL1_EN_H | PWM_PWL1_THRESHOLD(level));
        }
        hwp_pwm->PWL1_Config = val;
    }
}

// =============================================================================
// hal_PwlGlow
// =============================================================================
void hal_PwlGlow(UINT8 levelMin, UINT8 levelMax, UINT8 pulsePeriod, BOOL pulse)
{
    hal_PwmInit();

    UINT32 val = PWM_PWL0_SET_OE;

    if (pulse)
    {
        val |= PWM_PWL_MIN(levelMin) | PWM_PWL_MAX(levelMax)
             | PWM_PWL_PULSE_PER(pulsePeriod) | PWM_PWL_PULSE_EN
             | PWM_PWL0_EN_H;
    }
    else
    {
        if (levelMin == 0)
        {
            val |= PWM_PWL0_FORCE_L;
        }
        else if (levelMin == 0xFF)
        {
            val |= PWM_PWL0_FORCE_H;
        }
        else
        {
            val |= (PWM_PWL0_EN_H | PWM_PWL_MIN(levelMin));
        }
    }

    hwp_pwm->PWL0_Config = val;
}

// =============================================================================
// hal_BuzzStart
// =============================================================================
void hal_BuzzStart(UINT16 noteFreq, UINT16 level)
{
    hal_PwmInit();

    if (level == 0 || noteFreq < 152)
    {
        hwp_pwm->PWT_Config = 0;
        return;
    }

    if (level > 100) level = 100;

    UINT32 noteDiv = HAL_PWM_FREQ / ((UINT32)noteFreq * 8);
    UINT32 dutyCmp = (noteDiv * (UINT32)level) / 100;

    if (noteDiv >= (1 << 11)) noteDiv = (1 << 11) - 1;
    if (dutyCmp >= (1 << 10)) dutyCmp = (1 << 10) - 1;
    if (dutyCmp < 8) dutyCmp = 8;

    hwp_pwm->PWT_Config = (PWM_PWT_PERIOD(noteDiv) | PWM_PWT_DUTY(dutyCmp) | PWM_PWT_ENABLE);
}

// =============================================================================
// hal_BuzzStop
// =============================================================================
void hal_BuzzStop(void)
{
    hwp_pwm->PWT_Config = 0;
}

// =============================================================================
// hal_LpgStart
// =============================================================================
void hal_LpgStart(HAL_LPG_PERIOD_T period, HAL_LPG_ON_T onTime)
{
    hal_PwmInit();

    hwp_pwm->LPG_Config = 0;
    hwp_pwm->LPG_Config = PWM_LPG_PERIOD(period)
                        | PWM_LPG_ONTIME(onTime)
                        | PWM_LPG_RESET_L;
}

// =============================================================================
// hal_LpgStop
// =============================================================================
void hal_LpgStop(void)
{
    hwp_pwm->LPG_Config = 0;
}

// =============================================================================
// hal_PwmSetBacklight
// =============================================================================
void hal_PwmSetBacklight(UINT8 level)
{
    g_backlight_level = level;

    // 1. Control Baseband PWL0 and PWL1
    // Keep PWL lines in clean static DC (FORCE_H / FORCE_L) rather than active 5 kHz PWM
    // switching to prevent PWM switching noise/buzz from coupling into the Speaker PA.
    // LCD backlight brightness is smoothly controlled via PMU constant-current sinks below.
    if (level > 0)
    {
        hwp_pwm->PWL0_Config = PWM_PWL0_SET_OE | PWM_PWL0_FORCE_H;
        hwp_pwm->PWL1_Config = PWM_PWL1_SET_OE | PWM_PWL1_FORCE_H;
    }
    else
    {
        hwp_pwm->PWL0_Config = PWM_PWL0_SET_OE | PWM_PWL0_FORCE_L;
        hwp_pwm->PWL1_Config = PWM_PWL1_SET_OE | PWM_PWL1_FORCE_L;
    }

    // 2. Control PMU Backlight Current & Output Driver via PMD
    // Map level 0..255 to PMD 8-step range 0..7
    UINT8 pmd_level = (UINT8)(((UINT32)level * 7 + 127) / 255);
    hal_PmdSetBacklight(pmd_level);
}

// =============================================================================
// hal_PwmGetBacklight
// =============================================================================
UINT8 hal_PwmGetBacklight(void)
{
    return g_backlight_level;
}
