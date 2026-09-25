/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Smart Card Interface (SCI) Register Definitions for RDA8809
 */

#ifndef _SCI_H_
#define _SCI_H_

#include "cs_types.h"
#include "global_macros.h"

#define REG_SCI_BASE                0x01A10000

typedef volatile struct
{
    REG32                          Config;                       //0x00000000
    REG32                          Status;                       //0x00000004
    REG32                          Data;                         //0x00000008
    REG32                          ClkDiv;                       //0x0000000C
    REG32                          RxCnt;                        //0x00000010
    REG32                          Times;                        //0x00000014
    REG32                          Ch_Filt;                      //0x00000018
    REG32                          dbg;                          //0x0000001C
    REG32                          Int_Cause;                    //0x00000020
    REG32                          Int_Clr;                      //0x00000024
    REG32                          Int_Mask;                     //0x00000028
} HWP_SCI_T;

#define hwp_sci                     ((HWP_SCI_T*) KSEG1(REG_SCI_BASE))

// Config
#define SCI_ENABLE                  (1<<0)
#define SCI_PARITY_EVEN_PARITY      (0<<1)
#define SCI_PARITY_ODD_PARITY       (1<<1)
#define SCI_PARITY_EN               (1<<9)
#define SCI_RESET                   (1<<20)

// Status
#define SCI_RXDATA_RDY              (1<<0)
#define SCI_TX_FIFO_RDY             (1<<1)
#define SCI_CLK_RDY_H               (1<<5)
#define SCI_CLK_OFF                 (1<<6)

// ClkDiv
#define SCI_CLKDIV(n)               (((n)&0x1FF)<<0)
#define SCI_MAINDIV(n)              (((n)&0x3F)<<24)

#endif // _SCI_H_
