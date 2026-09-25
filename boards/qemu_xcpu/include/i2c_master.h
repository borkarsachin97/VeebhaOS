/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * I2C Master Register Definitions for RDA8809
 */

#ifndef _I2C_MASTER_H_
#define _I2C_MASTER_H_

#include "cs_types.h"
#include "global_macros.h"

#define REG_I2C_MASTER_BASE         0x01A07000
#define REG_I2C_MASTER2_BASE        0x01A22000
#define REG_I2C_MASTER3_BASE        0x01A23000

typedef volatile struct
{
    REG32                          CTRL;                         // 0x00000000
    REG32                          STATUS;                       // 0x00000004
    REG32                          TXRX_BUFFER;                  // 0x00000008
    REG32                          CMD;                          // 0x0000000C
    REG32                          IRQ_CLR;                      // 0x00000010
} HWP_I2C_MASTER_T;

#define hwp_i2cMaster               ((HWP_I2C_MASTER_T*) KSEG1(REG_I2C_MASTER_BASE))
#define hwp_i2cMaster2              ((HWP_I2C_MASTER_T*) KSEG1(REG_I2C_MASTER2_BASE))
#define hwp_i2cMaster3              ((HWP_I2C_MASTER_T*) KSEG1(REG_I2C_MASTER3_BASE))

// CTRL
#define I2C_MASTER_EN               (1 << 0)
#define I2C_MASTER_IRQ_MASK         (1 << 8)
#define I2C_MASTER_CLOCK_PRESCALE(n) (((n) & 0xFFFF) << 16)
#define I2C_MASTER_CLOCK_PRESCALE_MASK (0xFFFF << 16)

// STATUS
#define I2C_MASTER_IRQ_CAUSE        (1 << 0)
#define I2C_MASTER_IRQ_STATUS       (1 << 4)
#define I2C_MASTER_TIP              (1 << 8)
#define I2C_MASTER_AL               (1 << 12)
#define I2C_MASTER_BUSY             (1 << 16)
#define I2C_MASTER_RXACK            (1 << 20)

// TXRX_BUFFER
#define I2C_MASTER_TX_DATA(n)       (((n) & 0xFF) << 0)
#define I2C_MASTER_RX_DATA(n)       (((n) & 0xFF) << 0)

// CMD
#define I2C_MASTER_ACK              (1 << 0)
#define I2C_MASTER_RD               (1 << 4)
#define I2C_MASTER_STO              (1 << 8)
#define I2C_MASTER_WR               (1 << 12)
#define I2C_MASTER_STA              (1 << 16)

// IRQ_CLR
#define I2C_MASTER_IRQ_CLR          (1 << 0)

#endif // _I2C_MASTER_H_
