/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Hardware register definitions for RDA8809 Camera Controller (0x01A18000)
 */

#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "cs_types.h"
#include "global_macros.h"

#define REG_CAMERA_BASE                     0x01A18000
#define FIFORAM_SIZE                        80

typedef volatile struct
{
    REG32 CTRL;                             // 0x00000000
    REG32 STATUS;                           // 0x00000004
    REG32 DATA;                             // 0x00000008
    REG32 IRQ_MASK;                         // 0x0000000C
    REG32 IRQ_CLEAR;                        // 0x00000010
    REG32 IRQ_CAUSE;                        // 0x00000014
    REG32 CMD_SET;                          // 0x00000018
    REG32 CMD_CLR;                          // 0x0000001C
    REG32 DSTWINCOL;                        // 0x00000020
    REG32 DSTWINROW;                        // 0x00000024
    REG32 SCALE_CONFIG;                     // 0x00000028
    REG32 CAM_SPI_REG_0;                    // 0x0000002C
    REG32 CAM_SPI_REG_1;                    // 0x00000030
    REG32 CAM_SPI_REG_2;                    // 0x00000034
    REG32 CAM_SPI_REG_3;                    // 0x00000038
    REG32 CAM_SPI_REG_4;                    // 0x0000003C
    REG32 CAM_SPI_REG_5;                    // 0x00000040
    REG32 CAM_SPI_REG_6;                    // 0x00000044
    REG32 CAM_SPI_OBSERVE_REG_0;            // 0x00000048 (read only)
    REG32 CAM_SPI_OBSERVE_REG_1;            // 0x0000004C (read only)
    REG32 Reserved_00000050[108];           // 0x00000050..0x000001FF
    struct
    {
        REG32 RAMData;                      // 0x00000200 + i*4
    } FIFORAM[FIFORAM_SIZE];
} HWP_CAMERA_T;

#define hwp_camera                          ((HWP_CAMERA_T*) KSEG1(REG_CAMERA_BASE))

// CTRL Register bitfields
#define CAMERA_ENABLE                       (1 << 0)
#define CAMERA_1_BUFENABLE                  (1 << 1)
#define CAMERA_DATAFORMAT(n)                (((n) & 3) << 4)
#define CAMERA_DATAFORMAT_RGB565            (0 << 4)
#define CAMERA_DATAFORMAT_YUV422            (1 << 4)
#define CAMERA_DATAFORMAT_JPEG              (2 << 4)
#define CAMERA_DATAFORMAT_RESERVED          (3 << 4)
#define CAMERA_RESET_POL                    (1 << 8)
#define CAMERA_RESET_POL_INVERT             (1 << 8)
#define CAMERA_RESET_POL_NORMAL             (0 << 8)
#define CAMERA_PWDN_POL                     (1 << 9)
#define CAMERA_PWDN_POL_INVERT              (1 << 9)
#define CAMERA_PWDN_POL_NORMAL              (0 << 9)
#define CAMERA_VSYNC_POL                    (1 << 10)
#define CAMERA_VSYNC_POL_INVERT             (1 << 10)
#define CAMERA_VSYNC_POL_NORMAL             (0 << 10)
#define CAMERA_HREF_POL                     (1 << 11)
#define CAMERA_HREF_POL_INVERT              (1 << 11)
#define CAMERA_HREF_POL_NORMAL              (0 << 11)
#define CAMERA_PIXCLK_POL                   (1 << 12)
#define CAMERA_PIXCLK_POL_INVERT            (1 << 12)
#define CAMERA_PIXCLK_POL_NORMAL            (0 << 12)
#define CAMERA_VSYNC_DROP                   (1 << 14)
#define CAMERA_DECIMFRM(n)                  (((n) & 3) << 16)
#define CAMERA_DECIMFRM_ORIGINAL            (0 << 16)
#define CAMERA_DECIMFRM_DIV_2               (1 << 16)
#define CAMERA_DECIMFRM_DIV_3               (2 << 16)
#define CAMERA_DECIMFRM_DIV_4               (3 << 16)
#define CAMERA_DECIMCOL(n)                  (((n) & 3) << 18)
#define CAMERA_DECIMCOL_ORIGINAL            (0 << 18)
#define CAMERA_DECIMCOL_DIV_2               (1 << 18)
#define CAMERA_DECIMCOL_DIV_3               (2 << 18)
#define CAMERA_DECIMCOL_DIV_4               (3 << 18)
#define CAMERA_DECIMROW(n)                  (((n) & 3) << 20)
#define CAMERA_DECIMROW_ORIGINAL            (0 << 20)
#define CAMERA_DECIMROW_DIV_2               (1 << 20)
#define CAMERA_DECIMROW_DIV_3               (2 << 20)
#define CAMERA_DECIMROW_DIV_4               (3 << 20)
#define CAMERA_REORDER(n)                   (((n) & 7) << 24)
#define CAMERA_CROPEN                       (1 << 28)
#define CAMERA_CROPEN_ENABLE                (1 << 28)
#define CAMERA_CROPEN_DISABLE               (0 << 28)
#define CAMERA_BIST_MODE                    (1 << 30)
#define CAMERA_TEST                         (1 << 31)

// STATUS Register bitfields
#define CAMERA_OVFL                         (1 << 0)
#define CAMERA_VSYNC_R                      (1 << 1)
#define CAMERA_VSYNC_F                      (1 << 2)
#define CAMERA_DMA_DONE                     (1 << 3)
#define CAMERA_FIFO_EMPTY                   (1 << 4)

// CMD_SET Register bitfields
#define CAMERA_PWDN                         (1 << 0)
#define CAMERA_RESET                        (1 << 4)
#define CAMERA_FIFO_RESET                   (1 << 8)

// DSTWINCOL & DSTWINROW
#define CAMERA_DSTWINCOLSTART(n)            (((n) & 0xFFF) << 0)
#define CAMERA_DSTWINCOLEND(n)              (((n) & 0xFFF) << 16)
#define CAMERA_DSTWINROWSTART(n)            (((n) & 0xFFF) << 0)
#define CAMERA_DSTWINROWEND(n)              (((n) & 0xFFF) << 16)

// SCALE_CONFIG
#define CAMERA_SCALE_EN                     (1 << 0)
#define CAMERA_SCALE_COL(n)                 (((n) & 3) << 8)
#define CAMERA_SCALE_ROW(n)                 (((n) & 3) << 16)

#endif // _CAMERA_H_
