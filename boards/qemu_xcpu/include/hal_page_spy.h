/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Page Spy HAL Interface for RDA8809
 */

#ifndef _HAL_PAGE_SPY_H_
#define _HAL_PAGE_SPY_H_

#include "cs_types.h"
#include "page_spy.h"

typedef enum
{
    HAL_PAGE_SPY_MODE_READ       = 1,
    HAL_PAGE_SPY_MODE_WRITE      = 2,
    HAL_PAGE_SPY_MODE_READ_WRITE = 3
} HAL_PAGE_SPY_MODE_T;

// Public Page Spy HAL API
void hal_PageSpyInit(void);
void hal_PageSpySetup(UINT8 pageNum, HAL_PAGE_SPY_MODE_T mode, UINT32 startAddr, UINT32 endAddr);
void hal_PageSpyEnable(UINT8 pageNum);
void hal_PageSpyDisable(UINT8 pageNum);
BOOL hal_PageSpyIsEnabled(UINT8 pageNum);
UINT8 hal_PageSpyGetHitMaster(UINT8 pageNum);
UINT32 hal_PageSpyGetHitAddress(UINT8 pageNum);
HAL_PAGE_SPY_MODE_T hal_PageSpyGetHitMode(UINT8 pageNum);
UINT32 hal_PageSpyGetStatus(void);

#endif // _HAL_PAGE_SPY_H_
