/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Internal SPI (ISPI) Master Driver for RDA8809 / RDA8955 SoC
 * Connected to SPI3 controller (0x01A13000)
 *
 * Chip Select mappings:
 *   CS0: PMU (Power Management Unit / RDA1203)
 *   CS1: ABB (Analog Baseband / Audio Codec / USB PHY Control)
 *   CS2: FM  (FM Radio Analog Core)
 */

#ifndef _HAL_ISPI_H_
#define _HAL_ISPI_H_

#include "cs_types.h"
#include "global_macros.h"
#include "spi.h"

// ============================================================================
// ISPI Chip Select Enumeration
// ============================================================================
typedef enum
{
    HAL_ISPI_CS_PMU = 0,    // PMU / Power Management (RDA1203)
    HAL_ISPI_CS_ABB = 1,    // ABB / Analog Baseband Audio Codec & USB PHY
    HAL_ISPI_CS_FM  = 2,    // FM Radio Analog Core
    HAL_ISPI_CS_QTY
} HAL_ISPI_CS_T;

// ============================================================================
// ABB (Analog Baseband) Register Index Definitions (RDA8809)
// ============================================================================
#define ABB_REG_USB_CONTROL         0x04    // USB PHY and D+ pullup control
#define ABB_REG_MIC_SETTING         0x05    // Microphone input settings
#define ABB_REG_LINEIN_SETTING      0x08    // Line-in / FM audio to PA routing
#define ABB_REG_CODEC_LDO_SETTING1  0x09    // Codec power-down & PA disable bits
#define ABB_REG_CODEC_LDO_SETTING2  0x0A    // Codec VCOM/VREF reference bias
#define ABB_REG_CODEC_MISC_SETTING  0x0B    // HP VCOM setting
#define ABB_REG_CODEC_MODE_SEL      0x0C    // DAC / Line-in / Speaker mode select
#define ABB_REG_CODEC_POWER_CTRL    0x0E    // Codec reset, DAC enable, SPK PA enable
#define ABB_REG_CODEC_CLOCK_CODEC   0x11    // Codec clock divider
#define ABB_REG_CODEC_FM_MODE       0x28    // FM analog audio mode
#define ABB_REG_CODEC_DIG_EN        0x29    // Digital codec enable & DWA
#define ABB_REG_CODEC_DIG_DAC_GAIN  0x2A    // Digital DAC volume & mute

// ============================================================================
// PMU (Power Management Unit) Register Index Definitions (RDA1203 / RDA8809)
// ============================================================================
#define PMU_REG_LDO_SETTINGS        0x02    // LDO enables (FM, BT, ABB, LCD, etc.)
#define PMU_REG_LDO_ACTIVE1         0x03    // Active mode LDO power-down profile (bit 6 = vMmcOff)
#define PMU_REG_LDO_ACTIVE2         0x04    // Active mode LDO voltages (bit 8 = vMmcIs1_8)
#define PMU_REG_LDO_ACTIVE5         0x07    // Active mode LDO drive strength (bits 5:3 = vMmcIbit)
#define PMU_REG_LDO_LP1             0x08    // Low-power mode LDO profile 1
#define PMU_REG_LDO_LP2             0x09    // Low-power mode LDO voltages
#define PMU_REG_LDO_BUCK1           0x2D    // Buck converter 1
#define PMU_REG_LDO_BUCK2           0x2E    // Buck converter 2

// ============================================================================
// Public ISPI API
// ============================================================================
void    hal_IspiInit(void);
void    hal_IspiWrite(HAL_ISPI_CS_T cs, UINT16 reg, UINT16 val);
UINT16  hal_IspiRead(HAL_ISPI_CS_T cs, UINT16 reg);

// Convenience Helpers
void    hal_AbbWrite(UINT16 reg, UINT16 val);
UINT16  hal_AbbRead(UINT16 reg);
void    hal_PmuWrite(UINT16 reg, UINT16 val);
UINT16  hal_PmuRead(UINT16 reg);
void    hal_PmuSetLcdPower(BOOL on);
BOOL    hal_PmuGetLcdPower(void);

#endif // _HAL_ISPI_H_
