/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * MIPS Exception & IRQ Management API Header
 */

#ifndef _IRQ_H_
#define _IRQ_H_

#include "cs_types.h"

// CP0 Status Register Bit Fields
#define CP0_STATUS_BEV       0x00400000  // Bootstrap Exception Vector (1=ROM/ISRAM)
#define CP0_STATUS_IE        0x00000001  // Interrupt Enable bit (1=Enabled, 0=Disabled)
#define CP0_STATUS_IM_MASK   0x0000FF00  // Interrupt Mask bits

typedef void (*irq_handler_t)(UINT32 cause);

// IRQ Subsystem API
void    irq_init(void);
void    irq_enable_source(UINT32 mask);
void    irq_disable_source(UINT32 mask);
void    irq_enable_all(void);
void    irq_disable_all(void);
BOOL    irq_is_enabled(void);
UINT32  irq_get_count(void);
UINT32  irq_get_last_cause(void);
BOOL    irq_is_bev_cleared(void);

// Assembly CP0 Helpers
UINT32  cp0_get_status(void);
void    cp0_set_status(UINT32 status);
UINT32  cp0_get_cause(void);
void    cp0_clear_bev(void);
void    cp0_set_bev(void);

#endif // _IRQ_H_
