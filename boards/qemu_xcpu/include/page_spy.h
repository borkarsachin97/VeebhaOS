/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Page Spy Register Definitions for RDA8809
 */

#ifndef _PAGE_SPY_H_
#define _PAGE_SPY_H_

#include "cs_types.h"
#include "global_macros.h"

#define REG_PAGE_SPY_BASE           0x01A0C000
#define PAGE_SPY_NB_PAGE            16

typedef volatile struct
{
    REG32                          enable;                       //0x00000000
    REG32                          status;                       //0x00000004
    REG32                          disable;                      //0x00000008
    REG32                          Reserved_0000000C;            //0x0000000C
    struct
    {
        REG32                      start;                        //0x00000010 + i*0x10
        REG32                      end;                          //0x00000014 + i*0x10
        REG32                      master;                       //0x00000018 + i*0x10
        REG32                      addr;                         //0x0000001C + i*0x10
    } page[PAGE_SPY_NB_PAGE];
} HWP_PAGE_SPY_T;

#define hwp_pageSpy                 ((HWP_PAGE_SPY_T*) KSEG1(REG_PAGE_SPY_BASE))

#define PAGE_SPY_MODE(n)            (((n)&3)<<0)
#define PAGE_SPY_MODE_READ          (1<<0)
#define PAGE_SPY_MODE_WRITE         (2<<0)
#define PAGE_SPY_MODE_READ_WRITE    (3<<0)

#endif // _PAGE_SPY_H_
