/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Master SPI (SPI 1, 2, 3) HAL Interface for RDA8809
 */

#ifndef _HAL_SPI_H_
#define _HAL_SPI_H_

#include "cs_types.h"
#include "spi.h"

typedef enum
{
    HAL_SPI_1 = 0,
    HAL_SPI_2 = 1,
    HAL_SPI_3 = 2,
    HAL_SPI_QTY = 3
} HAL_SPI_ID_T;

typedef enum
{
    HAL_SPI_CS0 = 0,
    HAL_SPI_CS1 = 1,
    HAL_SPI_CS2 = 2,
    HAL_SPI_CS3 = 3
} HAL_SPI_CS_T;

typedef struct
{
    BOOL   clkFallEdge;
    UINT8  clkDelay;
    UINT8  doDelay;
    UINT8  diDelay;
    UINT8  csDelay;
    UINT8  csPulse;
    UINT8  frameSize;
    UINT32 baudRate;
} HAL_SPI_CFG_T;

// Public Master SPI HAL API
void hal_SpiInit(void);
int hal_SpiOpen(HAL_SPI_ID_T spiId, HAL_SPI_CS_T cs, const HAL_SPI_CFG_T *cfg);
void hal_SpiClose(HAL_SPI_ID_T spiId, HAL_SPI_CS_T cs);
void hal_SpiActivateCs(HAL_SPI_ID_T spiId, HAL_SPI_CS_T cs);
void hal_SpiDeActivateCs(HAL_SPI_ID_T spiId, HAL_SPI_CS_T cs);
UINT32 hal_SpiSendData(HAL_SPI_ID_T spiId, HAL_SPI_CS_T cs, const UINT8 *startAddr, UINT32 length);
UINT32 hal_SpiGetData(HAL_SPI_ID_T spiId, HAL_SPI_CS_T cs, UINT8 *destAddr, UINT32 length);
UINT8 hal_SpiTxFifoAvail(HAL_SPI_ID_T spiId);
UINT8 hal_SpiRxFifoLevel(HAL_SPI_ID_T spiId);
BOOL hal_SpiBusy(HAL_SPI_ID_T spiId);

#endif // _HAL_SPI_H_
