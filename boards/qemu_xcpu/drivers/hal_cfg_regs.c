/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Configuration Registers (cfg_regs) & Pin Mux Implementation for RDA8809
 */

#include "cs_types.h"
#include "global_macros.h"
#include "cfg_regs.h"
#include "hal_cfg_regs.h"

extern void os_log_printf(const char *fmt, ...);

static UINT32 g_chipId = 0;
static UINT32 g_buildVersion = 0;

// =============================================================================
// hal_CfgRegsInit - Query chip ID, build version & log details
// =============================================================================
void hal_CfgRegsInit(void)
{
    g_chipId = hwp_configRegs->CHIP_ID;
    g_buildVersion = hwp_configRegs->Build_Version;

    os_log_printf("[CFG_REGS] Module ready at 0x%08X (ChipID: 0x%08X, BuildVer: 0x%08X, Mux: 0x%08X)\n",
                  REG_CONFIG_REGS_BASE, g_chipId, g_buildVersion, hwp_configRegs->Alt_mux_select);
}

// =============================================================================
// Hardware Queries
// =============================================================================
UINT32 hal_CfgGetChipId(void)
{
    return hwp_configRegs->CHIP_ID;
}

UINT32 hal_CfgGetBuildVersion(void)
{
    return hwp_configRegs->Build_Version;
}

BOOL hal_CfgIsRda8809(void)
{
    return (hwp_configRegs->CHIP_ID == RDA8809_EXPECTED_CHIP_ID);
}

UINT32 hal_CfgGetAltMux(void)
{
    return hwp_configRegs->Alt_mux_select;
}

// =============================================================================
// Pin Multiplexing Controls
// =============================================================================
void hal_CfgSetPinMux(HAL_PIN_MUX_T mux, BOOL enable)
{
    UINT32 val = hwp_configRegs->Alt_mux_select;

    switch (mux)
    {
        case HAL_PIN_MUX_SDMMC_MODE:
            if (enable)
                val &= ~CFG_REGS_SDMMC_MASK; // 0 = SDMMC
            else
                val |= CFG_REGS_SDMMC_MASK;  // 1 = DigRF
            break;

        case HAL_PIN_MUX_DIGRF_MODE:
            if (enable)
                val |= CFG_REGS_SDMMC_MASK;
            else
                val &= ~CFG_REGS_SDMMC_MASK;
            break;

        case HAL_PIN_MUX_I2C2_MODE:
            if (enable)
                val &= ~CFG_REGS_I2C2_MASK;  // 0 = I2C2
            else
                val |= CFG_REGS_I2C2_MASK;   // 1 = USB Backup
            break;

        case HAL_PIN_MUX_USB_BACKUP_MODE:
            if (enable)
                val |= CFG_REGS_I2C2_MASK;
            else
                val &= ~CFG_REGS_I2C2_MASK;
            break;

        case HAL_PIN_MUX_I2C3_ON_SPI1:
            if (enable)
                val |= CFG_REGS_SPI1_SELECT_I2C_3;
            else
                val &= ~CFG_REGS_SPI1_SELECT_I2C_3;
            break;
    }

    hwp_configRegs->Alt_mux_select = val;
}

// =============================================================================
// I/O Drive Strengths
// =============================================================================
void hal_CfgSetIoDriveStrength(HAL_IO_DRIVE_DOMAIN_T domain, UINT8 strength)
{
    UINT32 d1 = hwp_configRegs->IO_Drive1_Select;

    switch (domain)
    {
        case HAL_IO_DRIVE_SDMMC:
            d1 = (d1 & ~CFG_REGS_SDMMC_CTRL_DRIVE_MASK) | ((strength & 3) << 17);
            break;
        default:
            break;
    }

    hwp_configRegs->IO_Drive1_Select = d1;
}

// =============================================================================
// Integrated FM Receiver Power Control
// =============================================================================
void hal_CfgSetFmPower(BOOL enable)
{
    if (enable)
    {
        hwp_configRegs->Alt_mux_select |= CFG_REGS_FM_POWER_ON;
    }
    else
    {
        hwp_configRegs->Alt_mux_select &= ~CFG_REGS_FM_POWER_ON;
    }
}
