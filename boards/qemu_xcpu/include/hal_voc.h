/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * VoC (Voice & Audio Coprocessor) HAL Interface for RDA8809
 */

#ifndef _HAL_VOC_H_
#define _HAL_VOC_H_

#include "cs_types.h"
#include "voc_ahb.h"
#include "voc_cfg.h"
#include "voc_ram.h"

typedef enum
{
    HAL_VOC_WAKEUP_EVENT_0 = 0,
    HAL_VOC_WAKEUP_EVENT_1 = 1,
    HAL_VOC_START          = 2
} HAL_VOC_WAKEUP_ID_T;

typedef enum
{
    HAL_VOC_DMA_READ  = 0,
    HAL_VOC_DMA_WRITE = 1
} HAL_VOC_DMA_DIR_T;

typedef enum
{
    HAL_VOC_DMA_BURST  = 0,
    HAL_VOC_DMA_SINGLE = 1
} HAL_VOC_DMA_TRANSFER_T;

typedef struct
{
    UINT32 wakeupIfc0:1;
    UINT32 wakeupIfc1:1;
    UINT32 wakeupDmae:1;
    UINT32 wakeupDmai:1;
    UINT32 wakeupSof0:1;
    UINT32 wakeupSof1:1;
} HAL_VOC_WAKEUP_EVENT_T;

typedef struct
{
    UINT32 voc:1;
    UINT32 unused1:1;
    UINT32 dmaVoc:1;
    UINT32 unused2:1;
} HAL_VOC_IRQ_STATUS_T;

typedef void (*HAL_VOC_IRQ_HANDLER_T)(HAL_VOC_IRQ_STATUS_T* status);

typedef struct
{
    const INT32            *vocCode;
    UINT32                 vocCodeSize;
    UINT16                 pcVal;
    UINT16                 pcValCriticalSecMin;
    UINT16                 pcValCriticalSecMax;
    BOOL                   needOpenDoneIrq;
    HAL_VOC_WAKEUP_EVENT_T eventMask;
    HAL_VOC_IRQ_STATUS_T   irqMask;
    HAL_VOC_IRQ_HANDLER_T  vocIrqHandler;
    BOOL                   enableFlashAccess;
} HAL_VOC_CFG_T;

typedef struct
{
    INT32             *extAddr;
    INT32             *vocLocalAddr;
    INT32              size;
    HAL_VOC_DMA_DIR_T  wr1Rd0;
    BOOL               needIrq;
} HAL_VOC_DMA_CFG_T;

// Public VoC HAL API
void hal_VocInit(void);
int hal_VocOpen(const HAL_VOC_CFG_T *pCfg);
void hal_VocClose(void);
int hal_VocWakeup(HAL_VOC_WAKEUP_ID_T wakeupId);
void* hal_VocGetPointer(INT32 vocLocalAddr);
INT32* hal_VocGetDmaiPointer(INT32 *vocExternAddr, HAL_VOC_DMA_DIR_T wr1Rd0, HAL_VOC_DMA_TRANSFER_T sngl1brst0);
int hal_VocDmaStart(const HAL_VOC_DMA_CFG_T *pCfg);
BOOL hal_VocDmaDone(void);
BOOL hal_VocStateActive(void);
void hal_VocIrqHandler(void);

#endif // _HAL_VOC_H_
