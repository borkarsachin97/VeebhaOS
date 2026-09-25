/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * System IFC (DMA Controller) Register Definitions for RDA8809
 */

#ifndef _SYS_IFC_H_
#define _SYS_IFC_H_

#include "cs_types.h"
#include "global_macros.h"

#define REG_SYS_IFC_BASE            0x01A09000
#define SYS_IFC_STD_CHAN_NB         7

typedef volatile struct
{
    REG32                          get_ch;                       // 0x00000000
    REG32                          dma_status;                   // 0x00000004
    REG32                          debug_status;                 // 0x00000008
    REG32                          Reserved_0000000C;            // 0x0000000C
    struct
    {
        REG32                      control;                      // 0x00000010 + i*0x10
        REG32                      status;                       // 0x00000014 + i*0x10
        REG32                      start_addr;                   // 0x00000018 + i*0x10
        REG32                      tc;                           // 0x0000001C + i*0x10
    } std_ch[SYS_IFC_STD_CHAN_NB];
} HWP_SYS_IFC_T;

#define hwp_sysIfc                  ((HWP_SYS_IFC_T*) KSEG1(REG_SYS_IFC_BASE))

// get_ch
#define SYS_IFC_CH_TO_USE(n)        (((n) & 15) << 0)
#define SYS_IFC_CH_TO_USE_MASK      (15 << 0)

// dma_status
#define SYS_IFC_CH_ENABLE(n)        (((n) & 0xFF) << 0)
#define SYS_IFC_CH_BUSY(n)          (((n) & 0x7F) << 16)

// control
#define SYS_IFC_ENABLE              (1 << 0)
#define SYS_IFC_DISABLE             (1 << 1)
#define SYS_IFC_CH_RD_HW_EXCH       (1 << 2)
#define SYS_IFC_CH_WR_HW_EXCH       (1 << 3)
#define SYS_IFC_AUTODISABLE         (1 << 4)
#define SYS_IFC_SIZE                (1 << 5)
#define SYS_IFC_REQ_SRC(n)          (((n) & 31) << 8)

// SYS IFC Request Source IDs for RDA8809
#define SYS_IFC_REQ_SRC_TX_SDMMC    (14 << 8)
#define SYS_IFC_REQ_SRC_RX_SDMMC    (15 << 8)
#define SYS_IFC_FLUSH               (1 << 16)

// status
#define SYS_IFC_FIFO_EMPTY          (1 << 4)

#endif // _SYS_IFC_H_
