/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Low Power Scheme (LPS) & Deep Sleep Driver Implementation for RDA8809 / RDA8955 SoC
 */

#include "cs_types.h"
#include "global_macros.h"
#include "sys_ctrl.h"
#include "sys_irq.h"
#include "timer.h"
#include "keypad.h"
#include "hal_ispi.h"
#include "hal_pwm.h"
#include "hal_lps.h"
#include "irq.h"
#include "usb_cdc.h"
#include "hal_usb.h"
#include "FreeRTOS.h"
#include "task.h"

// External Logging already declared in usb_cdc.h

#include "hal_gouda.h"
#include "lcd_panel.h"
#include "lcd_ili9225g.h"
#include "framebuffer.h"

// =============================================================================
//  PMU (RDA1203) Register Indices and Bits
// =============================================================================
#define PMU_REG_IRQ_SETTINGS        0x01
#define PMU_REG_LDO_SETTINGS        0x02
#define PMU_REG_LDO_ACTIVE1         0x03
#define PMU_REG_LDO_ACTIVE2         0x04
#define PMU_REG_LDO_ACTIVE5         0x07
#define PMU_REG_LDO_LP1             0x08
#define PMU_REG_LDO_LP2             0x09
#define PMU_REG_MISC_CTRL           0x18

// PMU IRQ Settings (0x01)
#define PMU_KEYIRQ_MASK             (1 << 6)
#define PMU_KEYIRQ_CLEAR            (1 << 12)
#define PMU_KEYIRQ_STATUS           (1 << 13)

// PMU LDO Settings (0x02)
#define PMU_LDO_FM_EN               (1 << 0)
#define PMU_LDO_BT_EN               (1 << 1)
#define PMU_LDO_KEY_EN              (1 << 2)
#define PMU_LDO_TSC_EN              (1 << 3)
#define PMU_LDO_VLCD_EN             (1 << 4)
#define PMU_LDO_VCAM_EN             (1 << 5)
#define PMU_LDO_VMIC_EN             (1 << 6)
#define PMU_LDO_VIBR_EN             (1 << 7)
#define PMU_LDO_VRF_EN              (1 << 9)
#define PMU_LDO_VABB_EN             (1 << 10)
#define PMU_LDO_VMMC_EN             (1 << 11)

// =============================================================================
//  Static State Management
// =============================================================================
static hal_lps_state_t g_lps_state = {
    .is_sleeping            = FALSE,
    .last_mode              = LPS_MODE_NONE,
    .last_wakeup_source     = LPS_WAKEUP_NONE,
    .last_sleep_ms          = 0,
    .last_sleep_duration_ms = 0,
    .total_sleep_count      = 0,
    .total_sleep_time_ms    = 0,
    .auto_sleep_timeout_sec = 0, // Disabled by default
    .last_activity_ms       = 0
};

// =============================================================================
//  hal_LpsInit
// =============================================================================
void hal_LpsInit(void)
{
    g_lps_state.last_activity_ms = timer_get_ms();

    // 1. Configure PMU Power Key interrupt over ISPI
    UINT16 irq_set = hal_PmuRead(PMU_REG_IRQ_SETTINGS);
    irq_set |= PMU_KEYIRQ_MASK;     // Enable Power Key IRQ
    irq_set |= PMU_KEYIRQ_CLEAR;    // Clear pending
    hal_PmuWrite(PMU_REG_IRQ_SETTINGS, irq_set);

    // 2. Configure hardware WakeUp mask in sysIrq controller
    hwp_sysIrq->WakeUp_Mask = SYS_IRQ_SYS_IRQ_KEYPAD |
                              SYS_IRQ_SYS_IRQ_PMU |
                              SYS_IRQ_SYS_IRQ_OS_TIMER |
                              SYS_IRQ_SYS_IRQ_GPIO;

    os_log_printf("[LPS] Low Power Scheme initialized (Auto-Sleep: %u s, Wakeup: Keypad/PowerKey/Timer)\n",
                  g_lps_state.auto_sleep_timeout_sec);
}

