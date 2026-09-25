/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * System IFC (DMA Controller) HAL Interface for RDA8809
 */

#ifndef _HAL_SYS_IFC_H_
#define _HAL_SYS_IFC_H_

#include "cs_types.h"
#include "sys_ifc.h"

#define HAL_UNKNOWN_CHANNEL         0xFF

typedef enum
{
    HAL_IFC_SCI_TX              = 0,
    HAL_IFC_SCI_RX              = 1,
    HAL_IFC_SPI_TX              = 2,
    HAL_IFC_SPI_RX              = 3,
    HAL_IFC_SPI2_TX             = 4,
    HAL_IFC_SPI2_RX             = 5,
    HAL_IFC_SPI3_TX             = 6,
    HAL_IFC_SPI3_RX             = 7,
    HAL_IFC_DEBUG_UART_TX       = 8,
    HAL_IFC_DEBUG_UART_RX       = 9,
    HAL_IFC_UART1_TX            = 10,
    HAL_IFC_UART1_RX            = 11,
    HAL_IFC_UART2_TX            = 12,
    HAL_IFC_UART2_RX            = 13,
    HAL_IFC_SDMMC_TX            = 14,
    HAL_IFC_SDMMC_RX            = 15,
    HAL_IFC_CAMERA_RX           = 17,
    HAL_IFC_SDMMC2_TX           = 18,
    HAL_IFC_SDMMC2_RX           = 19,
    HAL_IFC_NO_REQWEST          = 31
} HAL_IFC_REQUEST_ID_T;

typedef enum
{
    HAL_IFC_SIZE_8_MODE_MANUAL  = 0,
    HAL_IFC_SIZE_8_MODE_AUTO    = SYS_IFC_AUTODISABLE,
    HAL_IFC_SIZE_32_MODE_MANUAL = SYS_IFC_SIZE,
    HAL_IFC_SIZE_32_MODE_AUTO   = (SYS_IFC_SIZE | SYS_IFC_AUTODISABLE)
} HAL_IFC_MODE_T;

// Public IFC HAL API
void hal_IfcInit(void);
UINT8 hal_IfcTransferStart(HAL_IFC_REQUEST_ID_T requestId, UINT8* memStartAddr, UINT32 xferSize, HAL_IFC_MODE_T ifcMode);
void hal_IfcChannelRelease(HAL_IFC_REQUEST_ID_T requestId, UINT8 channel);
void hal_IfcChannelFlush(HAL_IFC_REQUEST_ID_T requestId, UINT8 channel);
UINT32 hal_IfcGetTc(HAL_IFC_REQUEST_ID_T requestId, UINT8 channel);
BOOL hal_IfcChannelIsFifoEmpty(HAL_IFC_REQUEST_ID_T requestId, UINT8 channel);
HAL_IFC_REQUEST_ID_T hal_IfcGetOwner(UINT8 channel);

#endif // _HAL_SYS_IFC_H_
