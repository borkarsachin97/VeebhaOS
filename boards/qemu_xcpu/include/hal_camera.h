/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Camera HAL interface for RDA8809
 */

#ifndef _HAL_CAMERA_H_
#define _HAL_CAMERA_H_

#include "cs_types.h"
#include "camera.h"

// Decimation / Subsampling ratios
typedef enum
{
    CAM_ROW_RATIO_1_1 = 0,
    CAM_ROW_RATIO_1_2 = 1,
    CAM_ROW_RATIO_1_3 = 2,
    CAM_ROW_RATIO_1_4 = 3
} HAL_CAM_ROW_RATIO_T;

typedef enum
{
    CAM_COL_RATIO_1_1 = 0,
    CAM_COL_RATIO_1_2 = 1,
    CAM_COL_RATIO_1_3 = 2,
    CAM_COL_RATIO_1_4 = 3
} HAL_CAM_COL_RATIO_T;

// Transfer status
typedef enum
{
    CAM_XFER_SUCCESS                = 0,
    CAM_XFER_NOT_FINISHED           = 1,
    CAM_XFER_FINISHED_WITH_OVERFLOW = 2,
    CAM_XFER_ERR_WRONG_PARAM        = 3
} HAL_CAMERA_XFER_STATUS_T;

// Interrupt Cause Flags
typedef struct
{
    UINT32 overflow:1;
    UINT32 fstart:1;
    UINT32 fend:1;
    UINT32 dma:1;
} HAL_CAMERA_IRQ_CAUSE_T;

typedef void (*HAL_CAMERA_IRQ_HANDLER_T)(HAL_CAMERA_IRQ_CAUSE_T cause);

// Configuration Structure
typedef struct
{
    BOOL rstActiveH;
    BOOL pdnActiveH;
    BOOL dropFrame;
    UINT8 camClkDiv;                        // Divider for MCLK output (e.g. 15 for 10.4MHz)
    BOOL vsync_inv;
    BOOL href_inv;
    BOOL pixclk_inv;
    HAL_CAM_ROW_RATIO_T rowRatio;
    HAL_CAM_COL_RATIO_T colRatio;
    BOOL cropEnable;
    UINT16 dstWinColStart;
    UINT16 dstWinColEnd;
    UINT16 dstWinRowStart;
    UINT16 dstWinRowEnd;
    UINT8 reOrder;
    UINT8 format;                           // CAMERA_DATAFORMAT_YUV422 or RGB565
} HAL_CAMERA_CFG_T;

// Camera HAL Public API
void hal_CameraPowerOn(void);
void hal_CameraPowerDown(void);
void hal_CameraReset(BOOL inReset);
void hal_CameraSetupClockDivider(UINT8 divider);
void hal_CameraSetVsyncInvert(BOOL invert);
void hal_CameraControllerEnable(BOOL enable);

void hal_CameraOpen(const HAL_CAMERA_CFG_T *cfg);
void hal_CameraClose(void);

UINT8 hal_CameraStartXfer(UINT32 bufSize, UINT8 *buffer);
HAL_CAMERA_XFER_STATUS_T hal_CameraStopXfer(BOOL stopController);
HAL_CAMERA_XFER_STATUS_T hal_CameraWaitXferFinish(void);
BOOL hal_CameraIsXferDone(void);
UINT32 hal_CameraGetXferRemaining(void);

void hal_CameraIrqSetHandler(HAL_CAMERA_IRQ_HANDLER_T handler);
void hal_CameraIrqSetMask(HAL_CAMERA_IRQ_CAUSE_T mask);
void hal_CameraIrqHandler(void);

#endif // _HAL_CAMERA_H_