// =============================================================================
//  hal_LpsEnterSleep
// =============================================================================
HAL_LPS_WAKEUP_SOURCE_T hal_LpsEnterSleep(HAL_LPS_SLEEP_MODE_T mode, UINT32 timeout_ms)
{
    if (mode == LPS_MODE_NONE)
    {
        return LPS_WAKEUP_NONE;
    }

    g_lps_state.is_sleeping = TRUE;
    g_lps_state.last_mode   = mode;
    g_lps_state.last_sleep_ms = timer_get_ms();

    UINT8 saved_backlight = hal_PwmGetBacklight();
    UINT16 saved_pmu_ldo  = hal_PmuRead(PMU_REG_LDO_SETTINGS);
    UINT16 saved_pmu_lp1  = hal_PmuRead(PMU_REG_LDO_LP1);
    UINT32 saved_sel_clock = hwp_sysCtrl->Sel_Clock;
    UINT32 saved_cfg_clk_sys = hwp_sysCtrl->Cfg_Clk_Sys;

    // 0. Wait for user to fully release all keys before entering sleep so key release does not trigger immediate wake
    UINT32 release_start = timer_get_ms();
    while (keypad_is_any_pressed() || (hwp_keypad->KP_DATA_L != 0) || (hwp_keypad->KP_DATA_H != 0) || (hwp_keypad->KP_STATUS & KEYPAD_KP_ON))
    {
        timer_delay_ms(10);
        if ((timer_get_ms() - release_start) > 3000) break;
    }
    timer_delay_ms(30); // Debounce settle time

    UINT32 pre_key_press_count = keypad_get_press_count();
    UINT32 pre_key_irq_count   = keypad_get_irq_count();

    os_log_printf("\n======================================================\n");
    os_log_printf("[LPS] [t=%u ms] ENTERING %s (Timeout: %u ms)\n",
                  g_lps_state.last_sleep_ms, hal_LpsGetSleepModeName(mode), timeout_ms);
    os_log_printf("[LPS] Pre-sleep state: Sel_Clock=0x%08X, Cfg_Clk_Sys=0x%08X, PMU_LDO=0x%04X, Backlight=%u\n",
                  saved_sel_clock, saved_cfg_clk_sys, saved_pmu_ldo, saved_backlight);

    // -------------------------------------------------------------------------
    // 1. Suspend FreeRTOS Scheduler
    // -------------------------------------------------------------------------
    // Suspend FreeRTOS scheduler so tick interrupts cannot cause premature context switch.
    // Notice: CP0 Status IE remains 1 so hardware interrupts can wake the MIPS CPU from sleep.
    // FreeRTOS's vTaskSuspendAll() guarantees no context switch will occur during sleep or wake.
    vTaskSuspendAll();

    // -------------------------------------------------------------------------
    // 2. Cut Display & Peripherals (Deep Sleep & Timed Sleep)
    // -------------------------------------------------------------------------
    if (mode == LPS_MODE_DEEP_SLEEP || mode == LPS_MODE_TIMED_SLEEP)
    {
        // Smoothly fade backlight to 0
        hal_PwmSetBacklight(0);

        // Ensure GOUDA DMA blitter is completely idle before issuing sleep commands to panel
        hal_GoudaWaitIdle(100);

        // Put LCD panel to sleep
        const lcd_panel_t *panel = lcd_ili9225g_get_panel();
        if (panel && panel->sleep)
        {
            panel->sleep();
        }

        // Gate unused PMU LDO rails (V_MMC, V_CAM, V_MIC, V_IBR, FM)
        // Maintain V_LCD power so ILI9225G stays in clean sleep mode (<5uA) without power-rail brownout
        UINT16 sleep_pmu_ldo = saved_pmu_ldo;
        sleep_pmu_ldo &= ~(PMU_LDO_VMMC_EN | PMU_LDO_VCAM_EN | PMU_LDO_VMIC_EN | PMU_LDO_VIBR_EN | PMU_LDO_FM_EN);
        hal_PmuWrite(PMU_REG_LDO_SETTINGS, sleep_pmu_ldo);

        // Set PMU LP Settings: ensure bit 7 (vLcdOff) is 0 so VLCD stays alive
        UINT16 sleep_pmu_lp1 = saved_pmu_lp1 & ~(1 << 7);
        sleep_pmu_lp1 |= (1 << 2) | (1 << 3) | (1 << 5) | (1 << 6);
        hal_PmuWrite(PMU_REG_LDO_LP1, sleep_pmu_lp1);
    }
    else
    {
        // Light Sleep: Dim backlight to 15%
        hal_PwmSetBacklight(saved_backlight > 30 ? (saved_backlight / 3) : 10);
    }

    // -------------------------------------------------------------------------
    // 3. Clear and Arm Wake-Up Sources
    // -------------------------------------------------------------------------
    // Clear keypad events and arm ONLY Key-Down (EVT0) so key release doesn't cause wakeups
    hwp_keypad->KP_IRQ_CLR = 0xFFFFFFFF;
    hwp_keypad->KP_IRQ_MASK = KEYPAD_KP_EVT0_IRQ_MASK;

    // Clear PMU Power Key IRQ & re-arm mask
    UINT16 pmu_irq = hal_PmuRead(PMU_REG_IRQ_SETTINGS);
    pmu_irq |= PMU_KEYIRQ_MASK | PMU_KEYIRQ_CLEAR;
    hal_PmuWrite(PMU_REG_IRQ_SETTINGS, pmu_irq);

    // Configure wake-up timers and sysIrq masks
    if (mode == LPS_MODE_TIMED_SLEEP && timeout_ms > 0)
    {
        // Program OS Timer downcounter for timeout_ms
        UINT32 load_val = (timeout_ms * 16384) / 1000;
        if (load_val == 0) load_val = 1;
        hwp_timer->OSTimer_Ctrl = 0;
        hwp_timer->Timer_Irq_Clr = TIMER_OSTIMER_CLR;
        hwp_timer->OSTimer_Ctrl = TIMER_ENABLE | TIMER_LOAD | TIMER_LOADVAL(load_val);
        hwp_timer->Timer_Irq_Mask_Set = TIMER_OSTIMER_MASK;

        UINT32 sleep_irqs = SYS_IRQ_SYS_IRQ_KEYPAD | SYS_IRQ_SYS_IRQ_PMU | SYS_IRQ_SYS_IRQ_OS_TIMER;
        hwp_sysIrq->Mask_Set       = sleep_irqs;
        hwp_sysIrq->Pulse_Mask_Set = sleep_irqs;
        hwp_sysIrq->WakeUp_Mask    = sleep_irqs | SYS_IRQ_SYS_IRQ_GPIO;
    }
    else // LPS_MODE_DEEP_SLEEP or LPS_MODE_LIGHT_SLEEP
    {
        // Stop OS timer tick so CPU stays asleep until user interaction
        hwp_timer->OSTimer_Ctrl = 0;
        hwp_timer->Timer_Irq_Mask_Clr = TIMER_OSTIMER_MASK;
        hwp_timer->Timer_Irq_Clr = TIMER_OSTIMER_CLR;

        UINT32 sleep_irqs = SYS_IRQ_SYS_IRQ_KEYPAD | SYS_IRQ_SYS_IRQ_PMU;
        hwp_sysIrq->Mask_Set       = sleep_irqs;
        hwp_sysIrq->Pulse_Mask_Set = sleep_irqs;
        hwp_sysIrq->WakeUp_Mask    = sleep_irqs | SYS_IRQ_SYS_IRQ_GPIO;
    }

    // -------------------------------------------------------------------------
    // 4. Execute Hardware CPU Sleep (XCPU Clock Gating)
    // -------------------------------------------------------------------------
    // External PSRAM (0x82000280) requires active system PLL.
    // Low power is achieved by halting CPU pipeline via Cpu_Sleep clock gating register.
    __asm__ volatile (
        ".align 4\n\t"
        "li $6, 1\n\t"
        "sw $6, 0(%0)\n\t"
        "lw $6, 0(%0)\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        :
        : "r"(&(hwp_sysIrq->Cpu_Sleep))
        : "$6", "memory"
    );

    // -------------------------------------------------------------------------
    // 5. RESUME / WAKEUP DETECTED
    // -------------------------------------------------------------------------
    UINT32 wake_time_ms = timer_get_ms();
    UINT32 slept_ms = wake_time_ms - g_lps_state.last_sleep_ms;
    g_lps_state.last_sleep_duration_ms = slept_ms;
    g_lps_state.total_sleep_time_ms += slept_ms;
    g_lps_state.total_sleep_count++;
    g_lps_state.is_sleeping = FALSE;
    g_lps_state.last_activity_ms = wake_time_ms;

    // -------------------------------------------------------------------------
    // 6. Restore Clocks (104 MHz Fast PLL Clock)
    // -------------------------------------------------------------------------
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_UNLOCK;
    hwp_sysCtrl->Sel_Clock &= ~(SYS_CTRL_SYS_SEL_FAST_SLOW | SYS_CTRL_BB_SEL_FAST_SLOW);
    hwp_sysCtrl->Cfg_Clk_Sys = SYS_CTRL_SYS_FREQ_104M | SYS_CTRL_FORCE_DIV_UPDATE;
    hwp_sysCtrl->Clk_Sys_Enable = 0xFFFFFFFF;
    hwp_sysCtrl->Clk_Per_Enable = 0xFFFFFFFF;
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_LOCK;

    // -------------------------------------------------------------------------
    // 7. Restore FreeRTOS Periodic OS Tick Timer & System IRQs
    // -------------------------------------------------------------------------
    // Stop sleep countdown timer and clear interrupt flags
    hwp_timer->OSTimer_Ctrl = 0;
    hwp_timer->Timer_Irq_Mask_Clr = TIMER_OSTIMER_MASK;
    hwp_timer->Timer_Irq_Clr = TIMER_OSTIMER_CLR;
    volatile uint32_t dummy_clr = hwp_timer->Timer_Irq_Clr;
    (void)dummy_clr;
    hwp_sysIrq->Pulse_Clear = 0xFFFFFFFF;

    // Restart FreeRTOS periodic OS tick (100 Hz wrap/repeat)
    timer_os_start(configTICK_RATE_HZ);

    // Restore hardware system IRQ masks (both Level Mask and Pulse Mask!)
    UINT32 wake_irqs = SYS_IRQ_SYS_IRQ_KEYPAD | SYS_IRQ_SYS_IRQ_OS_TIMER | SYS_IRQ_SYS_IRQ_TIMERS | SYS_IRQ_SYS_IRQ_PMU;
    hwp_sysIrq->Mask_Set       = wake_irqs;
    hwp_sysIrq->Pulse_Mask_Set = wake_irqs;

    // Restore Keypad event masks (EVT0 and EVT1)
    hwp_keypad->KP_IRQ_CLR = 0xFFFFFFFF;
    hwp_keypad->KP_IRQ_MASK = KEYPAD_KP_EVT0_IRQ_MASK | KEYPAD_KP_EVT1_IRQ_MASK;

    // -------------------------------------------------------------------------
    // 8. Identify Wake-Up Source
    // -------------------------------------------------------------------------
    UINT32 post_key_press_count = keypad_get_press_count();
    UINT32 post_key_irq_count   = keypad_get_irq_count();
    UINT32 irq_cause = hwp_sysIrq->Cause;
    UINT16 pmu_status = hal_PmuRead(PMU_REG_IRQ_SETTINGS);
    HAL_LPS_WAKEUP_SOURCE_T wakeup_src = LPS_WAKEUP_OTHER;

    if (pmu_status & PMU_KEYIRQ_STATUS)
    {
        wakeup_src = LPS_WAKEUP_POWER_KEY;
    }
    else if ((mode == LPS_MODE_TIMED_SLEEP) && (timeout_ms > 0) && (slept_ms >= (timeout_ms * 9) / 10))
    {
        wakeup_src = LPS_WAKEUP_TIMER;
    }
    else if ((irq_cause & SYS_IRQ_SYS_IRQ_KEYPAD) ||
             (post_key_press_count != pre_key_press_count) ||
             (post_key_irq_count != pre_key_irq_count) ||
             keypad_is_any_pressed())
    {
        wakeup_src = LPS_WAKEUP_KEYPAD_MATRIX;
    }
    else if (irq_cause & (SYS_IRQ_SYS_IRQ_TIMERS | SYS_IRQ_SYS_IRQ_OS_TIMER))
    {
        wakeup_src = LPS_WAKEUP_TIMER;
    }
    else if (irq_cause & SYS_IRQ_SYS_IRQ_USBC)
    {
        wakeup_src = LPS_WAKEUP_USB;
    }
    else if (irq_cause & SYS_IRQ_SYS_IRQ_PMU)
    {
        wakeup_src = LPS_WAKEUP_POWER_KEY;
    }
    else
    {
        wakeup_src = LPS_WAKEUP_KEYPAD_MATRIX;
    }
    g_lps_state.last_wakeup_source = wakeup_src;

    // Clear PMU key interrupt
    hal_PmuWrite(PMU_REG_IRQ_SETTINGS, pmu_status | PMU_KEYIRQ_CLEAR);

    // -------------------------------------------------------------------------
    // 9. Restore PMU LDO Rails & Display Panel
    // -------------------------------------------------------------------------
    if (mode == LPS_MODE_DEEP_SLEEP || mode == LPS_MODE_TIMED_SLEEP)
    {
        // Restore PMU active and low-power LDO configuration
        hal_PmuWrite(PMU_REG_LDO_SETTINGS, saved_pmu_ldo);
        hal_PmuWrite(PMU_REG_LDO_LP1, saved_pmu_lp1);
        timer_delay_ms(10);

        // Wake LCD panel controller from sleep mode
        const lcd_panel_t *panel = lcd_ili9225g_get_panel();
        if (panel && panel->wakeup)
        {
            panel->wakeup();
        }

        // Restore user backlight brightness after screen is fully ready
        hal_PwmSetBacklight(saved_backlight);
    }
    else
    {
        hal_PwmSetBacklight(saved_backlight);
    }

    // -------------------------------------------------------------------------
    // 10. Resume FreeRTOS Multitasking
    // -------------------------------------------------------------------------
    xTaskResumeAll();

    // -------------------------------------------------------------------------
    // 11. Output Wakeup Diagnostics
    // -------------------------------------------------------------------------
    os_log_printf("[LPS] >>> SYSTEM WOKE UP! <<< Trigger: %s (Cause: 0x%08X, PMU: 0x%04X), Slept: %u ms\n",
                  hal_LpsGetWakeupSourceName(wakeup_src), irq_cause, pmu_status, slept_ms);
    os_log_printf("[LPS] System clock restored to 104 MHz. Backlight restored to %u/255.\n", saved_backlight);
    os_log_printf("======================================================\n\n");

    return wakeup_src;
}

