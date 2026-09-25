/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Smart Card Interface (SCI) HAL Interface for RDA8809
 */

#ifndef _HAL_SCI_H_
#define _HAL_SCI_H_

#include "cs_types.h"
#include "sci.h"

// Public SCI HAL API
void hal_SciInit(void);
int hal_SciOpen(UINT32 clkRate);
void hal_SciClose(void);
int hal_SciPutChar(UINT8 ch);
int hal_SciGetChar(UINT8 *ch);
BOOL hal_SciTxReady(void);
BOOL hal_SciRxReady(void);

#endif // _HAL_SCI_H_
