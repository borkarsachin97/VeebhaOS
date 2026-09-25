/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Hardware GOUDA 2D Blitter & LCD Controller Driver Implementation for RDA8809
 */

#include "cs_types.h"
#include "global_macros.h"
#include "sys_ctrl.h"
#include "sys_irq.h"
#include "timer.h"
#include "os_vector_table.h"
#include "hal_gouda.h"
#include "FreeRTOS.h"
#include "semphr.h"

// External USB logging
extern void os_log_printf(const char *fmt, ...);

static volatile bool g_gouda_in_progress = false;

// =============================================================================
//  Assembly Delay Helper
// =============================================================================
static inline void gouda_wait_hw_idle(void)
{
    while ((hwp_gouda->gd_status & (GOUDA_STATUS_RUNNING | GOUDA_STATUS_BUSY)) != 0);
}

// =============================================================================
//  hal_GoudaInit
// =============================================================================
void hal_GoudaInit(void)
{
    // 1. Enable Gouda module clock and clear peripheral reset
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_UNLOCK;
    hwp_sysCtrl->Sys_Rst_Clr = SYS_CTRL_CLR_RST_GOUDA;
    hwp_sysCtrl->Clk_Sys_Enable = SYS_CTRL_ENABLE_SYS_GOUDA;
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_LOCK;

    // 2. Configure Gouda LCD controller timing and control registers (8-bit parallel bus)
    hwp_gouda->gd_spilcd_config = 0x00000000;
    hwp_gouda->gd_lcd_timing    = 0x00141488; // 312 MHz 8-bit bus timing (TAS=8, TAH=8, PWL=20, PWH=20 from SDK)

    /* Hardware Reset Pulse via Gouda resetb line (bit 25) */
    hwp_gouda->gd_lcd_ctrl      = 0x00000020; /* Assert Reset (resetb=0) */
    timer_delay_ms(25);
    hwp_gouda->gd_lcd_ctrl      = 0x02000020; /* Release Reset (resetb=1) */
    timer_delay_ms(50);

    // 3. Clear all video layer and overlay formats
    hwp_gouda->gd_vl_input_fmt = 0x00000000;
    hwp_gouda->Overlay_Layer[0].gd_ol_input_fmt = 0x00000000;
    hwp_gouda->Overlay_Layer[1].gd_ol_input_fmt = 0x00000000;
    hwp_gouda->Overlay_Layer[2].gd_ol_input_fmt = 0x00000000;

    // 4. Set default ROI (176x220: BR=(219, 175))
    hwp_gouda->gd_roi_tl_ppos = 0x00000000;
    hwp_gouda->gd_roi_br_ppos = 0x00DB00AF;
    hwp_gouda->gd_roi_bg_color = 0x00000000;

    // 5. Disable GOUDA EOF IRQ in hardware (we use fast ~1.4ms hardware polling)
    hwp_gouda->gd_eof_irq_mask = 0;
    hwp_gouda->gd_eof_irq = 1; // Clear any pending

    os_log_printf("[GOUDA] GOUDA 2D Blitter initialized (Timing=0x%08X, Ctrl=0x%08X, Polled DMA)\n",
                  hwp_gouda->gd_lcd_timing, hwp_gouda->gd_lcd_ctrl);
}

// =============================================================================
//  Single-Access Transactions
// =============================================================================
void hal_GoudaWriteCmd(uint8_t cmd)
{
    gouda_wait_hw_idle();
    hwp_gouda->gd_lcd_ctrl = 0x02000020;
    hwp_gouda->gd_lcd_single_access = GOUDA_START_WRITE | ((uint32_t)cmd & 0xFF);
}

void hal_GoudaWriteDataByte(uint8_t data)
{
    gouda_wait_hw_idle();
    hwp_gouda->gd_lcd_ctrl = 0x02000020;
    hwp_gouda->gd_lcd_single_access = GOUDA_START_WRITE | GOUDA_TYPE_DATA | ((uint32_t)data & 0xFF);
}

void hal_GoudaWriteData(uint16_t data)
{
    hal_GoudaWriteDataByte((data >> 8) & 0xFF);
    hal_GoudaWriteDataByte(data & 0xFF);
}

