/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Hardware GOUDA 2D Blitter & LCD Controller Driver Header for RDA8809
 */

#ifndef _HAL_GOUDA_H_
#define _HAL_GOUDA_H_

#include "cs_types.h"
#include "global_macros.h"

// =============================================================================
//  GOUDA HARDWARE REGISTERS & BASE ADDRESS
// =============================================================================
#define REG_GOUDA_BASE              0x01A21000

typedef volatile struct
{
    REG32                          gd_command;                   //0x00000000
    REG32                          gd_status;                    //0x00000004
    REG32                          gd_eof_irq;                   //0x00000008
    REG32                          gd_eof_irq_mask;              //0x0000000C
    REG32                          gd_roi_tl_ppos;               //0x00000010
    REG32                          gd_roi_br_ppos;               //0x00000014
    REG32                          gd_roi_bg_color;              //0x00000018
    REG32                          gd_vl_input_fmt;              //0x0000001C
    REG32                          gd_vl_tl_ppos;                //0x00000020
    REG32                          gd_vl_br_ppos;                //0x00000024
    REG32                          gd_vl_extents;                //0x00000028
    REG32                          gd_vl_blend_opt;              //0x0000002C
    REG32                          gd_vl_y_src;                  //0x00000030
    REG32                          gd_vl_u_src;                  //0x00000034
    REG32                          gd_vl_v_src;                  //0x00000038
    REG32                          gd_vl_resc_ratio;             //0x0000003C
    struct
    {
        REG32                      gd_ol_input_fmt;              //0x00000040
        REG32                      gd_ol_tl_ppos;                //0x00000044
        REG32                      gd_ol_br_ppos;                //0x00000048
        REG32                      gd_ol_blend_opt;              //0x0000004C
        REG32                      gd_ol_rgb_src;                //0x00000050
    } Overlay_Layer[3];
    REG32                          gd_lcd_ctrl;                  //0x0000007C
    REG32                          gd_lcd_timing;                //0x00000080
    REG32                          gd_lcd_mem_address;           //0x00000084
    REG32                          gd_lcd_stride_offset;         //0x00000088
    REG32                          gd_lcd_single_access;         //0x0000008C
    REG32                          gd_spilcd_config;             //0x00000090
    REG32                          gd_spilcd_rd;                 //0x00000094
    REG32                          gd_vl_fix_ratio;              //0x00000098
} HWP_GOUDA_T;

#define hwp_gouda                   ((HWP_GOUDA_T*) KSEG1(REG_GOUDA_BASE))

// GOUDA Status & Command Masks
#define GOUDA_TYPE_DATA             (1 << 16)
#define GOUDA_START_WRITE           (1 << 17)
#define GOUDA_START_READ            (1 << 18)

#define GOUDA_CMD_START_ROI         (1 << 0)
#define GOUDA_STATUS_RUNNING        (1 << 0)
#define GOUDA_STATUS_BUSY           (1 << 4)

// =============================================================================
//  PUBLIC GOUDA API
// =============================================================================

/**
 * @brief Initialize the hardware GOUDA controller, timing registers, and FreeRTOS synchronization.
 */
void hal_GoudaInit(void);

/**
 * @brief Send an 8-bit command byte to the LCD controller over GOUDA single access.
 */
void hal_GoudaWriteCmd(uint8_t cmd);

/**
 * @brief Send an 8-bit data byte to the LCD controller over GOUDA single access.
 */
void hal_GoudaWriteDataByte(uint8_t data);

/**
 * @brief Send a 16-bit data word to the LCD controller over GOUDA single access.
 */
void hal_GoudaWriteData(uint16_t data);

/**
 * @brief Write a 16-bit register index and 16-bit value to the LCD controller.
 */
void hal_GoudaWriteReg(uint16_t reg, uint16_t val);

/**
 * @brief Read a 16-bit register value from the LCD controller via GOUDA single-access read.
 */
uint16_t hal_GoudaReadReg(uint16_t reg);

/**
 * @brief Blit a sub-rectangle of an RGB565 buffer directly to the display via GOUDA DMA.
 * 
 * @param buf Pointer to the source RGB565 pixel buffer in memory.
 * @param buf_w Width of the source buffer in pixels (stride).
 * @param buf_h Height of the source buffer in pixels.
 * @param dst_x Target X coordinate on the display.
 * @param dst_y Target Y coordinate on the display.
 * @param width Width of the blit region.
 * @param height Height of the blit region.
 * @param non_blocking If TRUE, returns immediately while DMA runs in the background.
 * @return true if blit was successfully initiated.
 */
bool hal_GoudaBlitRoi(const uint16_t *buf, uint16_t buf_w, uint16_t buf_h,
                      uint16_t dst_x, uint16_t dst_y,
                      uint16_t width, uint16_t height,
                      bool non_blocking);

/**
 * @brief Clear a display region with a solid background color using GOUDA hardware fill.
 */
void hal_GoudaFillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * @brief Wait until the active GOUDA DMA transfer completes using FreeRTOS semaphore blocking.
 * 
 * @param timeout_ms Maximum time to wait in milliseconds.
 * @return true if DMA finished successfully, false if timed out.
 */
bool hal_GoudaWaitIdle(uint32_t timeout_ms);

/**
 * @brief Check if GOUDA hardware is currently active/busy.
 */
bool hal_GoudaIsActive(void);

/**
 * @brief Interrupt service routine for GOUDA End-Of-Frame (EOF) DMA completion.
 */
void hal_GoudaIrqHandler(uint32_t cause, uint32_t epc);

#endif // _HAL_GOUDA_H_
