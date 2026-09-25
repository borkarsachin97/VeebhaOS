/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */

#ifndef SDK_INCLUDE_VEEBHA_IRQ_H
#define SDK_INCLUDE_VEEBHA_IRQ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * RDA8809 Interrupt Controller (INTC) Base Address & Register Offsets
 * Memory-mapped at 0x1A900000 on RDA Microelectronics baseband silicon.
 */
#define RDA8809_INTC_BASE          0x1A900000UL
#define RDA8809_INTC_STATUS_OFF    0x0000
#define RDA8809_INTC_MASK_SET_OFF  0x0004
#define RDA8809_INTC_MASK_CLR_OFF  0x0008
#define RDA8809_INTC_RAW_OFF       0x000C
#define RDA8809_INTC_CLEAR_OFF     0x0010

/**
 * Hardware Interrupt Vectors for Feature Phone Peripherals
 */
typedef enum {
    IRQ_NONE            = 0,
    IRQ_KEYPAD          = 1,
    IRQ_MODEM_RING      = 2,
    IRQ_MODEM_SMS       = 3,
    IRQ_SDMMC_DETECT    = 4,
    IRQ_BATTERY_GAUGE   = 5,
    IRQ_RTC_ALARM       = 6,
    IRQ_MAX_VECTORS     = 16
} os_irq_vector_t;

typedef void (*os_isr_handler_t)(void *arg);

/**
 * Initialize the hardware interrupt subsystem and vector table.
 */
void os_irq_init(void);

/**
 * Register a top-half interrupt service routine (ISR) for a vector.
 *
 * @param vector  Interrupt vector number.
 * @param handler Top-half ISR callback.
 * @param arg     Context parameter passed to ISR.
 * @return true on success, false if invalid vector.
 */
bool os_irq_register(os_irq_vector_t vector, os_isr_handler_t handler, void *arg);

/**
 * Enable/unmask an interrupt vector.
 */
void os_irq_enable(os_irq_vector_t vector);

/**
 * Disable/mask an interrupt vector.
 */
void os_irq_disable(os_irq_vector_t vector);

/**
 * Dispatch an interrupt (invokes registered top-half ISR).
 */
void os_irq_dispatch(os_irq_vector_t vector);

/**
 * Simulator mock IRQ triggers:
 * Triggers hardware interrupt bottom-half dispatch into the system event queue.
 */
void os_irq_sim_trigger_call(const char *number, const char *caller_name);
void os_irq_sim_trigger_sms(const char *sender, const char *preview);
void os_irq_sim_trigger_sdcard_toggle(void);

#ifdef __cplusplus
}
#endif

#endif /* SDK_INCLUDE_VEEBHA_IRQ_H */
