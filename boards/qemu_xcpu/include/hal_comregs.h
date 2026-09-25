/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Communication Registers (comregs) HAL Interface for RDA8809
 */

#ifndef _HAL_COMREGS_H_
#define _HAL_COMREGS_H_

#include "cs_types.h"
#include "comregs.h"

typedef enum
{
    HAL_COMREGS_XCPU_IRQ_SLOT_0 = 0,
    HAL_COMREGS_XCPU_IRQ_SLOT_1 = 1,
    HAL_COMREGS_XCPU_IRQ_SLOT_QTY
} HAL_COMREGS_IRQ_SLOT_T;

typedef void (*HAL_COMREGS_IRQ_HANDLER_T)(HAL_COMREGS_IRQ_SLOT_T slot, UINT8 cause);

// Public Comregs HAL API
void hal_ComregsInit(void);

// Software Event / IRQ generation & clearing
void hal_ComregsSetIrq(HAL_COMREGS_IRQ_SLOT_T slot, UINT8 mask);
void hal_ComregsClrIrq(HAL_COMREGS_IRQ_SLOT_T slot, UINT8 mask);
UINT32 hal_ComregsGetCause(void);

// Mask control
void hal_ComregsSetMask(HAL_COMREGS_IRQ_SLOT_T slot, UINT8 mask);
void hal_ComregsClrMask(HAL_COMREGS_IRQ_SLOT_T slot, UINT8 mask);

// Snapshot Registers
UINT32 hal_ComregsGetSnap(void);
void hal_ComregsSetSnapCfg(BOOL wrap3);

// IRQ Registration
void hal_ComregsIrqSetHandler(HAL_COMREGS_IRQ_SLOT_T slot, HAL_COMREGS_IRQ_HANDLER_T handler);
void hal_ComregsIrqHandler(void);

#endif // _HAL_COMREGS_H_
