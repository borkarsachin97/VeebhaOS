/*
 * FreeRTOS Kernel V10.5.1
 *
 * MIPS32r1 Port Implementation for OBTEL B10 (RDA8809)
 *
 * SPDX-License-Identifier: MIT
 */

#include "FreeRTOS.h"
#include "task.h"
#include "boards/obtel_b10/include/global_macros.h"
#include "boards/obtel_b10/include/timer.h"
#include "boards/obtel_b10/include/sys_irq.h"

#define portINITIAL_SR          0x1000FF01UL /* Interrupts enabled, status register initial state */
#define portCONTEXT_SIZE        ( 32 * 4 )

static UBaseType_t uxCriticalNesting = 0xaaaaaaaa;

/*
 * Initialise the stack of a new task to look exactly as if a call to
 * portSAVE_CONTEXT had been called.
 */
StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                     StackType_t * pxEndOfStack,
                                     TaskFunction_t pxCode,
                                     void * pvParameters )
{
    ( void ) pxEndOfStack;

    /* Align stack to 8 bytes */
    pxTopOfStack = ( StackType_t * ) ( ( ( uint32_t ) pxTopOfStack ) & ~0x7UL );

    *pxTopOfStack = ( StackType_t ) 0xdeadbeef;
    pxTopOfStack--;

    *pxTopOfStack = ( StackType_t ) portINITIAL_SR;     /* CP0 Status */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) pxCode;             /* EPC */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x00000000;         /* HI */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x00000000;         /* LO */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x00000000;         /* $31 ($ra) */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x00000000;         /* $30 ($fp) */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) 0x00000000;         /* $28 ($gp) */
    pxTopOfStack--;

    /* General Purpose Registers $1 to $25 */
    for( int i = 25; i >= 1; i-- ) {
        if( i == 4 ) {
            *pxTopOfStack = ( StackType_t ) pvParameters; /* $4 ($a0) */
        } else {
            *pxTopOfStack = ( StackType_t ) ( 0x10000000UL | i );
        }
        pxTopOfStack--;
    }

    *pxTopOfStack = ( StackType_t ) 0; /* Critical Nesting counter */

    return pxTopOfStack;
}

BaseType_t xPortStartScheduler( void )
{
    uxCriticalNesting = 0;

    /* Initialize HW timer for system tick at configTICK_RATE_HZ */
    rda_timer_init( configTICK_RATE_HZ );
    rda_timer_start();

    /* Start the first task via assembly restore context */
#ifdef __mips__
    __asm__ volatile(
        "la    $k0, uxCriticalNesting\n\t"
        "sw    $zero, 0($k0)\n\t"
        "la    $k0, pxCurrentTCB\n\t"
        "lw    $k1, 0($k0)\n\t"
        "lw    $sp, 0($k1)\n\t"
        "j     vPortRestoreContextStub\n\t"
        "nop\n\t"
    );
#endif

    return pdFALSE;
}

void vPortEndScheduler( void )
{
    rda_timer_stop();
}

void vPortYield( void )
{
#ifdef __mips__
    __asm__ volatile( "syscall 0x1\n\tnop" ::: "memory" );
#endif
}

void vPortEnterCritical( void )
{
    portDISABLE_INTERRUPTS();
    uxCriticalNesting++;
}

void vPortExitCritical( void )
{
    if( uxCriticalNesting > 0 ) {
        uxCriticalNesting--;
        if( uxCriticalNesting == 0 ) {
            portENABLE_INTERRUPTS();
        }
    }
}

void vPortSystemTickHandler( void )
{
    if( xTaskIncrementTick() != pdFALSE ) {
        vTaskSwitchContext();
    }
}
