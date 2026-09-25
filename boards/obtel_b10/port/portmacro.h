/*
 * FreeRTOS Kernel V10.5.1
 *
 * Port Macro for MIPS32r1 (OBTEL B10 / RDA8809)
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PORTMACRO_H
#define PORTMACRO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/* Type definitions */
#define portCHAR                char
#define portFLOAT               float
#define portDOUBLE              double
#define portLONG                long
#define portSHORT               short
#define portSTACK_TYPE          uint32_t
#define portBASE_TYPE           long
#define portPOINTER_SIZE_TYPE   uint32_t

typedef portSTACK_TYPE StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;

#if ( configUSE_16_BIT_TICKS == 1 )
    typedef uint16_t TickType_t;
    #define portMAX_DELAY ( TickType_t ) 0xffff
#else
    typedef uint32_t TickType_t;
    #define portMAX_DELAY ( TickType_t ) 0xffffffffUL
#endif

/* Architecture specifics */
#define portSTACK_GROWTH        ( -1 )
#define portTICK_PERIOD_MS      ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT      8
#define portNOP()               __asm__ volatile( "nop" )

/* Critical section management */
extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );
extern void vPortYield( void );

#define portDISABLE_INTERRUPTS()        __asm__ volatile( "di; ehb" ::: "memory" )
#define portENABLE_INTERRUPTS()         __asm__ volatile( "ei; ehb" ::: "memory" )
#define portENTER_CRITICAL()            vPortEnterCritical()
#define portEXIT_CRITICAL()             vPortExitCritical()
#define portYIELD()                     vPortYield()
#define portYIELD_FROM_ISR( xHigherPriorityTaskWoken ) if( ( xHigherPriorityTaskWoken ) != pdFALSE ) vPortYield()

/* Task function macros */
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters ) void vFunction( void *pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters ) void vFunction( void *pvParameters )

#ifdef __cplusplus
}
#endif

#endif /* PORTMACRO_H */
