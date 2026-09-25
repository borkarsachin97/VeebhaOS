/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * System IRQ Hardware Register Definitions for RDA8809 / RDA8955 SoC
 */

#ifndef _SYS_IRQ_H_
#define _SYS_IRQ_H_

#include "cs_types.h"

#define REG_SYS_IRQ_BASE            0x01A01000

typedef volatile struct
{
    REG32                          Cause;                        //0x00000000
    REG32                          Status;                       //0x00000004
    REG32                          Mask_Set;                     //0x00000008
    REG32                          Mask_Clear;                   //0x0000000C
    REG32                          NonMaskable;                  //0x00000010
    REG32                          SC;                           //0x00000014
    REG32                          WakeUp_Mask;                  //0x00000018
    REG32                          Cpu_Sleep;                    //0x0000001C
    REG32                          Pulse_Mask_Set;               //0x00000020
    REG32                          Pulse_Mask_Clr;               //0x00000024
    REG32                          Pulse_Clear;                  //0x00000028
    REG32                          Pulse_Status;                 //0x0000002C
} HWP_SYS_IRQ_T;

#define hwp_sysIrq                  ((HWP_SYS_IRQ_T*) KSEG1(REG_SYS_IRQ_BASE))

// System IRQ Source Bit Masks
#define SYS_IRQ_SYS_IRQ_TCU0        (1<<0)
#define SYS_IRQ_SYS_IRQ_TCU1        (1<<1)
#define SYS_IRQ_SYS_IRQ_FRAME       (1<<2)
#define SYS_IRQ_SYS_IRQ_COM0        (1<<3)
#define SYS_IRQ_SYS_IRQ_COM1        (1<<4)
#define SYS_IRQ_SYS_IRQ_VOC         (1<<5)
#define SYS_IRQ_SYS_IRQ_DMA         (1<<6)
#define SYS_IRQ_SYS_IRQ_GPIO        (1<<7)
#define SYS_IRQ_SYS_IRQ_KEYPAD      (1<<8)
#define SYS_IRQ_SYS_IRQ_TIMERS      (1<<9)
#define SYS_IRQ_SYS_IRQ_OS_TIMER    (1<<10)
#define SYS_IRQ_SYS_IRQ_CALENDAR    (1<<11)
#define SYS_IRQ_SYS_IRQ_SPI1        (1<<12)
#define SYS_IRQ_SYS_IRQ_SPI2        (1<<13)
#define SYS_IRQ_SYS_IRQ_SPI3        (1<<14)
#define SYS_IRQ_SYS_IRQ_DEBUG_UART  (1<<15)
#define SYS_IRQ_SYS_IRQ_UART        (1<<16)
#define SYS_IRQ_SYS_IRQ_UART2       (1<<17)
#define SYS_IRQ_SYS_IRQ_I2C         (1<<18)
#define SYS_IRQ_SYS_IRQ_I2C2        (1<<19)
#define SYS_IRQ_SYS_IRQ_I2C3        (1<<20)
#define SYS_IRQ_SYS_IRQ_SCI         (1<<21)
#define SYS_IRQ_SYS_IRQ_RF_SPI      (1<<22)
#define SYS_IRQ_SYS_IRQ_LPS         (1<<23)
#define SYS_IRQ_SYS_IRQ_BBIFC0      (1<<24)
#define SYS_IRQ_SYS_IRQ_BBIFC1      (1<<25)
#define SYS_IRQ_SYS_IRQ_USBC        (1<<26)
#define SYS_IRQ_SYS_IRQ_GOUDA       (1<<27)
#define SYS_IRQ_SYS_IRQ_SDMMC       (1<<28)
#define SYS_IRQ_SYS_IRQ_CAMERA      (1<<29)
#define SYS_IRQ_SYS_IRQ_PMU         (1<<30)
#define SYS_IRQ_SYS_IRQ_SDMMC2      (1<<31)

#endif // _SYS_IRQ_H_
