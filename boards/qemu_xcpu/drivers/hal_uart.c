/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * UART HAL Driver Implementation for RDA8809
 * Supports UART1 (0x01A15000) and UART2 (0x01A16000)
 */

#include "cs_types.h"
#include "global_macros.h"
#include "uart.h"
#include "hal_uart.h"
#include "sys_ctrl.h"
#include "sys_irq.h"
#include "cfg_regs.h"
#include "timer.h"
#include "FreeRTOS.h"
#include "task.h"

#define UART_RX_RING_SIZE 2048

static HWP_UART_T * const g_uartHwp[2] = { hwp_uart1, hwp_uart2 };
static HAL_UART_IRQ_HANDLER_T g_uartIrqHandlers[2] = { NULL, NULL };
static uint8_t s_uart_rx_ring[2][UART_RX_RING_SIZE];
static volatile uint16_t s_uart_ring_head[2] = {0, 0};
static volatile uint16_t s_uart_ring_tail[2] = {0, 0};

// =============================================================================
// hal_UartSetRts - Set or clear RTS signal (host ready to receive)
// =============================================================================
void hal_UartSetRts(HAL_UART_ID_T id, BOOL ready)
{
    if (id != HAL_UART_1 && id != HAL_UART_2) return;

    HWP_UART_T *hwp = g_uartHwp[id - 1];
    if (ready)
    {
        hwp->CMD_Set = UART_RTS;
    }
    else
    {
        hwp->CMD_Clr = UART_RTS;
    }
}

// =============================================================================
// hal_UartSetBaudRate - Calculate clock divider and update sysCtrl
// =============================================================================
void hal_UartSetBaudRate(HAL_UART_ID_T id, HAL_UART_BAUD_RATE_T baud)
{
    if (id != HAL_UART_1 && id != HAL_UART_2) return;

    UINT32 mode = 4;
    HWP_UART_T *hwp = g_uartHwp[id - 1];

    if (baud <= 4800)
    {
        mode = 16;
        hwp->ctrl |= UART_DIVISOR_MODE;
    }
    else
    {
        mode = 4;
        hwp->ctrl &= ~UART_DIVISOR_MODE;
    }

    // Formula for RDA8809 with 26 MHz Slow Clock:
    // divider = ((26000000 + (mode/2)*baud) / (mode*baud)) - 2;
    UINT32 fsSys = 26000000;
    UINT32 divider = ((fsSys + ((mode / 2) * (UINT32)baud)) / (mode * (UINT32)baud)) - 2;

    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_UNLOCK;
    if (id < 3)
    {
        hwp_sysCtrl->Cfg_Clk_Uart[id] = SYS_CTRL_UART_DIVIDER(divider) | SYS_CTRL_UART_SEL_PLL_SLOW;
    }
    if (id > 0 && (id - 1) < 3)
    {
        hwp_sysCtrl->Cfg_Clk_Uart[id - 1] = SYS_CTRL_UART_DIVIDER(divider) | SYS_CTRL_UART_SEL_PLL_SLOW;
    }
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_LOCK;
}

