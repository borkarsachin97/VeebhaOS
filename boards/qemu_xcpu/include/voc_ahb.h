/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * VoC AHB Registers for RDA8809
 */

#ifndef _VOC_AHB_H_
#define _VOC_AHB_H_

#include "cs_types.h"
#include "global_macros.h"

#define REG_VOC_AHB_BASE            0x01970000

typedef volatile struct
{
    REG32                          Irq_Status;                   //0x00000000
    REG32                          Irq_Mask_Set;                 //0x00000004
    REG32                          Irq_Mask_Clr;                 //0x00000008
    REG32                          DMA_Cfg;                      //0x0000000C
    REG32                          DMA_LAddr;                    //0x00000010
    REG32                          DMA_EAddr;                    //0x00000014
    REG32                          DMA_Status;                   //0x00000018
    REG32                          DMA_Sema;                     //0x0000001C
    REG32                          DMA_2D;                       //0x00000020
    REG32                          DMA_Stop;                     //0x00000024
} HWP_VOC_AHB_T;

#define hwp_vocAhb                  ((HWP_VOC_AHB_T*) KSEG1(REG_VOC_AHB_BASE))

// Irq_Status
#define VOC_AHB_XCPU_VOC_IRQ_CAUSE  (1<<0)
#define VOC_AHB_XCPU_DMA_IRQ_CAUSE  (1<<1)
#define VOC_AHB_XCPU_DMAVOC_IRQ_CAUSE (1<<2)
#define VOC_AHB_XCPU_DEBUG_IRQ_CAUSE (1<<3)
#define VOC_AHB_XCPU_VOC_IRQ_STATUS (1<<16)
#define VOC_AHB_XCPU_DMA_IRQ_STATUS (1<<17)
#define VOC_AHB_XCPU_DMAVOC_IRQ_STATUS (1<<18)
#define VOC_AHB_XCPU_DEBUG_IRQ_STATUS (1<<19)

#define VOC_AHB_XCPU_IRQ_CAUSE(n)   (((n)&15)<<0)
#define VOC_AHB_XCPU_IRQ_CAUSE_MASK (15<<0)
#define VOC_AHB_XCPU_IRQ_STATUS(n)  (((n)&15)<<16)
#define VOC_AHB_XCPU_IRQ_STATUS_MASK (15<<16)

// Irq_Mask_Set
#define VOC_AHB_XCPU_VOC_IRQ_MASK   (1<<0)
#define VOC_AHB_XCPU_DMA_IRQ_MASK   (1<<1)
#define VOC_AHB_XCPU_DMAVOC_IRQ_MASK (1<<2)
#define VOC_AHB_XCPU_DEBUG_IRQ_MASK (1<<3)
#define VOC_AHB_XCPU_IRQ_MASK(n)    (((n)&15)<<0)
#define VOC_AHB_XCPU_IRQ_MASK_MASK  (15<<0)

// DMA_Cfg
#define VOC_AHB_DMA_RUN             (1<<16)
#define VOC_AHB_DMA_DIR_READ        (0<<20)
#define VOC_AHB_DMA_DIR_WRITE       (1<<20)
#define VOC_AHB_DMA_SIZE(n)         (((n)&0xFFFF)<<0)
#define VOC_AHB_DMA_SIZE_MASK       (0xFFFF<<0)

// DMA_LAddr
#define VOC_AHB_DMA_LADDR(n)        (((n)&0xFFFF)<<1)
#define VOC_AHB_DMA_LADDR_MASK      (0xFFFF<<1)

// DMA_EAddr
#define VOC_AHB_DMA_EADDR(n)        (((n)&0x3FFFFFF)<<2)
#define VOC_AHB_DMA_EADDR_MASK      (0x3FFFFFF<<2)

// DMA_Status
#define VOC_AHB_DMA_ON              (1<<0)
#define VOC_AHB_DMA_SEMA_STATUS     (1<<31)

#endif // _VOC_AHB_H_
