/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * OBTEL B10 (RDA8809) Hardware Timer Driver Interface
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BOARDS_OBTEL_B10_TIMER_H
#define BOARDS_OBTEL_B10_TIMER_H

#include <stdint.h>
#include <stdbool.h>
#include "global_macros.h"

/* RDA8809 Hardware Timer Registers */
#define RDA_TIMER1_LOAD             REG32(RDA_BASE_TIMER + 0x00)
#define RDA_TIMER1_VALUE            REG32(RDA_BASE_TIMER + 0x04)
#define RDA_TIMER1_CTRL             REG32(RDA_BASE_TIMER + 0x08)
#define RDA_TIMER1_INT_CLR          REG32(RDA_BASE_TIMER + 0x0C)

#define RDA_TIMER_CTRL_ENABLE       BIT(0)
#define RDA_TIMER_CTRL_MODE_PERIOD  BIT(1)
#define RDA_TIMER_CTRL_INT_ENABLE   BIT(2)

void rda_timer_init(uint32_t freq_hz);
void rda_timer_start(void);
void rda_timer_stop(void);
uint32_t rda_timer_get_ticks(void);

#endif /* BOARDS_OBTEL_B10_TIMER_H */