// =============================================================================
// hal_UartOpen - Configure and enable UART peripheral
// =============================================================================
void hal_UartOpen(HAL_UART_ID_T id, const HAL_UART_CFG_T *cfg)
{
    if (id != HAL_UART_1 && id != HAL_UART_2) return;

    HWP_UART_T *hwp = g_uartHwp[id - 1];

    // 1. Enable peripheral clock and reset
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_UNLOCK;
    if (id == HAL_UART_1)
    {
        hwp_sysCtrl->Clk_Per_Enable |= SYS_CTRL_ENABLE_PER_UART;
        hwp_sysCtrl->Clk_Per_Mode   &= ~SYS_CTRL_MODE_PER_UART_MANUAL;
        hwp_sysCtrl->Sys_Rst_Clr     = SYS_CTRL_CLR_RST_UART;
        // Configure GPIO Mode for UART1 CTS/RTS to ALT (not GPIO)
        hwp_configRegs->GPIO_Mode   &= ~((1 << 20) | (1 << 21));
    }
    else
    {
        hwp_sysCtrl->Clk_Per_Enable |= SYS_CTRL_ENABLE_PER_UART2;
        hwp_sysCtrl->Clk_Per_Mode   &= ~SYS_CTRL_MODE_PER_UART2_MANUAL;
        hwp_sysCtrl->Sys_Rst_Clr     = SYS_CTRL_CLR_RST_UART2;
        // Configure GPIO Mode for UART2 CTS/RTS/RXD/TXD to ALT (not GPIO)
        hwp_configRegs->GPIO_Mode   &= ~((1 << 5) | (1 << 7) | (1 << 12) | (1 << 13));
    }
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_LOCK;

    // 2. Clear control and reset FIFOs
    hwp->CMD_Clr = UART_RI | UART_DCD | UART_DSR | UART_TX_BREAK_CONTROL |
                   UART_TX_FINISH_N_WAIT | UART_RTS;
    hwp->CMD_Set = UART_RX_FIFO_RESET | UART_TX_FIFO_RESET;

    // 3. Configure Baud Rate
    HAL_UART_BAUD_RATE_T baud = (cfg && cfg->rate > 0) ? cfg->rate : HAL_UART_BAUD_115200;
    hal_UartSetBaudRate(id, baud);

    // 4. Build Control Register
    UINT32 ctrl = UART_ENABLE | UART_RX_BREAK_LENGTH(13);

    if (cfg)
    {
        if (cfg->data == HAL_UART_7_DATA_BITS)
            ctrl |= UART_DATA_BITS_7_BITS;
        else
            ctrl |= UART_DATA_BITS_8_BITS;

        if (cfg->stop == HAL_UART_2_STOP_BITS)
            ctrl |= UART_TX_STOP_BITS_2_BITS;
        else
            ctrl |= UART_TX_STOP_BITS_1_BIT;

        if (cfg->parity == HAL_UART_ODD_PARITY)
            ctrl |= UART_PARITY_ENABLE_YES | UART_PARITY_SELECT_ODD;
        else if (cfg->parity == HAL_UART_EVEN_PARITY)
            ctrl |= UART_PARITY_ENABLE_YES | UART_PARITY_SELECT_EVEN;
        else
            ctrl |= UART_PARITY_ENABLE_NO;

        if (cfg->auto_flow_ctrl)
            ctrl |= UART_AUTO_FLOW_CONTROL_ENABLE;
        else
            ctrl |= UART_AUTO_FLOW_CONTROL_DISABLE;

        hwp->triggers = UART_RX_TRIGGER(cfg->rx_trigger & 31) | UART_TX_TRIGGER(cfg->tx_trigger & 15);
    }
    else
    {
        ctrl |= UART_DATA_BITS_8_BITS | UART_TX_STOP_BITS_1_BIT | UART_PARITY_ENABLE_NO | UART_AUTO_FLOW_CONTROL_DISABLE;
        hwp->triggers = UART_RX_TRIGGER(1) | UART_TX_TRIGGER(0);
    }

    hwp->irq_mask = UART_RX_DATA_AVAILABLE | UART_RX_TIMEOUT;
    hwp->ctrl = ctrl;

    // Assert RTS by default so remote device knows host is ready
    hwp->CMD_Set = UART_RTS;

    // Unmask hardware interrupt in system IRQ controller
    UINT32 sys_irq = (id == HAL_UART_1) ? SYS_IRQ_SYS_IRQ_UART : SYS_IRQ_SYS_IRQ_UART2;
    hwp_sysIrq->Mask_Set       = sys_irq;
    hwp_sysIrq->Pulse_Mask_Set = sys_irq;
}

// =============================================================================
// hal_UartClose - Disable peripheral and cut clock
// =============================================================================
void hal_UartClose(HAL_UART_ID_T id)
{
    if (id != HAL_UART_1 && id != HAL_UART_2) return;

    HWP_UART_T *hwp = g_uartHwp[id - 1];
    hwp->ctrl &= ~UART_ENABLE;
    hwp->irq_mask = 0;

    UINT32 sys_irq = (id == HAL_UART_1) ? SYS_IRQ_SYS_IRQ_UART : SYS_IRQ_SYS_IRQ_UART2;
    hwp_sysIrq->Mask_Clear     = sys_irq;
    hwp_sysIrq->Pulse_Mask_Clr = sys_irq;

    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_UNLOCK;
    if (id == HAL_UART_1)
    {
        hwp_sysCtrl->Clk_Per_Disable = SYS_CTRL_DISABLE_PER_UART;
    }
    else
    {
        hwp_sysCtrl->Clk_Per_Disable = SYS_CTRL_DISABLE_PER_UART2;
    }
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_LOCK;
}

