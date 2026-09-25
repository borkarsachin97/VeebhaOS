/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * System Control (sys_ctrl) HAL Implementation for RDA8809
 */

#include "cs_types.h"
#include "global_macros.h"
#include "sys_ctrl.h"
#include "hal_sys_ctrl.h"
#include "timer.h"

extern void os_log_printf(const char *fmt, ...);

static HAL_SYS_FREQ_T g_currentCpuFreq = HAL_SYS_FREQ_104M;
static HAL_SYS_RESET_CAUSE_T g_bootResetCause = HAL_SYS_RESET_CAUSE_UNKNOWN;
static UINT32 g_rawResetCause = 0;

// =============================================================================
// hal_SysProtectUnlock / Lock
// =============================================================================
void hal_SysProtectUnlock(void)
{
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_UNLOCK;
}

void hal_SysProtectLock(void)
{
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_LOCK;
}

// =============================================================================
// hal_SysCtrlInit - Initialize SysCtrl, read reset cause & configure baseline
// =============================================================================
void hal_SysCtrlInit(void)
{
    // 1. Capture reset cause from hardware before anything can clear it
    g_rawResetCause = hwp_sysCtrl->Reset_Cause;

    if (g_rawResetCause & SYS_CTRL_WATCHDOG_RESET_HAPPENED)
    {
        g_bootResetCause = HAL_SYS_RESET_CAUSE_WATCHDOG;
    }
    else if (g_rawResetCause & SYS_CTRL_GLOBALSOFT_RESET_HAPPENED)
    {
        g_bootResetCause = HAL_SYS_RESET_CAUSE_SOFT_RESET;
    }
    else if (g_rawResetCause & SYS_CTRL_HOSTDEBUG_RESET_HAPPENED)
    {
        g_bootResetCause = HAL_SYS_RESET_CAUSE_HOST_DEBUG;
    }
    else if (g_rawResetCause & SYS_CTRL_ALARMCAUSE_HAPPENED)
    {
        g_bootResetCause = HAL_SYS_RESET_CAUSE_ALARM;
    }
    else
    {
        g_bootResetCause = HAL_SYS_RESET_CAUSE_POWER_ON;
    }

    // 2. Unlock protected registers
    hal_SysProtectUnlock();

    // 3. Clear all system peripheral and baseband controller resets
    hwp_sysCtrl->Sys_Rst_Clr = 0x7FFFFFFF;
    hwp_sysCtrl->BB_Rst_Clr  = SYS_CTRL_CLR_RST_AIF | SYS_CTRL_CLR_RST_IFC2 | SYS_CTRL_CLR_RST_BB_IFC;

    // Explicitly enable all system, peripheral, and baseband clocks (un-gate AHB DMA bus, EBC, GOUDA)
    hwp_sysCtrl->Clk_Sys_Enable = 0xFFFFFFFF;
    hwp_sysCtrl->Clk_Per_Enable = 0xFFFFFFFF;
    hwp_sysCtrl->Clk_BB_Enable  = 0xFFFFFFFF;

    // Enable hardware cache RAM automatic mode for XCPU and BCPU (from reference SDK)
    hwp_sysCtrl->Cfg_Cpus_Cache_Ram_Disable |=
        SYS_CTRL_XCPU_CACHE_RAM_DISABLE(1) | SYS_CTRL_BCPU_CACHE_RAM_DISABLE(1);

    // 4. Set CPU clock to maximum 312 MHz for highest performance
    hwp_sysCtrl->Cfg_Clk_Sys = SYS_CTRL_SYS_FREQ_312M | SYS_CTRL_FORCE_DIV_UPDATE;
    hwp_sysCtrl->Sel_Clock  &= ~(SYS_CTRL_SYS_SEL_FAST_SLOW | SYS_CTRL_BB_SEL_FAST_SLOW);
    g_currentCpuFreq = HAL_SYS_FREQ_312M;

    // 5. Lock protected registers
    hal_SysProtectLock();

    os_log_printf("[SYS_CTRL] Driver initialized. CPU: %u MHz, ResetCause: %s (Raw: 0x%08X)\n",
                  hal_SysGetCpuFreqHz() / 1000000, hal_SysGetResetCauseString(), g_rawResetCause);
}

