/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Hardware register definitions for RDA8809 UART1 (0x01A15000) and UART2 (0x01A16000)
 */

#ifndef _UART_H_
#define _UART_H_

#include "cs_types.h"
#include "global_macros.h"

#define REG_UART_BASE                       0x01A15000
#define REG_UART2_BASE                      0x01A16000

#define UART_RX_FIFO_SIZE                   32
#define UART_TX_FIFO_SIZE                   16

typedef volatile struct
{
    REG32 ctrl;                             // 0x00000000
    REG32 status;                           // 0x00000004
    REG32 rxtx_buffer;                      // 0x00000008
    REG32 irq_mask;                         // 0x0000000C
    REG32 irq_cause;                        // 0x00000010
    REG32 triggers;                         // 0x00000014
    REG32 CMD_Set;                          // 0x00000018
    REG32 CMD_Clr;                          // 0x0000001C
} HWP_UART_T;

#define hwp_uart1                           ((HWP_UART_T*) KSEG1(REG_UART_BASE))
#define hwp_uart2                           ((HWP_UART_T*) KSEG1(REG_UART2_BASE))

// ctrl Register Bitfields
#define UART_ENABLE                         (1 << 0)
#define UART_DATA_BITS_7_BITS               (0 << 1)
#define UART_DATA_BITS_8_BITS               (1 << 1)
#define UART_TX_STOP_BITS_1_BIT             (0 << 2)
#define UART_TX_STOP_BITS_2_BITS            (1 << 2)
#define UART_PARITY_ENABLE_NO               (0 << 3)
#define UART_PARITY_ENABLE_YES              (1 << 3)
#define UART_PARITY_SELECT_ODD              (0 << 4)
#define UART_PARITY_SELECT_EVEN             (1 << 4)
#define UART_PARITY_SELECT_SPACE            (2 << 4)
#define UART_PARITY_SELECT_MARK             (3 << 4)
#define UART_DIVISOR_MODE                   (1 << 20)
#define UART_IRDA_ENABLE                    (1 << 21)
#define UART_DMA_MODE_ENABLE                (1 << 22)
#define UART_AUTO_FLOW_CONTROL_ENABLE       (1 << 23)
#define UART_AUTO_FLOW_CONTROL_DISABLE      (0 << 23)
#define UART_LOOP_BACK_MODE                 (1 << 24)
#define UART_RX_BREAK_LENGTH(n)             (((n) & 15) << 28)

// status Register Bitfields
#define UART_RX_FIFO_LEVEL(n)               (((n) & 0x3F) << 0)
#define UART_RX_FIFO_LEVEL_MASK             (0x3F << 0)
#define UART_RX_FIFO_LEVEL_SHIFT            0
#define UART_TX_FIFO_SPACE(n)               (((n) & 31) << 8)
#define UART_TX_FIFO_SPACE_MASK             (31 << 8)
#define UART_TX_FIFO_SPACE_SHIFT            8
#define UART_TX_ACTIVE                      (1 << 14)
#define UART_RX_ACTIVE                      (1 << 15)
#define UART_RX_OVERFLOW_ERR                (1 << 16)
#define UART_TX_OVERFLOW_ERR                (1 << 17)
#define UART_RX_PARITY_ERR                  (1 << 18)
#define UART_RX_FRAMING_ERR                 (1 << 19)
#define UART_RX_BREAK_INT                   (1 << 20)
#define UART_CTS                            (1 << 25)

// irq_mask / irq_cause Bitfields
#define UART_TX_MODEM_STATUS                (1 << 0)
#define UART_RX_DATA_AVAILABLE              (1 << 1)
#define UART_TX_DATA_NEEDED                 (1 << 2)
#define UART_RX_TIMEOUT                     (1 << 3)
#define UART_RX_LINE_ERR                    (1 << 4)
#define UART_TX_DMA_DONE                    (1 << 5)
#define UART_RX_DMA_DONE                    (1 << 6)
#define UART_RX_DMA_TIMEOUT                 (1 << 7)

// triggers Register Bitfields
#define UART_RX_TRIGGER(n)                  (((n) & 31) << 0)
#define UART_TX_TRIGGER(n)                  (((n) & 15) << 8)
#define UART_AFC_LEVEL(n)                   (((n) & 31) << 16)

// CMD_Set / CMD_Clr Register Bitfields
#define UART_RI                             (1 << 0)
#define UART_DCD                            (1 << 1)
#define UART_DSR                            (1 << 2)
#define UART_TX_BREAK_CONTROL               (1 << 3)
#define UART_TX_FINISH_N_WAIT               (1 << 4)
#define UART_RTS                            (1 << 5)
#define UART_RX_FIFO_RESET                  (1 << 6)
#define UART_TX_FIFO_RESET                  (1 << 7)

#endif // _UART_H_
