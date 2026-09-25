/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * MCD (Medium Card Driver) SD/MMC Header for RDA8809
 */

#ifndef _MCD_SDMMC_H_
#define _MCD_SDMMC_H_

#include "cs_types.h"

typedef enum
{
    MCD_CARD_TYPE_UNKNOWN = 0,
    MCD_CARD_TYPE_SD_V1,
    MCD_CARD_TYPE_SDSC_V2,
    MCD_CARD_TYPE_SDHC_V2,
    MCD_CARD_TYPE_MMC
} mcd_card_type_t;

typedef enum
{
    MCD_STATUS_NOTPRESENT = 0,
    MCD_STATUS_INSERTED,
    MCD_STATUS_OPEN,
    MCD_STATUS_ERROR
} mcd_status_t;

typedef enum
{
    MCD_ERR_NO = 0,
    MCD_ERR_CARD_TIMEOUT,
    MCD_ERR_NO_CARD,
    MCD_ERR_CARD_NO_RESPONSE,
    MCD_ERR_CARD_RESPONSE_BAD_CRC,
    MCD_ERR_INIT_FAILED,
    MCD_ERR_READ_FAILED,
    MCD_ERR_WRITE_FAILED,
    MCD_ERR_PARAM
} MCD_ERR_T;

typedef struct
{
    mcd_card_type_t cardType;
    mcd_status_t    status;
    UINT32          capacityMB;
    UINT32          sectorCount;
    UINT32          sectorSize;
    UINT16          rca;
    char            productName[6];
    UINT32          serialNumber;
    UINT16          manufYear;
    UINT8           manufMonth;
    BOOL            is4BitBus;
    UINT32          clockFreqHz;
} mcd_card_info_t;

MCD_ERR_T mcd_Open(void);
void      mcd_Close(void);
BOOL      mcd_IsOpen(void);
MCD_ERR_T mcd_Read(UINT32 sector, UINT8 *buf, UINT32 count);
MCD_ERR_T mcd_Write(UINT32 sector, const UINT8 *buf, UINT32 count);
BOOL      mcd_GetCardInfo(mcd_card_info_t *info);

#endif // _MCD_SDMMC_H_
