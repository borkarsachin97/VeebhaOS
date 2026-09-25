/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * HAL SDMMC Driver Header for RDA8809
 */

#ifndef _HAL_SDMMC_H_
#define _HAL_SDMMC_H_

#include "cs_types.h"
#include "sdmmc.h"
#include "sys_ifc.h"

#define HAL_SDMMC_ACMD_SEL              0x80000000
#define HAL_SDMMC_CMD_MASK              0x3F

typedef enum
{
    HAL_SDMMC_CMD_GO_IDLE_STATE         = 0,
    HAL_SDMMC_CMD_MMC_SEND_OP_COND      = 1,
    HAL_SDMMC_CMD_ALL_SEND_CID          = 2,
    HAL_SDMMC_CMD_SEND_RELATIVE_ADDR    = 3,
    HAL_SDMMC_CMD_SET_DSR               = 4,
    HAL_SDMMC_CMD_SWITCH                = 6,
    HAL_SDMMC_CMD_SELECT_CARD           = 7,
    HAL_SDMMC_CMD_SEND_IF_COND          = 8,
    HAL_SDMMC_CMD_SEND_CSD              = 9,
    HAL_SDMMC_CMD_STOP_TRANSMISSION     = 12,
    HAL_SDMMC_CMD_SEND_STATUS           = 13,
    HAL_SDMMC_CMD_SET_BLOCKLEN          = 16,
    HAL_SDMMC_CMD_READ_SINGLE_BLOCK     = 17,
    HAL_SDMMC_CMD_READ_MULT_BLOCK       = 18,
    HAL_SDMMC_CMD_WRITE_SINGLE_BLOCK    = 24,
    HAL_SDMMC_CMD_WRITE_MULT_BLOCK      = 25,
    HAL_SDMMC_CMD_APP_CMD               = 55,
    HAL_SDMMC_CMD_SET_BUS_WIDTH         = (6  | HAL_SDMMC_ACMD_SEL),
    HAL_SDMMC_CMD_SEND_OP_COND          = (41 | HAL_SDMMC_ACMD_SEL)
} HAL_SDMMC_CMD_T;

typedef enum
{
    HAL_SDMMC_DIRECTION_READ,
    HAL_SDMMC_DIRECTION_WRITE
} HAL_SDMMC_DIRECTION_T;

typedef struct
{
    UINT8*                  sysMemAddr;
    UINT8*                  sdCardAddr;
    UINT32                  blockNum;
    UINT32                  blockSize;
    HAL_SDMMC_DIRECTION_T   direction;
} HAL_SDMMC_TRANSFER_T;

typedef enum
{
    HAL_SDMMC_DATA_BUS_WIDTH_1 = 0x0,
    HAL_SDMMC_DATA_BUS_WIDTH_4 = 0x2
} HAL_SDMMC_DATA_BUS_WIDTH_T;

typedef union
{
    UINT32 reg;
    struct
    {
        UINT32 operationNotOver     :1;
        UINT32 busy                 :1;
        UINT32 dataLineBusy         :1;
        UINT32 suspend              :1;
        UINT32                      :4;
        UINT32 responseCrcError     :1;
        UINT32 noResponseReceived   :1;
        UINT32                      :2;
        UINT32 crcStatus            :3;
        UINT32                      :1;
        UINT32 dataError            :8;
        UINT32 dat3Val              :1;
        UINT32                      :7;
    } fields;
} HAL_SDMMC_OP_STATUS_T;

void hal_SdmmcOpen(UINT8 clk_adj);
void hal_SdmmcClose(void);
void hal_SdmmcSetClk(UINT32 clock);
void hal_SdmmcSetClkMode(BOOL auto_clk);
void hal_SdmmcEnterDataTransferMode(void);
HAL_SDMMC_OP_STATUS_T hal_SdmmcGetOpStatus(void);
void hal_SdmmcSendCmd(HAL_SDMMC_CMD_T cmd, UINT32 arg, BOOL suspend);
BOOL hal_SdmmcCmdDone(void);
BOOL hal_SdmmcNeedResponse(HAL_SDMMC_CMD_T cmd);
void hal_SdmmcGetResp(HAL_SDMMC_CMD_T cmd, UINT32 *arg, BOOL suspend);
void hal_SdmmcSetDataWidth(HAL_SDMMC_DATA_BUS_WIDTH_T width);
BOOL hal_SdmmcTransfer(HAL_SDMMC_TRANSFER_T *transfer);
BOOL hal_SdmmcTransferDone(void);
void hal_SdmmcStopTransfer(HAL_SDMMC_TRANSFER_T *transfer);

#endif // _HAL_SDMMC_H_
