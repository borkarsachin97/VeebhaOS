/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * OS Vector Table & ISRAM Jump Table Subsystem for RDA8809
 */

#include "cs_types.h"
#include "global_macros.h"
#include "sys_ctrl.h"
#include "sys_irq.h"
#include "timer.h"
#include "keypad.h"
#include "irq.h"
#include "os_vector_table.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hal_aif.h"
#include "hal_gouda.h"
#include "hal_uart.h"

// Jump table pointer maps directly onto ISRAM at 0x81C00380
os_jump_table_t * const g_isram_jump_table = (os_jump_table_t *)ISRAM_JUMP_TABLE_ADDR;

// Internal static jump table in BSS
static os_irq_handler_t g_irq_handlers[MAX_IRQ_HANDLERS];

static volatile uint32_t g_irq_count  = 0;
static volatile uint32_t g_last_cause = 0;
static volatile uint32_t g_last_epc   = 0;

void reboot(void)
{
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_UNLOCK;
    hwp_sysCtrl->Sys_Rst_Set = SYS_CTRL_SOFT_RST;
}

static void default_dummy_handler(uint32_t cause, uint32_t epc);

// ============================================================================
// os_master_c_dispatcher (Fixed Address in RAM: 0x82000300)
// ============================================================================
void __attribute__((section(".fixed_irq_dispatcher"), noinline)) os_master_c_dispatcher(uint32_t cause_cp0, uint32_t epc)
{

    uint32_t hw_cause = hwp_sysIrq->Cause;
    if (hw_cause == 0)
    {
        hw_cause = (hwp_sysIrq->Status & hwp_sysIrq->Mask_Set) |
                   (hwp_sysIrq->Pulse_Status & hwp_sysIrq->Pulse_Mask_Set);
    }

    g_last_cause = hw_cause ? hw_cause : cause_cp0;
    g_last_epc   = epc;
    g_irq_count++;

    // -------------------------------------------------------------------------
    // 1. OS Timer & Hardware Timers IRQ (FreeRTOS Periodic Tick)
    // -------------------------------------------------------------------------
    if (g_last_cause & (SYS_IRQ_SYS_IRQ_TIMERS | SYS_IRQ_SYS_IRQ_OS_TIMER))
    {
        // Acknowledge / clear hardware OSTimer IRQ
        hwp_timer->Timer_Irq_Clr = TIMER_OSTIMER_CLR
                                  | TIMER_HWTIMER_WRAP_CLR
                                  | TIMER_HWTIMER_ITV_CLR;
        volatile uint32_t dummy = hwp_timer->Timer_Irq_Clr;
        (void)dummy;

        // Process FreeRTOS OS Tick and preempt if higher priority task is ready
        if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        {
            if (xTaskIncrementTick() != pdFALSE)
            {
                vTaskSwitchContext();
            }
        }

        if (g_irq_handlers[IRQ_INDEX_TIMER])
        {
            g_irq_handlers[IRQ_INDEX_TIMER](g_last_cause, epc);
        }
    }

    // -------------------------------------------------------------------------
    // 2. Keypad IRQ
    // -------------------------------------------------------------------------
    if (g_last_cause & SYS_IRQ_SYS_IRQ_KEYPAD)
    {
        if (g_irq_handlers[IRQ_INDEX_KEYPAD])
        {
            g_irq_handlers[IRQ_INDEX_KEYPAD](g_last_cause, epc);
        }

        // Clear all keypad IRQ cause bits
        hwp_keypad->KP_IRQ_CLR = 0xFFFFFFFF;
    }

    // -------------------------------------------------------------------------
    // 3. USB IRQ
    // -------------------------------------------------------------------------
    if (g_last_cause & SYS_IRQ_SYS_IRQ_USBC)
    {
        if (g_irq_handlers[IRQ_INDEX_USB] && g_irq_handlers[IRQ_INDEX_USB] != default_dummy_handler)
        {
            g_irq_handlers[IRQ_INDEX_USB](g_last_cause, epc);
        }
        else
        {
            // USB driver not registered or inactive: immediately mask USBC IRQ to prevent an infinite storm!
            hwp_sysIrq->Mask_Clear = SYS_IRQ_SYS_IRQ_USBC;
            hwp_sysIrq->Pulse_Clear = SYS_IRQ_SYS_IRQ_USBC;
        }
    }

    // -------------------------------------------------------------------------
    // 4. BaseBand IFC (Audio DMA) IRQ
    // -------------------------------------------------------------------------
    if (g_last_cause & (SYS_IRQ_SYS_IRQ_BBIFC1 | SYS_IRQ_SYS_IRQ_BBIFC0))
    {
        if (g_irq_handlers[IRQ_INDEX_BBIFC] && g_irq_handlers[IRQ_INDEX_BBIFC] != default_dummy_handler)
        {
            g_irq_handlers[IRQ_INDEX_BBIFC](g_last_cause, epc);
        }
        else
        {
            hal_AifDmaIrqHandler();
        }
    }

    // -------------------------------------------------------------------------
    // 5. GOUDA EOF IRQ (2D Blitter DMA completion)
    // -------------------------------------------------------------------------
    if (g_last_cause & SYS_IRQ_SYS_IRQ_GOUDA)
    {
        if (g_irq_handlers[IRQ_INDEX_GOUDA] && g_irq_handlers[IRQ_INDEX_GOUDA] != default_dummy_handler)
        {
            g_irq_handlers[IRQ_INDEX_GOUDA](g_last_cause, epc);
        }
        else
        {
            hal_GoudaIrqHandler(g_last_cause, epc);
        }
    }

    // -------------------------------------------------------------------------
    // 6. PMU IRQ (Power Key / Charger)
    // -------------------------------------------------------------------------
    if (g_last_cause & SYS_IRQ_SYS_IRQ_PMU)
    {
        // Mask PMU IRQ to prevent storm; hal_lps will re-arm when entering sleep
        hwp_sysIrq->Mask_Clear     = SYS_IRQ_SYS_IRQ_PMU;
        hwp_sysIrq->Pulse_Mask_Clr = SYS_IRQ_SYS_IRQ_PMU;
    }

    // -------------------------------------------------------------------------
    // 7. UART1 / UART2 IRQ (Bluetooth / Host Serial)
    // -------------------------------------------------------------------------
    if (g_last_cause & SYS_IRQ_SYS_IRQ_UART)
    {
        hal_UartIrqHandler(HAL_UART_1);
    }
    if (g_last_cause & SYS_IRQ_SYS_IRQ_UART2)
    {
        hal_UartIrqHandler(HAL_UART_2);
    }

    // -------------------------------------------------------------------------
    // 8. Mask unexpected hardware interrupt sources
    // -------------------------------------------------------------------------
    uint32_t known_irqs = SYS_IRQ_SYS_IRQ_KEYPAD | SYS_IRQ_SYS_IRQ_TIMERS |
                          SYS_IRQ_SYS_IRQ_OS_TIMER | SYS_IRQ_SYS_IRQ_USBC |
                          SYS_IRQ_SYS_IRQ_BBIFC1 | SYS_IRQ_SYS_IRQ_BBIFC0 |
                          SYS_IRQ_SYS_IRQ_GOUDA | SYS_IRQ_SYS_IRQ_PMU |
                          SYS_IRQ_SYS_IRQ_GPIO | SYS_IRQ_SYS_IRQ_UART |
                          SYS_IRQ_SYS_IRQ_UART2;
    uint32_t unknown_irqs = g_last_cause & ~known_irqs;
    if (unknown_irqs != 0)
    {
        hwp_sysIrq->Mask_Clear     = unknown_irqs;
        hwp_sysIrq->Pulse_Mask_Clr = unknown_irqs;
    }

    // Clear all pending pulse interrupts
    hwp_sysIrq->Pulse_Clear = 0xFFFFFFFF;

    // -------------------------------------------------------------------------
    // 5. Switch context if requested from any ISR
    // -------------------------------------------------------------------------
    if (xYieldPendingFromISR != pdFALSE)
    {
        xYieldPendingFromISR = pdFALSE;
        if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        {
            vTaskSwitchContext();
        }
    }
}