void hal_GoudaWriteReg(uint16_t reg, uint16_t val)
{
    hal_GoudaWriteCmd(((uint16_t)(reg) >> 8) & 0xFF);
    hal_GoudaWriteCmd((uint16_t)(reg) & 0xFF);
    hal_GoudaWriteDataByte(((uint16_t)(val) >> 8) & 0xFF);
    hal_GoudaWriteDataByte((uint16_t)(val) & 0xFF);
}

uint16_t hal_GoudaReadReg(uint16_t reg)
{
    UINT32 old_timing = hwp_gouda->gd_lcd_timing;

    /* Slower timing for read cycles: tas=2, tah=4, pwl=40, pwh=40 */
    hwp_gouda->gd_lcd_timing = (2 << 0) | (4 << 4) | (40 << 8) | (40 << 16);

    hal_GoudaWriteCmd(((uint16_t)(reg) >> 8) & 0xFF);
    hal_GoudaWriteCmd((uint16_t)(reg) & 0xFF);

    /* Read High Byte */
    gouda_wait_hw_idle();
    hwp_gouda->gd_lcd_ctrl = 0x02000020;
    hwp_gouda->gd_lcd_single_access = GOUDA_START_READ | GOUDA_TYPE_DATA;
    gouda_wait_hw_idle();
    uint8_t b0 = (uint8_t)(hwp_gouda->gd_lcd_single_access & 0xFF);

    /* Read Low Byte */
    gouda_wait_hw_idle();
    hwp_gouda->gd_lcd_ctrl = 0x02000020;
    hwp_gouda->gd_lcd_single_access = GOUDA_START_READ | GOUDA_TYPE_DATA;
    gouda_wait_hw_idle();
    uint8_t b1 = (uint8_t)(hwp_gouda->gd_lcd_single_access & 0xFF);

    /* Restore normal bus timing */
    hwp_gouda->gd_lcd_timing = old_timing;

    return ((uint16_t)b0 << 8) | b1;
}

// =============================================================================
//  hal_GoudaBlitRoi
// =============================================================================
bool hal_GoudaBlitRoi(const uint16_t *buf, uint16_t buf_w, uint16_t buf_h,
                      uint16_t dst_x, uint16_t dst_y,
                      uint16_t width, uint16_t height,
                      bool non_blocking)
{
    if (!buf || width == 0 || height == 0) return false;

    // Ensure any previous DMA transfer is completed
    hal_GoudaWaitIdle(50);

    uint16_t dst_end_x = dst_x + width - 1;
    uint16_t dst_end_y = dst_y + height - 1;

    // 1. Configure Gouda ROI bounding box (Y in high 16 bits, X in low 16 bits)
    hwp_gouda->gd_roi_tl_ppos = ((UINT32)dst_y << 16) | dst_x;
    hwp_gouda->gd_roi_br_ppos = ((UINT32)dst_end_y << 16) | dst_end_x;

    // 2. Configure Overlay Layer 0 as the source buffer
    // RGB565 format (0), stride = line width in words, active bit (1 << 31)
    hwp_gouda->Overlay_Layer[0].gd_ol_input_fmt =
        (((UINT32)buf_w & 0x1FFF) << 2) | (1U << 31);

    // Overlay spans source buffer coordinates matching the ROI target
    if (buf_w == width && buf_h == height) {
        // Tightly packed sub-rectangle buffer (e.g. LVGL partial flush)
        hwp_gouda->Overlay_Layer[0].gd_ol_tl_ppos = ((UINT32)dst_y << 16) | dst_x;
        hwp_gouda->Overlay_Layer[0].gd_ol_br_ppos = ((UINT32)dst_end_y << 16) | dst_end_x;
    } else {
        // Full framebuffer source buffer
        hwp_gouda->Overlay_Layer[0].gd_ol_tl_ppos = 0;
        hwp_gouda->Overlay_Layer[0].gd_ol_br_ppos = ((UINT32)(buf_h - 1) << 16) | (buf_w - 1);
    }

    // Full opacity
    hwp_gouda->Overlay_Layer[0].gd_ol_blend_opt = (255U << 20);

    // Buffer address in KSEG1 (uncached) or physical
    hwp_gouda->Overlay_Layer[0].gd_ol_rgb_src = ((UINT32)buf | 0x20000000);

    // Disable video layer and other overlay layers
    hwp_gouda->gd_vl_input_fmt = 0;
    hwp_gouda->Overlay_Layer[1].gd_ol_input_fmt = 0;
    hwp_gouda->Overlay_Layer[2].gd_ol_input_fmt = 0;

    // Direct CS0 LCD streaming (resetb = 1)
    hwp_gouda->gd_lcd_ctrl = 0x02000020;
    hwp_gouda->gd_lcd_mem_address = 0;
    hwp_gouda->gd_lcd_stride_offset = 0;

    // Drain write buffers and ensure cache coherency before triggering DMA
    extern void mips_cache_flush(void);
    mips_cache_flush();
    __asm__ volatile ("" ::: "memory");

    g_gouda_in_progress = true;

    // 3. Trigger Gouda DMA Blit
    hwp_gouda->gd_command = GOUDA_CMD_START_ROI;

    if (!non_blocking)
    {
        bool ret = hal_GoudaWaitIdle(100);
        hwp_gouda->Overlay_Layer[0].gd_ol_input_fmt = 0;
        return ret;
    }

    return true;
}

