/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * UART HAL interface for RDA8809
 */

#ifndef _HAL_UART_H_
#define _HAL_UART_H_

#include "cs_types.h"
#include "uart.h"

typedef enum
{
    HAL_UART_1 = 1,
    HAL_UART_2 = 2
} HAL_UART_ID_T;

typedef enum
{
    HAL_UART_BAUD_9600      = 9600,
    HAL_UART_BAUD_19200     = 19200,
    HAL_UART_BAUD_38400     = 38400,
    HAL_UART_BAUD_57600     = 57600,
    HAL_UART_BAUD_115200    = 115200,
    HAL_UART_BAUD_230400    = 230400,
    HAL_UART_BAUD_460800    = 460800,
    HAL_UART_BAUD_921600    = 921600
} HAL_UART_BAUD_RATE_T;

typedef enum
{
    HAL_UART_7_DATA_BITS = 7,
    HAL_UART_8_DATA_BITS = 8
} HAL_UART_DATA_BITS_T;

typedef enum
{
    HAL_UART_1_STOP_BIT  = 1,
    HAL_UART_2_STOP_BITS = 2
} HAL_UART_STOP_BITS_T;

typedef enum
{
    HAL_UART_NO_PARITY   = 0,
    HAL_UART_ODD_PARITY  = 1,
    HAL_UART_EVEN_PARITY = 2
} HAL_UART_PARITY_T;

typedef struct
{
    HAL_UART_BAUD_RATE_T rate;
    HAL_UART_DATA_BITS_T data;
    HAL_UART_STOP_BITS_T stop;
    HAL_UART_PARITY_T    parity;
    UINT8                rx_trigger;        // RX FIFO threshold (0..31)
    UINT8                tx_trigger;        // TX FIFO threshold (0..15)
    BOOL                 auto_flow_ctrl;    // Hardware RTS/CTS flow control
} HAL_UART_CFG_T;

typedef void (*HAL_UART_IRQ_HANDLER_T)(HAL_UART_ID_T id, UINT32 status);

// Public UART HAL Functions
void hal_UartOpen(HAL_UART_ID_T id, const HAL_UART_CFG_T *cfg);
void hal_UartClose(HAL_UART_ID_T id);

void hal_UartSetBaudRate(HAL_UART_ID_T id, HAL_UART_BAUD_RATE_T baud);
void hal_UartSetRts(HAL_UART_ID_T id, BOOL ready);

void hal_UartPutChar(HAL_UART_ID_T id, UINT8 c);
UINT8 hal_UartGetChar(HAL_UART_ID_T id);

UINT32 hal_UartWrite(HAL_UART_ID_T id, const UINT8 *data, UINT32 length);
UINT32 hal_UartRead(HAL_UART_ID_T id, UINT8 *buffer, UINT32 maxLength, UINT32 timeout_ms);

BOOL hal_UartTxFinished(HAL_UART_ID_T id);
BOOL hal_UartRxAvailable(HAL_UART_ID_T id);
UINT8 hal_UartGetRxCount(HAL_UART_ID_T id);
UINT8 hal_UartGetTxFreeSpace(HAL_UART_ID_T id);

void hal_UartFifoFlush(HAL_UART_ID_T id);

void hal_UartIrqSetHandler(HAL_UART_ID_T id, HAL_UART_IRQ_HANDLER_T handler);
void hal_UartIrqSetMask(HAL_UART_ID_T id, UINT32 mask);
void hal_UartIrqHandler(HAL_UART_ID_T id);

#endif // _HAL_UART_H_
