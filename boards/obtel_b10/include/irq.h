/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * OBTEL B10 (RDA8809) MIPS32 Coprocessor 0 Interrupt Primitives
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BOARDS_OBTEL_B10_IRQ_H
#define BOARDS_OBTEL_B10_IRQ_H

#include <stdint.h>

#ifdef __mips__
static inline uint32_t rda_disable_interrupts(void)
{
    uint32_t status;
    __asm__ volatile(
        "mfc0 %0, $12\n\t"
        "ins  %0, $0, 0, 1\n\t"
        "mtc0 %0, $12\n\t"
        "ehb\n\t"
        : "=r"(status)
        :
        : "memory"
    );
    return status;
}

static inline void rda_enable_interrupts(uint32_t status)
{
    __asm__ volatile(
        "mtc0 %0, $12\n\t"
        "ehb\n\t"
        :
        : "r"(status)
        : "memory"
    );
}
#else
static inline uint32_t rda_disable_interrupts(void) { return 0; }
static inline void rda_enable_interrupts(uint32_t status) { (void)status; }
#endif

#endif /* BOARDS_OBTEL_B10_IRQ_H */
