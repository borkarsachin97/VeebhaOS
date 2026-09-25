/*
 * FreeRTOS Kernel Port for RDA8809 (MIPS XCPU)
 */

#ifndef PORTMACRO_H
#define PORTMACRO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Type definitions */
#define portCHAR        char
#define portFLOAT       float
#define portDOUBLE      double
#define portLONG        long
#define portSHORT       short
#define portSTACK_TYPE  uint32_t
#define portBASE_TYPE   long

typedef portSTACK_TYPE StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;

#if( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_16_BITS )
    typedef uint16_t TickType_t;
    #define portMAX_DELAY ( TickType_t ) 0xffff
#elif ( configTICK_TYPE_WIDTH_IN_BITS == TICK_TYPE_WIDTH_32_BITS )
    typedef uint32_t TickType_t;
    #define portMAX_DELAY ( TickType_t ) 0xffffffffUL
    #define portTICK_TYPE_IS_ATOMIC 1
#else
    #error configTICK_TYPE_WIDTH_IN_BITS set to unsupported tick type width.
#endif

/* Architecture specifics */
#define portBYTE_ALIGNMENT          8
#define portSTACK_GROWTH            -1
#define portTICK_PERIOD_MS          ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portNOP()                   __asm__ volatile ( "nop" )

/* Critical section management */
extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );
extern void vPortYield( void );

#define portENTER_CRITICAL()        vPortEnterCritical()
#define portEXIT_CRITICAL()         vPortExitCritical()

/* Interrupt control */
static inline uint32_t port_get_cp0_status( void )
{
    uint32_t status;
    __asm__ volatile ( "mfc0 %0, $12" : "=r" (status) );
    return status;
}

static inline void port_set_cp0_status( uint32_t status )
{
    __asm__ volatile (
        "mtc0 %0, $12\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        : : "r" (status) : "memory"
    );
}

#define portDISABLE_INTERRUPTS()    port_set_cp0_status( port_get_cp0_status() & ~0x00000001 )
#define portENABLE_INTERRUPTS()     port_set_cp0_status( port_get_cp0_status() |  0x00000001 )

static inline UBaseType_t uxPortSetInterruptMaskFromISR( void )
{
    uint32_t status = port_get_cp0_status();
    port_set_cp0_status( status & ~0x00000001 );
    return (UBaseType_t)status;
}

static inline void vPortClearInterruptMaskFromISR( UBaseType_t uxSavedStatus )
{
    port_set_cp0_status( (uint32_t)uxSavedStatus );
}

#define portSET_INTERRUPT_MASK_FROM_ISR()           uxPortSetInterruptMaskFromISR()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR( val )    vPortClearInterruptMaskFromISR( val )

/* Task utilities */
extern volatile BaseType_t xYieldPendingFromISR;
extern volatile uint32_t uxInterruptNesting;
extern volatile uint32_t uxCriticalNesting;
#define portYIELD()                                 vPortYield()
#define portYIELD_FROM_ISR( x )                     do { if( ( x ) != pdFALSE ) { xYieldPendingFromISR = pdTRUE; } } while( 0 )
#define portEND_SWITCHING_ISR( x )                  portYIELD_FROM_ISR( x )

/* Task function macros */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters ) void vFunction( void *pvParameters ) __attribute__((noreturn))
#define portTASK_FUNCTION( vFunction, pvParameters ) void vFunction( void *pvParameters )

#ifdef __cplusplus
}
#endif

#endif /* PORTMACRO_H */
