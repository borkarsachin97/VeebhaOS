/*
 * FreeRTOS Kernel Port for RDA8809 (MIPS XCPU)
 */

#include "FreeRTOS.h"
#include "task.h"
#include "global_macros.h"
#include "sys_irq.h"
#include "timer.h"
#include "irq.h"

/* Critical section nesting counter */
volatile uint32_t uxCriticalNesting = 0;

/* Interrupt nesting counter */
volatile uint32_t uxInterruptNesting = 0;

/* Global flag indicating scheduler is actively running */
volatile uint32_t g_freertos_scheduler_started = 0;

/* Context switch pending flag set from ISRs */
volatile BaseType_t xYieldPendingFromISR = pdFALSE;

/* Assembly helpers defined in isram_stub.S */
extern void vPortStartFirstTask( void );
extern void cp0_clear_bev( void );

static void prvTaskExitError( void )
{
    portDISABLE_INTERRUPTS();
    for( ;; );
}

/*
 * Stack frame initialization for a new task (32 words = 128 bytes):
 *
 * Offset 0x00..0x6C: GPRs ($at, $v0-$v1, $a0-$a3, $t0-$t9, $s0-$s7, $gp, $fp, $ra)
 * Offset 0x70: LO
 * Offset 0x74: HI
 * Offset 0x78: CP0 Cause
 * Offset 0x7C: CP0 EPC (task entry point)
 */
StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters )
{
    /* Maintain 8-byte stack alignment */
    pxTopOfStack = (StackType_t *)( ( (uint32_t)pxTopOfStack ) & ~0x7UL );

    /* Reserve 32 words (128 bytes) for initial context frame */
    pxTopOfStack -= 32;

    /* Zero out all registers */
    for (int i = 0; i < 32; i++) {
        pxTopOfStack[i] = 0;
    }

    register uint32_t current_fp __asm__("$30");

    pxTopOfStack[3]  = (uint32_t)pvParameters;     /* $a0 (offset 0x0C: task parameter) */
    pxTopOfStack[24] = (uint32_t)pxCode;           /* $t9 (offset 0x60: entry function in MIPS ABI) */
    pxTopOfStack[25] = 0;                          /* $gp (offset 0x64: global pointer = 0 in -G0) */
    pxTopOfStack[26] = current_fp;                 /* $fp (offset 0x68: frame pointer) */
    pxTopOfStack[27] = (uint32_t)prvTaskExitError; /* $ra (offset 0x6C: return address) */
    pxTopOfStack[30] = (uint32_t)0x0000FF01;       /* CP0 Status (offset 0x78: IE=1, IM=0xFF) */
    pxTopOfStack[31] = (uint32_t)pxCode;           /* CP0 EPC (offset 0x7C: initial PC) */

    return pxTopOfStack;
}

BaseType_t xPortStartScheduler( void )
{
    /* Reset critical section nesting */
    uxCriticalNesting = 0;

    /* Start hardware 24-bit down-counter OS Timer at FreeRTOS tick rate */
    timer_os_start( configTICK_RATE_HZ );

    /* Unmask hardware peripheral interrupts in RDA System IRQ controller */
    UINT32 irq_mask = SYS_IRQ_SYS_IRQ_KEYPAD | SYS_IRQ_SYS_IRQ_OS_TIMER | SYS_IRQ_SYS_IRQ_TIMERS | SYS_IRQ_SYS_IRQ_UART;
    hwp_sysIrq->Mask_Set       = irq_mask;
    hwp_sysIrq->Pulse_Mask_Set = irq_mask;
    hwp_sysIrq->SC             = 1;
    cp0_clear_bev();

    /* Launch first task from pxCurrentTCB.
     * vPortStartFirstTask will set g_freertos_scheduler_started = 1 and
     * enable CPU global interrupts (CP0 Status = 0xFF01) atomically upon jumping
     * to the first task.
     */
    vPortStartFirstTask();
    /* Should never reach here */
    prvTaskExitError();
    return pdFALSE;
}

void vPortEndScheduler( void )
{
    g_freertos_scheduler_started = 0;
    timer_os_stop();
    irq_disable_all();
}

void vPortEnterCritical( void )
{
    portDISABLE_INTERRUPTS();
    uxCriticalNesting++;
    __asm__ volatile( "" ::: "memory" );
}

void vPortExitCritical( void )
{
    if (uxCriticalNesting > 0)
    {
        uxCriticalNesting--;
        if (uxCriticalNesting == 0)
        {
            portENABLE_INTERRUPTS();
        }
    }
}

void vAssertCalled( const char *pcFile, unsigned long ulLine )
{
    (void)pcFile;
    (void)ulLine;
    portDISABLE_INTERRUPTS();
    while ( 1 )
    {
        portNOP();
    }
}