static void default_dummy_handler(uint32_t cause, uint32_t epc)
{
    (void)cause;
    (void)epc;
}

// ============================================================================
// os_exception_c_handler
// ============================================================================
void os_exception_c_handler(uint32_t exc_code, uint32_t cause, uint32_t epc, os_exception_frame_t *frame)
{
    g_last_cause = cause;
    g_last_epc   = epc;
    (void)exc_code;
    (void)frame;
}

// ============================================================================
// os_vector_system_init
// ============================================================================
void os_vector_system_init(void)
{
    const uint32_t *src = (const uint32_t *)isram_vector_stub_start;
    uint32_t stub_size = (uint32_t)(isram_vector_stub_end - isram_vector_stub_start);
    if (stub_size > 2048) stub_size = 2048;
    uint32_t stub_words = (stub_size + 3) / 4;

    // 1. Copy ISRAM Vector Stub to 0x81C00280 (cached) & 0xA1C00280 (uncached)
    volatile uint32_t *isram_cached280   = (volatile uint32_t *)0x81C00280;
    volatile uint32_t *isram_uncached280 = (volatile uint32_t *)0xA1C00280;
    for (uint32_t i = 0; i < stub_words; i++) {
        isram_cached280[i]   = src[i];
        isram_uncached280[i] = src[i];
    }

    extern void mips_cache_flush(void);
    mips_cache_flush();

    // 2. Initialize ISRAM metadata and dedicated IRQ stack
    volatile uint32_t *isram_meta_unc = (volatile uint32_t *)0xA1C000C0;
    volatile uint32_t *isram_meta_cac = (volatile uint32_t *)0x81C000C0;

    isram_meta_unc[0] = 0;          // [0xC0] nesting lock = 0
    isram_meta_cac[0] = 0;
    isram_meta_unc[1] = 0x81C0FFF8; // [0xC4] IRQ stack top (8-byte aligned)
    isram_meta_cac[1] = 0x81C0FFF8;
    mips_cache_flush();

    for (int i = 0; i < MAX_IRQ_HANDLERS; i++) {
        g_irq_handlers[i] = default_dummy_handler;
        g_isram_jump_table->handlers[i] = default_dummy_handler;
    }
    g_irq_handlers[IRQ_INDEX_MASTER_DISPATCHER] = os_master_c_dispatcher;
    g_isram_jump_table->handlers[IRQ_INDEX_MASTER_DISPATCHER] = os_master_c_dispatcher;

    hwp_sysIrq->Mask_Clear  = 0xFFFFFFFF;
    hwp_sysIrq->Pulse_Clear = 0xFFFFFFFF;
    hwp_sysIrq->SC          = 1;

    // Clear BEV in CP0 Status so interrupts vector to RAM/ISRAM (0x81C00280)
    cp0_clear_bev();
}

void os_register_irq_handler(irq_index_t irq, os_irq_handler_t handler)
{
    if ((uint32_t)irq < MAX_IRQ_HANDLERS && handler != NULL) {
        g_irq_handlers[irq] = handler;
        g_isram_jump_table->handlers[irq] = handler;
    }
}

uint32_t os_get_irq_count(void)  { return g_irq_count;  }
uint32_t os_get_last_cause(void) { return g_last_cause; }
uint32_t os_get_last_epc(void)   { return g_last_epc;   }

BOOL os_is_bev_cleared(void)
{
    return (cp0_get_status() & CP0_STATUS_BEV) == 0;
}
