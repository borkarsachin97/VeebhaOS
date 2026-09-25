/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * OBTEL B10 (RDA8809) System IRQ Definitions
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BOARDS_OBTEL_B10_SYS_IRQ_H
#define BOARDS_OBTEL_B10_SYS_IRQ_H

#include <stdint.h>

typedef enum {
    RDA_IRQ_TIMER1      = 0,
    RDA_IRQ_TIMER2      = 1,
    RDA_IRQ_KEYPAD      = 2,
    RDA_IRQ_UART1       = 3,
    RDA_IRQ_UART2       = 4,
    RDA_IRQ_LCD         = 5,
    RDA_IRQ_DMA         = 6,
    RDA_IRQ_GPIO        = 7,
    RDA_IRQ_AUDIO       = 8,
    RDA_IRQ_PMU         = 9,
    RDA_IRQ_USB         = 10,
    RDA_IRQ_SDMMC       = 11,
    RDA_IRQ_MAX         = 16
} rda_irq_line_t;

typedef void (*rda_irq_handler_t)(void *arg);

void rda_irq_enable(rda_irq_line_t irq);
void rda_irq_disable(rda_irq_line_t irq);
void rda_irq_register(rda_irq_line_t irq, rda_irq_handler_t handler, void *arg);
void rda_irq_clear(rda_irq_line_t irq);

#endif /* BOARDS_OBTEL_B10_SYS_IRQ_H */
