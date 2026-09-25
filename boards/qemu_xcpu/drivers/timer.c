/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Timer Driver Implementation for RDA8809 / RDA8955 SoC
 */

#include "cs_types.h"
#include "global_macros.h"
#include "sys_ctrl.h"
#include "timer.h"
#include "os_vector_table.h"

// 8192 Hz base hardware clock tick frequency (32768 Hz oscillator / 4)
#define HW_TIMER_FREQ_HZ    8192

void timer_init(void)
{
    // Clear timer module reset in sysCtrl
    hwp_sysCtrl->Sys_Rst_Clr = SYS_CTRL_CLR_RST_TIMER;

    // Clear watchdog timer to prevent unexpected hardware resets during testing
    hwp_timer->WDTimer_Ctrl = 0;
    hwp_timer->WDTimer_LoadVal = 0;
}

UINT32 timer_get_ticks(void)
{
    return hwp_timer->HWTimer_CurVal;
}

UINT32 timer_get_ms(void)
{
    UINT64 ticks = hwp_timer->HWTimer_CurVal;
    return (UINT32)((ticks * 1000) / HW_TIMER_FREQ_HZ);
}

UINT32 timer_get_sec(void)
{
    return hwp_timer->HWTimer_CurVal / HW_TIMER_FREQ_HZ;
}

void timer_delay_ms(UINT32 ms)
{
    UINT32 start = hwp_timer->HWTimer_CurVal;
    UINT32 delay_ticks = (ms * HW_TIMER_FREQ_HZ) / 1000;
    while ((hwp_timer->HWTimer_CurVal - start) < delay_ticks);
}

void timer_delay_us(UINT32 us)
{
    UINT32 start = hwp_timer->HWTimer_CurVal;
    UINT32 delay_ticks = (us * HW_TIMER_FREQ_HZ) / 1000000;
    if (delay_ticks == 0) delay_ticks = 1;
    while ((hwp_timer->HWTimer_CurVal - start) < delay_ticks);
}

void timer_stopwatch_reset(timer_stopwatch_t *sw)
{
    if (sw)
    {
        sw->start_ticks = 0;
        sw->stop_ticks = 0;
        sw->running = 0;
    }
}

void timer_stopwatch_start(timer_stopwatch_t *sw)
{
    if (sw && !sw->running)
    {
        sw->start_ticks = hwp_timer->HWTimer_CurVal - (sw->stop_ticks - sw->start_ticks);
        sw->running = 1;
    }
}

void timer_stopwatch_stop(timer_stopwatch_t *sw)
{
    if (sw && sw->running)
    {
        sw->stop_ticks = hwp_timer->HWTimer_CurVal;
        sw->running = 0;
    }
}

UINT32 timer_stopwatch_get_ms(timer_stopwatch_t *sw)
{
    if (!sw) return 0;

    UINT32 current_ticks;
    if (sw->running)
    {
        current_ticks = hwp_timer->HWTimer_CurVal;
    }
    else
    {
        current_ticks = sw->stop_ticks;
    }

    UINT64 elapsed_ticks = current_ticks - sw->start_ticks;
    return (UINT32)((elapsed_ticks * 1000) / HW_TIMER_FREQ_HZ);
}

// =============================================================================
//  OS TIMER IRQ DRIVER (Periodic Hardware Interrupt Generator)
// =============================================================================

static volatile UINT32 g_timer_irq_count = 0;
static volatile UINT32 g_os_uptime_ms    = 0;
static volatile UINT32 g_timer_tick_hz   = 100;

void timer_os_start(UINT32 tick_hz)
{
    if (tick_hz == 0) tick_hz = 100;
    g_timer_tick_hz = tick_hz;

    // Calculate load value for 24-bit downcounter based on 16384 Hz clock
    UINT32 load_val = HW_TIMER_FREQ_HZ / tick_hz;
    if (load_val == 0) load_val = 1;

    // 1. Stop and clear OSTimer
    hwp_timer->OSTimer_Ctrl = 0;
    hwp_timer->Timer_Irq_Clr = TIMER_OSTIMER_CLR;

    // 2. Configure OSTimer:
    // - 24-bit downcounter (TIMER_LOADVAL)
    // - Load initial count immediately (TIMER_LOAD)
    // - Enable timer counting (TIMER_ENABLE)
    // - Wrap back to load_val when counter reaches zero (TIMER_WRAP)
    // When zero is reached, hardware generates OSTimer IRQ.
    hwp_timer->OSTimer_Ctrl = TIMER_ENABLE | TIMER_LOAD | TIMER_LOADVAL(load_val) | TIMER_WRAP | TIMER_REPEAT;

    // 3. Enable OSTimer hardware IRQ mask in timer block
    hwp_timer->Timer_Irq_Mask_Set = TIMER_OSTIMER_MASK;

    // 4. Register timer IRQ handler with OS vector table
    os_register_irq_handler(IRQ_INDEX_TIMER, timer_irq_handler);
}

void timer_os_stop(void)
{
    hwp_timer->OSTimer_Ctrl = 0;
    hwp_timer->Timer_Irq_Mask_Clr = TIMER_OSTIMER_MASK;
    hwp_timer->Timer_Irq_Clr = TIMER_OSTIMER_CLR;
}

void timer_irq_handler(uint32_t cause, uint32_t epc)
{
    (void)cause;
    (void)epc;
    g_timer_irq_count++;
    g_os_uptime_ms += (1000 / g_timer_tick_hz);

    // Clear OSTimer hardware IRQ flag
    hwp_timer->Timer_Irq_Clr = TIMER_OSTIMER_CLR;
}

UINT32 timer_get_irq_count(void)
{
    return g_timer_irq_count;
}

UINT32 timer_get_uptime_ms(void)
{
    return g_os_uptime_ms;
}