// =============================================================================
// hal_SysSetCpuFreq - Change XCPU core clock frequency
// =============================================================================
BOOL hal_SysSetCpuFreq(HAL_SYS_FREQ_T freq)
{
    UINT32 cfgVal = 0;

    switch (freq)
    {
        case HAL_SYS_FREQ_32K:
            // Switch to 32kHz slow clock
            hal_SysProtectUnlock();
            hwp_sysCtrl->Sel_Clock |= SYS_CTRL_SYS_SEL_FAST_SLOW;
            hal_SysProtectLock();
            g_currentCpuFreq = freq;
            return TRUE;

        case HAL_SYS_FREQ_26M:  cfgVal = SYS_CTRL_SYS_FREQ_26M;  break;
        case HAL_SYS_FREQ_39M:  cfgVal = SYS_CTRL_SYS_FREQ_39M;  break;
        case HAL_SYS_FREQ_52M:  cfgVal = SYS_CTRL_SYS_FREQ_52M;  break;
        case HAL_SYS_FREQ_78M:  cfgVal = SYS_CTRL_SYS_FREQ_78M;  break;
        case HAL_SYS_FREQ_89M:  cfgVal = SYS_CTRL_SYS_FREQ_89M;  break;
        case HAL_SYS_FREQ_104M: cfgVal = SYS_CTRL_SYS_FREQ_104M; break;
        case HAL_SYS_FREQ_113M: cfgVal = SYS_CTRL_SYS_FREQ_113M; break;
        case HAL_SYS_FREQ_125M: cfgVal = SYS_CTRL_SYS_FREQ_125M; break;
        case HAL_SYS_FREQ_139M: cfgVal = SYS_CTRL_SYS_FREQ_139M; break;
        case HAL_SYS_FREQ_156M: cfgVal = SYS_CTRL_SYS_FREQ_156M; break;
        case HAL_SYS_FREQ_178M: cfgVal = SYS_CTRL_SYS_FREQ_178M; break;
        case HAL_SYS_FREQ_208M: cfgVal = SYS_CTRL_SYS_FREQ_208M; break;
        case HAL_SYS_FREQ_250M: cfgVal = SYS_CTRL_SYS_FREQ_250M; break;
        case HAL_SYS_FREQ_312M: cfgVal = SYS_CTRL_SYS_FREQ_312M; break;
        default:
            return FALSE;
    }

    hal_SysProtectUnlock();

    // Ensure fast PLL clock is selected
    hwp_sysCtrl->Sel_Clock &= ~SYS_CTRL_SYS_SEL_FAST_SLOW;

    // Apply frequency divider update
    hwp_sysCtrl->Cfg_Clk_Sys = cfgVal | SYS_CTRL_FORCE_DIV_UPDATE;

    hal_SysProtectLock();

    g_currentCpuFreq = freq;
    return TRUE;
}

HAL_SYS_FREQ_T hal_SysGetCpuFreq(void)
{
    return g_currentCpuFreq;
}

UINT32 hal_SysGetCpuFreqHz(void)
{
    return (UINT32)g_currentCpuFreq;
}

// =============================================================================
// Peripheral Clock Control
// =============================================================================
void hal_SysClkPerEnable(UINT32 perMask)
{
    hwp_sysCtrl->Clk_Per_Enable = perMask;
}

void hal_SysClkPerDisable(UINT32 perMask)
{
    hal_SysProtectUnlock();
    hwp_sysCtrl->Clk_Per_Disable = perMask;
    hal_SysProtectLock();
}

// =============================================================================
// System Bus Clock Control
// =============================================================================
void hal_SysClkSysEnable(UINT32 sysMask)
{
    hwp_sysCtrl->Clk_Sys_Enable = sysMask;
}

void hal_SysClkSysDisable(UINT32 sysMask)
{
    hal_SysProtectUnlock();
    hwp_sysCtrl->Clk_Sys_Disable = sysMask;
    hal_SysProtectLock();
}

// =============================================================================
// Peripheral Resets
// =============================================================================
void hal_SysResetPulse(UINT32 rstMask)
{
    hal_SysProtectUnlock();
    hwp_sysCtrl->Sys_Rst_Set = rstMask;
    timer_delay_us(10);
    hwp_sysCtrl->Sys_Rst_Clr = rstMask;
    hal_SysProtectLock();
}

void hal_SysResetAssert(UINT32 rstMask)
{
    hal_SysProtectUnlock();
    hwp_sysCtrl->Sys_Rst_Set = rstMask;
    hal_SysProtectLock();
}

void hal_SysResetRelease(UINT32 rstMask)
{
    hwp_sysCtrl->Sys_Rst_Clr = rstMask;
}

// =============================================================================
// Software Reboot
// =============================================================================
void hal_SysSoftReset(void)
{
    hal_SysProtectUnlock();
    hwp_sysCtrl->Sys_Rst_Set = SYS_CTRL_SOFT_RST;
    while (1);
}

// =============================================================================
// Reset Cause Queries
// =============================================================================
HAL_SYS_RESET_CAUSE_T hal_SysGetResetCause(void)
{
    return g_bootResetCause;
}

UINT32 hal_SysGetRawResetCause(void)
{
    return g_rawResetCause;
}

const char* hal_SysGetResetCauseString(void)
{
    switch (g_bootResetCause)
    {
        case HAL_SYS_RESET_CAUSE_POWER_ON:   return "POWER_ON";
        case HAL_SYS_RESET_CAUSE_WATCHDOG:   return "WATCHDOG_RESET";
        case HAL_SYS_RESET_CAUSE_SOFT_RESET: return "GLOBAL_SOFT_RESET";
        case HAL_SYS_RESET_CAUSE_HOST_DEBUG: return "HOST_DEBUG_RESET";
        case HAL_SYS_RESET_CAUSE_ALARM:      return "ALARM_WAKEUP";
        default:                             return "UNKNOWN";
    }
}

// =============================================================================
// Clock Outputs
// =============================================================================
void hal_SysSetupOutClock(UINT8 divider)
{
    hal_SysProtectUnlock();
    hwp_sysCtrl->Cfg_Clk_Out = SYS_CTRL_CLKOUT_SEL_DIVIDER | SYS_CTRL_CLKOUT_DIVIDER(divider & 31);
    hal_SysProtectLock();
}

void hal_SysSetupAuxClock(BOOL enable)
{
    hwp_sysCtrl->Cfg_Clk_Auxclk = enable ? SYS_CTRL_AUXCLK_EN_ENABLE : SYS_CTRL_AUXCLK_EN_DISABLE;
}
