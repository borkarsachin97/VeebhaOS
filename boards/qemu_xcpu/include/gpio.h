/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * GPIO Hardware Register Definitions for RDA8809
 */

#ifndef _GPIO_H_
#define _GPIO_H_

#include "cs_types.h"
#include "global_macros.h"

#define REG_GPIO_BASE               0x01A03000

typedef volatile struct
{
    REG32                          gpio_oen_val;                 //0x00000000
    REG32                          gpio_oen_set_out;             //0x00000004
    REG32                          gpio_oen_set_in;              //0x00000008
    REG32                          gpio_val;                     //0x0000000C
    REG32                          gpio_set;                     //0x00000010
    REG32                          gpio_clr;                     //0x00000014
    REG32                          gpint_ctrl_set;               //0x00000018
    REG32                          gpint_ctrl_clr;               //0x0000001C
    REG32                          int_clr;                      //0x00000020
    REG32                          int_status;                   //0x00000024
    REG32                          chg_ctrl;                     //0x00000028
    REG32                          chg_cmd;                      //0x0000002C
    REG32                          gpo_set;                      //0x00000030
    REG32                          gpo_clr;                      //0x00000034
} HWP_GPIO_T;

#define hwp_gpio                    ((HWP_GPIO_T*) KSEG1(REG_GPIO_BASE))

#define GPIO_PIN_MASK(pin)          (1 << (pin))

#endif // _GPIO_H_