// =============================================================================
//  Activity Timer & Auto-Sleep
// =============================================================================
void hal_LpsResetActivityTimer(void)
{
    g_lps_state.last_activity_ms = timer_get_ms();
}

void hal_LpsSetAutoSleepTimeout(UINT32 timeout_sec)
{
    g_lps_state.auto_sleep_timeout_sec = timeout_sec;
    g_lps_state.last_activity_ms       = timer_get_ms();
}

UINT32 hal_LpsGetAutoSleepTimeout(void)
{
    return g_lps_state.auto_sleep_timeout_sec;
}

BOOL hal_LpsIsAutoSleepExpired(void)
{
    if (g_lps_state.auto_sleep_timeout_sec == 0 || g_lps_state.is_sleeping)
    {
        return FALSE;
    }

    UINT32 elapsed_ms = timer_get_ms() - g_lps_state.last_activity_ms;
    return (elapsed_ms >= (g_lps_state.auto_sleep_timeout_sec * 1000));
}

UINT32 hal_LpsGetAutoSleepRemainingSec(void)
{
    if (g_lps_state.auto_sleep_timeout_sec == 0)
    {
        return 0;
    }

    UINT32 elapsed_ms = timer_get_ms() - g_lps_state.last_activity_ms;
    UINT32 timeout_ms = g_lps_state.auto_sleep_timeout_sec * 1000;
    if (elapsed_ms >= timeout_ms)
    {
        return 0;
    }
    return (timeout_ms - elapsed_ms) / 1000;
}