// =============================================================================
//  hal_GoudaFillRect
// =============================================================================
void hal_GoudaFillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (w == 0 || h == 0) return;

    hal_GoudaWaitIdle(50);

    uint16_t dst_end_x = x + w - 1;
    uint16_t dst_end_y = y + h - 1;

    hwp_gouda->gd_roi_tl_ppos = ((UINT32)y << 16) | x;
    hwp_gouda->gd_roi_br_ppos = ((UINT32)dst_end_y << 16) | dst_end_x;
    hwp_gouda->gd_roi_bg_color = (UINT32)color;

    hwp_gouda->gd_vl_input_fmt = 0;
    hwp_gouda->Overlay_Layer[0].gd_ol_input_fmt = 0;
    hwp_gouda->Overlay_Layer[1].gd_ol_input_fmt = 0;
    hwp_gouda->Overlay_Layer[2].gd_ol_input_fmt = 0;
    hwp_gouda->gd_lcd_ctrl = 0x02000020;
    hwp_gouda->gd_lcd_mem_address = 0;
    hwp_gouda->gd_lcd_stride_offset = 0;

    hwp_gouda->gd_command = GOUDA_CMD_START_ROI;

    hal_GoudaWaitIdle(50);
}

// =============================================================================
//  hal_GoudaWaitIdle
// =============================================================================
bool hal_GoudaWaitIdle(uint32_t timeout_ms)
{
    if (!hal_GoudaIsActive())
    {
        hwp_gouda->Overlay_Layer[0].gd_ol_input_fmt = 0;
        g_gouda_in_progress = false;
        return true;
    }

    // Direct hardware status check with safe ms-level timeout
    UINT32 start = timer_get_ms();
    while (hal_GoudaIsActive())
    {
        if (timeout_ms > 0 && (timer_get_ms() - start) > timeout_ms)
        {
            hwp_gouda->Overlay_Layer[0].gd_ol_input_fmt = 0;
            g_gouda_in_progress = false;
            return false;
        }
    }

    hwp_gouda->Overlay_Layer[0].gd_ol_input_fmt = 0;
    g_gouda_in_progress = false;
    return true;
}

// =============================================================================
//  hal_GoudaIsActive
// =============================================================================
bool hal_GoudaIsActive(void)
{
    return (hwp_gouda->gd_status & (GOUDA_STATUS_RUNNING | GOUDA_STATUS_BUSY)) != 0;
}

// =============================================================================
//  hal_GoudaIrqHandler (End-of-Frame Completion Stub)
// =============================================================================
void hal_GoudaIrqHandler(uint32_t cause, uint32_t epc)
{
    (void)cause;
    (void)epc;

    // Acknowledge and clear Gouda EOF interrupt in hardware
    hwp_gouda->gd_eof_irq = 1;
    g_gouda_in_progress = false;
}