// =============================================================================
// hal_UartPutChar - Send a single byte (blocking if FIFO full)
// =============================================================================
void hal_UartPutChar(HAL_UART_ID_T id, UINT8 c)
{
    if (id != HAL_UART_1 && id != HAL_UART_2) return;

    HWP_UART_T *hwp = g_uartHwp[id - 1];
    uint32_t wait_timeout = 200000;
    while (((hwp->status & UART_TX_FIFO_SPACE_MASK) == 0) && (--wait_timeout > 0));

    if (wait_timeout > 0)
    {
        hwp->rxtx_buffer = c;
    }
}

// =============================================================================
// hal_UartGetChar - Read a single byte (blocking if FIFO empty)
// =============================================================================
UINT8 hal_UartGetChar(HAL_UART_ID_T id)
{
    UINT8 c = 0;
    if (hal_UartRead(id, &c, 1, 200) > 0)
    {
        return c;
    }
    return 0;
}

// =============================================================================
// hal_UartWrite - Transmit buffer
// =============================================================================
UINT32 hal_UartWrite(HAL_UART_ID_T id, const UINT8 *data, UINT32 length)
{
    if (!data || length == 0 || (id != HAL_UART_1 && id != HAL_UART_2)) return 0;

    HWP_UART_T *hwp = g_uartHwp[id - 1];
    for (UINT32 i = 0; i < length; i++)
    {
        uint32_t wait_timeout = 200000;
        while (((hwp->status & UART_TX_FIFO_SPACE_MASK) == 0) && (--wait_timeout > 0));
        if (wait_timeout == 0)
        {
            return i; // Timeout protection: prevent system freeze if TX FIFO is full
        }
        hwp->rxtx_buffer = data[i];
    }
    return length;
}

// =============================================================================
// hal_UartRead - Read into buffer with millisecond timeout (from ring buffer + HW FIFO)
// =============================================================================
UINT32 hal_UartRead(HAL_UART_ID_T id, UINT8 *buffer, UINT32 maxLength, UINT32 timeout_ms)
{
    if (!buffer || maxLength == 0 || (id != HAL_UART_1 && id != HAL_UART_2)) return 0;

    int idx = id - 1;
    HWP_UART_T *hwp = g_uartHwp[idx];
    UINT32 readCount = 0;
    UINT32 start_ms = timer_get_ms();

    while (readCount < maxLength)
    {
        // 1. Drain available bytes from software ring buffer
        while (readCount < maxLength && s_uart_ring_head[idx] != s_uart_ring_tail[idx])
        {
            buffer[readCount++] = s_uart_rx_ring[idx][s_uart_ring_tail[idx]];
            s_uart_ring_tail[idx] = (s_uart_ring_tail[idx] + 1) & (UART_RX_RING_SIZE - 1);
        }

        if (readCount >= maxLength) break;

        // 2. If ring buffer is empty, check and drain hardware FIFO under critical section
        if (s_uart_ring_head[idx] == s_uart_ring_tail[idx])
        {
            taskENTER_CRITICAL();
            while ((hwp->status & UART_RX_FIFO_LEVEL_MASK) > 0)
            {
                uint8_t byte = (uint8_t)(hwp->rxtx_buffer & 0xFF);
                uint16_t next_head = (s_uart_ring_head[idx] + 1) & (UART_RX_RING_SIZE - 1);
                if (next_head != s_uart_ring_tail[idx])
                {
                    s_uart_rx_ring[idx][s_uart_ring_head[idx]] = byte;
                    s_uart_ring_head[idx] = next_head;
                }
                else
                {
                    break; // Ring buffer full
                }
            }
            taskEXIT_CRITICAL();

            while (readCount < maxLength && s_uart_ring_head[idx] != s_uart_ring_tail[idx])
            {
                buffer[readCount++] = s_uart_rx_ring[idx][s_uart_ring_tail[idx]];
                s_uart_ring_tail[idx] = (s_uart_ring_tail[idx] + 1) & (UART_RX_RING_SIZE - 1);
            }
        }

        if (readCount >= maxLength) break;
        if (timeout_ms == 0 || (timer_get_ms() - start_ms) >= timeout_ms) break;
        timer_delay_ms(1);
    }
    return readCount;
}

