/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * OBTEL B10 (RDA8809) Global Hardware Register Definitions
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BOARDS_OBTEL_B10_GLOBAL_MACROS_H
#define BOARDS_OBTEL_B10_GLOBAL_MACROS_H

#include <stdint.h>

#define REG32(addr)                 (*(volatile uint32_t *)(addr))
#define REG16(addr)                 (*(volatile uint16_t *)(addr))
#define REG8(addr)                  (*(volatile uint8_t *)(addr))

/* Base Memory Mapping */
#define RDA_BASE_ISRAM              0x81C00000UL /* 64 KB Internal SRAM */
#define RDA_BASE_PSRAM              0x82000000UL /* 8 MB External PSRAM */
#define RDA_BASE_SPI_FLASH          0x88000000UL /* 4 MB SPI NOR Flash */

/* Peripheral Bases */
#define RDA_BASE_SYS_CTRL           0x01A00000UL
#define RDA_BASE_IRQ                0x01A02000UL
#define RDA_BASE_TIMER              0x01A03000UL
#define RDA_BASE_KEYPAD             0x01A08000UL
#define RDA_BASE_GPIO               0x01A0A000UL
#define RDA_BASE_LCD                0x01A18000UL
#define RDA_BASE_DMA                0x01A1C000UL
#define RDA_BASE_AUDIO              0x01A20000UL
#define RDA_BASE_PMU                0x01A24000UL

/* Bitfield manipulation helpers */
#define BIT(n)                      (1UL << (n))
#define GET_BIT(reg, bit)           (((reg) >> (bit)) & 1UL)
#define SET_BIT(reg, bit)           ((reg) |= (1UL << (bit)))
#define CLR_BIT(reg, bit)           ((reg) &= ~(1UL << (bit)))

#endif /* BOARDS_OBTEL_B10_GLOBAL_MACROS_H */
