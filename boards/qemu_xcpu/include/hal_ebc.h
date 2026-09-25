/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * External Bus Controller (EBC) HAL Interface for RDA8809
 */

#ifndef _HAL_EBC_H_
#define _HAL_EBC_H_

#include "cs_types.h"
#include "mem_bridge.h"

typedef enum
{
    HAL_EBC_CS0 = 0,
    HAL_EBC_CS1 = 1,
    HAL_EBC_CS2 = 2,
    HAL_EBC_CS3 = 3,
    HAL_EBC_CS4 = 4
} HAL_EBC_CS_T;

// Public EBC HAL API
void hal_EbcInit(void);
int hal_EbcCsOpen(HAL_EBC_CS_T cs, UINT32 csMode, UINT32 csTime);
void hal_EbcCsClose(HAL_EBC_CS_T cs);
UINT32 hal_EbcGetStatus(void);

#endif // _HAL_EBC_H_
