/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * BaseBand IFC (DMA Controller) Register Definitions for RDA8809
 * Handles AIF Audio Streaming DMA (Channel 0: Record, Channel 1: Play)
 */

#ifndef _BB_IFC_H_
#define _BB_IFC_H_

#include "cs_types.h"
#include "global_macros.h"

#define REG_BB_IFC_BASE             0x01921000

// Compatibility alias for user prompt
#define AIF_DMA_BASE                REG_BB_IFC_BASE

typedef volatile struct
{
    struct
    {
        REG32                      control;                      // 0x00000000 + i*0x20
        REG32                      status;                       // 0x00000004 + i*0x20
        REG32                      start_addr;                   // 0x00000008 + i*0x20
        REG32                      Fifo_Size;                    // 0x0000000C + i*0x20
        REG32                      Reserved_00000010;            // 0x00000010 + i*0x20
        REG32                      int_mask;                     // 0x00000014 + i*0x20
        REG32                      int_clear;                    // 0x00000018 + i*0x20
        REG32                      cur_ahb_addr;                 // 0x0000001C + i*0x20
    } ch[2];
    REG32                          ch2_control;                  // 0x00000040
    REG32                          ch2_status;                   // 0x00000044
    REG32                          ch2_start_addr;               // 0x00000048
    REG32                          ch2_end_addr;                 // 0x0000004C
    REG32                          ch2_tc;                       // 0x00000050
    REG32                          ch2_int_mask;                 // 0x00000054
    REG32                          ch2_int_clear;                // 0x00000058
    REG32                          ch2_cur_ahb_addr;             // 0x0000005C
    REG32                          ch3_control;                  // 0x00000060
    REG32                          ch3_status;                   // 0x00000064
    REG32                          ch3_start_addr;               // 0x00000068
    REG32                          Reserved_0000006C;            // 0x0000006C
    REG32                          ch3_tc;                       // 0x00000070
    REG32                          ch3_int_mask;                 // 0x00000074
    REG32                          ch3_int_clear;                // 0x00000078
    REG32                          ch3_cur_ahb_addr;             // 0x0000007C
} HWP_BB_IFC_T;

#define hwp_bbIfc                   ((HWP_BB_IFC_T*) KSEG1(REG_BB_IFC_BASE))

// control
#define BB_IFC_ENABLE               (1<<0)
#define BB_IFC_DISABLE              (1<<1)
#define BB_IFC_AUTO_DISABLE         (1<<4)

// status
#define BB_IFC_FIFO_EMPTY           (1<<4)
#define BB_IFC_CAUSE_IEF            (1<<8)
#define BB_IFC_CAUSE_IHF            (1<<9)
#define BB_IFC_CAUSE_I4F            (1<<10)
#define BB_IFC_CAUSE_I3_4F          (1<<11)
#define BB_IFC_IEF                  (1<<16)
#define BB_IFC_IHF                  (1<<17)
#define BB_IFC_I4F                  (1<<18)
#define BB_IFC_I3_4F                (1<<19)

// int_mask & int_clear
#define BB_IFC_END_FIFO             (1<<8)
#define BB_IFC_HALF_FIFO            (1<<9)
#define BB_IFC_QUARTER_FIFO         (1<<10)
#define BB_IFC_THREE_QUARTER_FIFO   (1<<11)

// Channels
#define BB_IFC_CH_RECORD            0
#define BB_IFC_CH_PLAY              1

#endif // _BB_IFC_H_
