/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Configuration Registers (cfg_regs) & Pin Mux HAL for RDA8809
 */

#ifndef _HAL_CFG_REGS_H_
#define _HAL_CFG_REGS_H_

#include "cs_types.h"
#include "cfg_regs.h"

#define RDA8809_EXPECTED_CHIP_ID    0x88094009

typedef enum
{
    HAL_PIN_MUX_SDMMC_MODE      = 0,    // Pins routed to SDMMC 4-bit controller
    HAL_PIN_MUX_DIGRF_MODE      = 1,    // Pins routed to Baseband DigRF
    HAL_PIN_MUX_I2C2_MODE       = 2,    // Pins routed to I2C2 master
    HAL_PIN_MUX_USB_BACKUP_MODE = 3,    // Pins routed to USB backup
    HAL_PIN_MUX_I2C3_ON_SPI1    = 4     // I2C3 routed to SPI1 pins
} HAL_PIN_MUX_T;

typedef enum
{
    HAL_IO_DRIVE_SDMMC = 0,
    HAL_IO_DRIVE_LCD   = 1,
    HAL_IO_DRIVE_SPI   = 2,
    HAL_IO_DRIVE_CAMERA= 3
} HAL_IO_DRIVE_DOMAIN_T;

// Public CFG_REGS HAL API
void hal_CfgRegsInit(void);

UINT32 hal_CfgGetChipId(void);
UINT32 hal_CfgGetBuildVersion(void);
BOOL hal_CfgIsRda8809(void);

void hal_CfgSetPinMux(HAL_PIN_MUX_T mux, BOOL enable);
UINT32 hal_CfgGetAltMux(void);

void hal_CfgSetIoDriveStrength(HAL_IO_DRIVE_DOMAIN_T domain, UINT8 strength);

void hal_CfgSetFmPower(BOOL enable);

#endif // _HAL_CFG_REGS_H_
