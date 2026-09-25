/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * IRQ subsystem helpers for RDA8809 bare-metal OS.
 */

#include "cs_types.h"
#include "global_macros.h"
#include "sys_ctrl.h"
#include "sys_irq.h"
#include "timer.h"
#include "keypad.h"
#include "irq.h"
#include "hal_aif.h"
#include "hal_uart.h"

static volatile UINT32 g_irq_count    = 0;
static volatile UINT32 g_last_irq_cause = 0;

void irq_c_handler(void)
{
    UINT32 cause = hwp_sysIrq->Cause;
    g_last_irq_cause = cause;
    g_irq_count++;

    if (cause & (SYS_IRQ_SYS_IRQ_TIMERS | SYS_IRQ_SYS_IRQ_OS_TIMER))
    {
        hwp_timer->Timer_Irq_Clr = TIMER_OSTIMER_CLR
                                  | TIMER_HWTIMER_WRAP_CLR
                                  | TIMER_HWTIMER_ITV_CLR;
    }

    if (cause & SYS_IRQ_SYS_IRQ_KEYPAD)
    {
        hwp_keypad->KP_IRQ_CLR = KEYPAD_KP_IRQ_CLR;
    }

    if (cause & (SYS_IRQ_SYS_IRQ_BBIFC1 | SYS_IRQ_SYS_IRQ_BBIFC0))
    {
        hal_AifDmaIrqHandler();
    }

    if (cause & SYS_IRQ_SYS_IRQ_UART)
    {
        hal_UartIrqHandler(HAL_UART_1);
    }
    if (cause & SYS_IRQ_SYS_IRQ_UART2)
    {
        hal_UartIrqHandler(HAL_UART_2);
    }

    hwp_sysIrq->Pulse_Clear = cause;
}

void irq_init(void)
{
    // Default safe state: masked and disabled
    irq_disable_all();
}

void irq_enable_all(void)
{
    // 1. Enable Keypad, OS Timer, Timers, and UART in RDA System IRQ controller
    UINT32 irq_mask = SYS_IRQ_SYS_IRQ_KEYPAD | SYS_IRQ_SYS_IRQ_OS_TIMER | SYS_IRQ_SYS_IRQ_TIMERS | SYS_IRQ_SYS_IRQ_UART;
    hwp_sysIrq->Mask_Set       = irq_mask;
    hwp_sysIrq->Pulse_Mask_Set = irq_mask;

    // 2. Clear BEV bit to 0 in CP0 Status so interrupts vector to RAM/OS vector
    cp0_clear_bev();

    // 3. Enable CPU Global Interrupt bit in CP0 Status
    cp0_set_status(cp0_get_status() | CP0_STATUS_IE | CP0_STATUS_IM_MASK);
}

void irq_disable_all(void)
{
    // 1. Disable CPU Global Interrupt bit
    cp0_set_status(cp0_get_status() & ~CP0_STATUS_IE);

    // 2. Mask all hardware IRQ sources
    hwp_sysIrq->Mask_Clear    = 0xFFFFFFFF;
    hwp_sysIrq->Pulse_Mask_Clr = 0xFFFFFFFF;
    hwp_sysIrq->Pulse_Clear   = 0xFFFFFFFF;
}

BOOL irq_is_enabled(void)
{
    return (cp0_get_status() & CP0_STATUS_IE) != 0;
}

void irq_enable_source(UINT32 mask)
{
    hwp_sysIrq->Mask_Set       = mask;
    hwp_sysIrq->Pulse_Mask_Set = mask;
}

void irq_disable_source(UINT32 mask)
{
    hwp_sysIrq->Mask_Clear    = mask;
    hwp_sysIrq->Pulse_Mask_Clr = mask;
}

UINT32 irq_get_count(void)      { return g_irq_count; }
UINT32 irq_get_last_cause(void) { return g_last_irq_cause; }

BOOL irq_is_bev_cleared(void)
{
    return (cp0_get_status() & CP0_STATUS_BEV) == 0;
}