// =============================================================================
// Status Queries
// =============================================================================
BOOL hal_UartTxFinished(HAL_UART_ID_T id)
{
    if (id != HAL_UART_1 && id != HAL_UART_2) return TRUE;
    HWP_UART_T *hwp = g_uartHwp[id - 1];
    return !(hwp->status & UART_TX_ACTIVE);
}

BOOL hal_UartRxAvailable(HAL_UART_ID_T id)
{
    if (id != HAL_UART_1 && id != HAL_UART_2) return FALSE;
    int idx = id - 1;
    if (s_uart_ring_head[idx] != s_uart_ring_tail[idx]) return TRUE;
    HWP_UART_T *hwp = g_uartHwp[idx];
    return ((hwp->status & UART_RX_FIFO_LEVEL_MASK) > 0);
}

UINT8 hal_UartGetRxCount(HAL_UART_ID_T id)
{
    if (id != HAL_UART_1 && id != HAL_UART_2) return 0;
    int idx = id - 1;
    uint16_t head = s_uart_ring_head[idx];
    uint16_t tail = s_uart_ring_tail[idx];
    uint16_t count = (head >= tail) ? (head - tail) : (UART_RX_RING_SIZE - tail + head);
    HWP_UART_T *hwp = g_uartHwp[idx];
    count += (hwp->status & UART_RX_FIFO_LEVEL_MASK) >> UART_RX_FIFO_LEVEL_SHIFT;
    return (count > 255) ? 255 : (UINT8)count;
}

UINT8 hal_UartGetTxFreeSpace(HAL_UART_ID_T id)
{
    if (id != HAL_UART_1 && id != HAL_UART_2) return 0;
    return (UINT8)((g_uartHwp[id - 1]->status & UART_TX_FIFO_SPACE_MASK) >> UART_TX_FIFO_SPACE_SHIFT);
}

void hal_UartFifoFlush(HAL_UART_ID_T id)
{
    if (id != HAL_UART_1 && id != HAL_UART_2) return;
    int idx = id - 1;
    HWP_UART_T *hwp = g_uartHwp[idx];
    taskENTER_CRITICAL();
    hwp->CMD_Set = UART_RX_FIFO_RESET | UART_TX_FIFO_RESET;
    s_uart_ring_head[idx] = 0;
    s_uart_ring_tail[idx] = 0;
    taskEXIT_CRITICAL();
}

// =============================================================================
// Interrupt Handling
// =============================================================================
void hal_UartIrqSetHandler(HAL_UART_ID_T id, HAL_UART_IRQ_HANDLER_T handler)
{
    if (id == HAL_UART_1 || id == HAL_UART_2)
    {
        g_uartIrqHandlers[id - 1] = handler;
    }
}

void hal_UartIrqSetMask(HAL_UART_ID_T id, UINT32 mask)
{
    if (id == HAL_UART_1 || id == HAL_UART_2)
    {
        g_uartHwp[id - 1]->irq_mask = mask;
    }
}

void hal_UartIrqHandler(HAL_UART_ID_T id)
{
    if (id != HAL_UART_1 && id != HAL_UART_2) return;

    int idx = id - 1;
    HWP_UART_T *hwp = g_uartHwp[idx];
    UINT32 cause = hwp->irq_cause;

    // Fast drain all hardware FIFO bytes into software ring buffer
    while ((hwp->status & UART_RX_FIFO_LEVEL_MASK) > 0)
    {
        uint8_t byte = (uint8_t)(hwp->rxtx_buffer & 0xFF);
        uint16_t next_head = (s_uart_ring_head[idx] + 1) & (UART_RX_RING_SIZE - 1);
        if (next_head != s_uart_ring_tail[idx])
        {
            s_uart_rx_ring[idx][s_uart_ring_head[idx]] = byte;
            s_uart_ring_head[idx] = next_head;
        }
    }

    // Acknowledge interrupt cause in UART peripheral
    hwp->irq_cause = cause;

    // Safety: ensure RX interrupts are always active and mask only unwanted TX/modem interrupts
    hwp->irq_mask = (hwp->irq_mask & ~(UART_TX_DATA_NEEDED | UART_TX_MODEM_STATUS)) |
                    (UART_RX_DATA_AVAILABLE | UART_RX_TIMEOUT);

    if (g_uartIrqHandlers[idx])
    {
        g_uartIrqHandlers[idx](id, cause);
    }
}