const hal_lps_state_t* hal_LpsGetState(void)
{
    return &g_lps_state;
}

const char* hal_LpsGetWakeupSourceName(HAL_LPS_WAKEUP_SOURCE_T src)
{
    switch (src)
    {
        case LPS_WAKEUP_KEYPAD_MATRIX: return "KEYPAD_MATRIX";
        case LPS_WAKEUP_POWER_KEY:     return "PMU_POWER_KEY";
        case LPS_WAKEUP_TIMER:         return "OS_TIMER";
        case LPS_WAKEUP_GPADC:         return "GPADC_BATTERY";
        case LPS_WAKEUP_USB:           return "USB_CABLE";
        case LPS_WAKEUP_OTHER:         return "HARDWARE_IRQ";
        default:                       return "NONE";
    }
}

const char* hal_LpsGetSleepModeName(HAL_LPS_SLEEP_MODE_T mode)
{
    switch (mode)
    {
        case LPS_MODE_LIGHT_SLEEP: return "LIGHT_SLEEP (Screen ON, Clock Gated)";
        case LPS_MODE_DEEP_SLEEP:  return "DEEP_SLEEP (Screen OFF, 32kHz, LDO Cut)";
        case LPS_MODE_TIMED_SLEEP: return "TIMED_SLEEP (Auto-wake Timer)";
        default:                   return "NONE";
    }
}
