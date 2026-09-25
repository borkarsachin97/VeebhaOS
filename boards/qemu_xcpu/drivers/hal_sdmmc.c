/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * HAL SDMMC Driver Implementation for RDA8809
 * Controls hardware SDMMC interface (0x01A17000) & SYS IFC DMA (0x01A09000)
 */

#include "cs_types.h"
#include "global_macros.h"
#include "cfg_regs.h"
#include "sys_ctrl.h"
#include "sdmmc.h"
#include "sys_ifc.h"
#include "hal_sdmmc.h"
#include "hal_ispi.h"
#include "hal_pmd.h"
#include "timer.h"
#include "usb_cdc.h"

#define HAL_UNKNOWN_CHANNEL         0xFF
#define SDMMC_CMD_TIMEOUT_CYCLES    200000
#define SDMMC_XFER_TIMEOUT_CYCLES   5000000

static UINT8 g_sdmmcReadCh  = HAL_UNKNOWN_CHANNEL;
static UINT8 g_sdmmcWriteCh = HAL_UNKNOWN_CHANNEL;

// =============================================================================
// hal_SdmmcOpen - Enable power, clocks, and configure pin muxing for SDMMC
// =============================================================================
void hal_SdmmcOpen(UINT8 clk_adj)
{
    os_log_printf("[SDMMC_HAL] Initializing SDMMC controller and power rails...\n");

    // 1. Power on PMU MMC LDO:
    hal_PmdSetMmcPower(TRUE);

    // Reg 0x02: bit 11 = vMmcEnable
    UINT16 reg02 = hal_PmuRead(PMU_REG_LDO_SETTINGS);
    reg02 |= (1 << 11);
    hal_PmuWrite(PMU_REG_LDO_SETTINGS, reg02);

    // Reg 0x03: bit 6 = vMmcOff. Must be CLEARED (0) so MMC LDO powers up in active mode!
    UINT16 reg03 = hal_PmuRead(PMU_REG_LDO_ACTIVE1);
    reg03 &= ~(1 << 6);
    hal_PmuWrite(PMU_REG_LDO_ACTIVE1, reg03);

    // Reg 0x04: bit 8 = vMmcIs1_8. Must be CLEARED (0) for 2.8V / 3.0V output!
    UINT16 reg04 = hal_PmuRead(PMU_REG_LDO_ACTIVE2);
    reg04 &= ~(1 << 8);
    hal_PmuWrite(PMU_REG_LDO_ACTIVE2, reg04);

    // Reg 0x07: bits [5:3] = vMmcIbit. Set to 4 (nominal drive strength)
    UINT16 reg07 = hal_PmuRead(PMU_REG_LDO_ACTIVE5);
    reg07 = (reg07 & ~(0x7 << 3)) | (4 << 3);
    hal_PmuWrite(PMU_REG_LDO_ACTIVE5, reg07);

    // Reg 0x08: bit 6 = vMmcOff in Low Power mode. Clear to avoid shutoff in LP
    UINT16 reg08 = hal_PmuRead(PMU_REG_LDO_LP1);
    reg08 &= ~(1 << 6);
    hal_PmuWrite(PMU_REG_LDO_LP1, reg08);

    os_log_printf("[SDMMC_HAL] PMU LDO: Reg02=0x%04X, Reg03=0x%04X, Reg04=0x%04X, Reg07=0x%04X\n",
                  hal_PmuRead(PMU_REG_LDO_SETTINGS),
                  hal_PmuRead(PMU_REG_LDO_ACTIVE1),
                  hal_PmuRead(PMU_REG_LDO_ACTIVE2),
                  hal_PmuRead(PMU_REG_LDO_ACTIVE5));

    // 2. Peripheral Clock & Reset for SDMMC
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_UNLOCK;
    hwp_sysCtrl->Clk_Per_Enable |= SYS_CTRL_ENABLE_PER_SDMMC;
    hwp_sysCtrl->Clk_Per_Mode   &= ~SYS_CTRL_MODE_PER_SDMMC_MANUAL;
    hwp_sysCtrl->Sys_Rst_Clr     = SYS_CTRL_CLR_RST_SDMMC;
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_LOCK;

    // 3. Pin Multiplexing: Route pins to standard SDMMC interface (bit 10 = 0)
    hwp_configRegs->Alt_mux_select &= ~CFG_REGS_SDMMC_MASK;
    // Set fast drive strength for SDMMC signals
    hwp_configRegs->IO_Drive1_Select &= ~CFG_REGS_SDMMC_CTRL_DRIVE_MASK;

    // 4. Configure SDMMC controller
    hwp_sdmmc->SDMMC_INT_MASK    = 0x0;
    hwp_sdmmc->SDMMC_MCLK_ADJUST = (clk_adj & 0x0F) | SDMMC_CLK_INV;

    // Default to 1-bit bus width until negotiated
    hwp_sdmmc->SDMMC_DATA_WIDTH = 1;

    os_log_printf("[SDMMC_HAL] SDMMC hardware ready (MMC LDO powered, pin mux configured).\n");
}

// =============================================================================
// hal_SdmmcClose - Power down SDMMC controller and cut MMC LDO
// =============================================================================
void hal_SdmmcClose(void)
{
    // 1. Put SDMMC in reset and disable peripheral clock
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_UNLOCK;
    hwp_sysCtrl->Sys_Rst_Set     = SYS_CTRL_SET_RST_SDMMC;
    hwp_sysCtrl->Clk_Per_Disable = SYS_CTRL_DISABLE_PER_SDMMC;
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_LOCK;

    // 2. Power off PMU MMC LDO (set vMmcOff = 1 in Reg 0x03, clear vMmcEnable in Reg 0x02)
    UINT16 reg03 = hal_PmuRead(PMU_REG_LDO_ACTIVE1);
    reg03 |= (1 << 6);
    hal_PmuWrite(PMU_REG_LDO_ACTIVE1, reg03);

    UINT16 reg02 = hal_PmuRead(PMU_REG_LDO_SETTINGS);
    reg02 &= ~(1 << 11);
    hal_PmuWrite(PMU_REG_LDO_SETTINGS, reg02);

    g_sdmmcReadCh  = HAL_UNKNOWN_CHANNEL;
    g_sdmmcWriteCh = HAL_UNKNOWN_CHANNEL;

    os_log_printf("[SDMMC_HAL] SDMMC powered down.\n");
}

// =============================================================================
// hal_SdmmcSetClkMode - Set continuous (manual) or gated (auto) clock mode
// =============================================================================
void hal_SdmmcSetClkMode(BOOL auto_clk)
{
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_UNLOCK;
    UINT32 value = hwp_sysCtrl->Clk_Per_Mode;
    hwp_sysCtrl->Clk_Per_Mode = auto_clk ?
        (value & ~SYS_CTRL_MODE_PER_SDMMC_MANUAL) : (value | SYS_CTRL_MODE_PER_SDMMC_MANUAL);
    hwp_sysCtrl->Clk_Per_Enable = SYS_CTRL_ENABLE_PER_SDMMC;
    hwp_sysCtrl->Sys_Rst_Clr     = SYS_CTRL_CLR_RST_SDMMC;
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_LOCK;
}

// =============================================================================
// hal_SdmmcEnterDataTransferMode - Enable RD_WT_EN for data phases
// =============================================================================
void hal_SdmmcEnterDataTransferMode(void)
{
    hwp_sdmmc->SDMMC_CONFIG |= SDMMC_RD_WT_EN;
}

// =============================================================================
// hal_SdmmcGetOpStatus - Return full operational status of SDMMC controller
// =============================================================================
HAL_SDMMC_OP_STATUS_T hal_SdmmcGetOpStatus(void)
{
    HAL_SDMMC_OP_STATUS_T status;
    status.reg = hwp_sdmmc->SDMMC_STATUS;
    return status;
}

// =============================================================================
// hal_SdmmcSetClk - Configure SDMMC clock speed (from 52 MHz system clock)
// =============================================================================
void hal_SdmmcSetClk(UINT32 clock)
{
    if (clock == 0) return;

    UINT32 sysFreq = 52000000;
    UINT32 divider = (sysFreq - 1) / (2 * clock);
    if (divider > 0xFF) divider = 0xFF;

    hwp_sdmmc->SDMMC_TRANS_SPEED = SDMMC_SDMMC_TRANS_SPEED(divider);

    os_log_printf("[SDMMC_HAL] Clock set to ~%u Hz (divider=%u)\n",
                  sysFreq / (2 * (divider + 1)), divider);
}

// =============================================================================
// hal_SdmmcSendCmd - Send an SD command via hardware controller
// =============================================================================
void hal_SdmmcSendCmd(HAL_SDMMC_CMD_T cmd, UINT32 arg, BOOL suspend)
{
    (void)suspend;
    UINT32 configReg = 0;

    switch (cmd)
    {
        case HAL_SDMMC_CMD_GO_IDLE_STATE:
        case HAL_SDMMC_CMD_SET_DSR:
            configReg = SDMMC_SDMMC_SENDCMD;
            break;

        case HAL_SDMMC_CMD_ALL_SEND_CID:
        case HAL_SDMMC_CMD_SEND_CSD:
            configReg = SDMMC_RSP_SEL_R2 | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
            break;

        case HAL_SDMMC_CMD_MMC_SEND_OP_COND:
        case HAL_SDMMC_CMD_SEND_OP_COND:
            configReg = SDMMC_RSP_SEL_R3 | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
            break;

        case HAL_SDMMC_CMD_READ_SINGLE_BLOCK:
            configReg = SDMMC_RD_WT_SEL_READ | SDMMC_RD_WT_EN | SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
            break;

        case HAL_SDMMC_CMD_READ_MULT_BLOCK:
            configReg = SDMMC_S_M_SEL_MULTIPLE | SDMMC_RD_WT_SEL_READ | SDMMC_RD_WT_EN | SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
            break;

        case HAL_SDMMC_CMD_WRITE_SINGLE_BLOCK:
            configReg = SDMMC_RD_WT_SEL_WRITE | SDMMC_RD_WT_EN | SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
            break;

        case HAL_SDMMC_CMD_WRITE_MULT_BLOCK:
            configReg = SDMMC_S_M_SEL_MULTIPLE | SDMMC_RD_WT_SEL_WRITE | SDMMC_RD_WT_EN | SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
            break;

        default:
            // Standard R1 / R6 / R7 response commands
            configReg = SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
            break;
    }

    hwp_sdmmc->SDMMC_CMD_INDEX = SDMMC_COMMAND(cmd & HAL_SDMMC_CMD_MASK);
    hwp_sdmmc->SDMMC_CMD_ARG   = SDMMC_ARGUMENT(arg);
    hwp_sdmmc->SDMMC_CONFIG    = configReg | SDMMC_AUTO_FLAG_EN;
}

BOOL hal_SdmmcCmdDone(void)
{
    return !(hwp_sdmmc->SDMMC_STATUS & SDMMC_NOT_SDMMC_OVER);
}

BOOL hal_SdmmcNeedResponse(HAL_SDMMC_CMD_T cmd)
{
    switch (cmd)
    {
        case HAL_SDMMC_CMD_GO_IDLE_STATE:
        case HAL_SDMMC_CMD_SET_DSR:
        case HAL_SDMMC_CMD_STOP_TRANSMISSION:
            return FALSE;
        default:
            return TRUE;
    }
}

void hal_SdmmcGetResp(HAL_SDMMC_CMD_T cmd, UINT32 *arg, BOOL suspend)
{
    (void)suspend;
    if (!arg) return;

    if (cmd == HAL_SDMMC_CMD_ALL_SEND_CID || cmd == HAL_SDMMC_CMD_SEND_CSD)
    {
        // 128-bit response (R2)
        arg[0] = hwp_sdmmc->SDMMC_RESP_ARG0;
        arg[1] = hwp_sdmmc->SDMMC_RESP_ARG1;
        arg[2] = hwp_sdmmc->SDMMC_RESP_ARG2;
        arg[3] = hwp_sdmmc->SDMMC_RESP_ARG3;
    }
    else
    {
        // 32-bit response (R1, R3, R6, R7)
        arg[0] = hwp_sdmmc->SDMMC_RESP_ARG3;
        arg[1] = 0;
        arg[2] = 0;
        arg[3] = 0;
    }
}

void hal_SdmmcSetDataWidth(HAL_SDMMC_DATA_BUS_WIDTH_T width)
{
    if (width == HAL_SDMMC_DATA_BUS_WIDTH_4)
    {
        hwp_sdmmc->SDMMC_DATA_WIDTH = 4;
        os_log_printf("[SDMMC_HAL] Bus width switched to 4-bit.\n");
    }
    else
    {
        hwp_sdmmc->SDMMC_DATA_WIDTH = 1;
        os_log_printf("[SDMMC_HAL] Bus width switched to 1-bit.\n");
    }
}

// =============================================================================
// hal_SdmmcTransfer - Initiate SYS IFC DMA transfer for SDMMC
// =============================================================================
BOOL hal_SdmmcTransfer(HAL_SDMMC_TRANSFER_T *transfer)
{
    if (!transfer || transfer->blockNum == 0) return FALSE;

    UINT32 length = transfer->blockSize;
    UINT32 lengthExp = 0;
    while (length > 1)
    {
        length >>= 1;
        lengthExp++;
    }

    // Clear any pending interrupt flags before transfer
    hwp_sdmmc->SDMMC_INT_CLEAR  = 0xFFFFFFFF;

    // Configure SDMMC block count and size
    hwp_sdmmc->SDMMC_BLOCK_CNT  = SDMMC_SDMMC_BLOCK_CNT(transfer->blockNum);
    hwp_sdmmc->SDMMC_BLOCK_SIZE = SDMMC_SDMMC_BLOCK_SIZE(lengthExp);
    hwp_sdmmc->apbi_ctrl_sdmmc  = SDMMC_SOFT_RST_L | SDMMC_L_ENDIAN(1);

    // Allocate an IFC channel
    UINT8 ch = SYS_IFC_CH_TO_USE(hwp_sysIfc->get_ch);
    if (ch >= SYS_IFC_STD_CHAN_NB)
    {
        os_log_printf("[SDMMC_HAL] Error: No available IFC channel!\n");
        return FALSE;
    }

    UINT32 req = (transfer->direction == HAL_SDMMC_DIRECTION_READ) ?
                 SYS_IFC_REQ_SRC_RX_SDMMC : SYS_IFC_REQ_SRC_TX_SDMMC;

    hwp_sysIfc->std_ch[ch].start_addr = (UINT32)transfer->sysMemAddr;
    hwp_sysIfc->std_ch[ch].tc         = transfer->blockNum * transfer->blockSize;
    hwp_sysIfc->std_ch[ch].control    = req | SYS_IFC_SIZE | SYS_IFC_CH_RD_HW_EXCH | SYS_IFC_ENABLE;

    if (transfer->direction == HAL_SDMMC_DIRECTION_READ)
    {
        g_sdmmcReadCh = ch;
    }
    else
    {
        g_sdmmcWriteCh = ch;
    }

    return TRUE;
}

// =============================================================================
// hal_SdmmcTransferDone - Check if DMA IFC transfer has completed
// =============================================================================
BOOL hal_SdmmcTransferDone(void)
{
    if (g_sdmmcReadCh != HAL_UNKNOWN_CHANNEL)
    {
        if ((hwp_sdmmc->SDMMC_INT_STATUS & SDMMC_DAT_OVER_INT) &&
            (hwp_sysIfc->std_ch[g_sdmmcReadCh].tc == 0))
        {
            hwp_sdmmc->SDMMC_INT_CLEAR = SDMMC_DAT_OVER_CL;
            hwp_sysIfc->std_ch[g_sdmmcReadCh].control |= SYS_IFC_DISABLE;
            g_sdmmcReadCh = HAL_UNKNOWN_CHANNEL;
            hwp_sdmmc->apbi_ctrl_sdmmc = SDMMC_L_ENDIAN(1);
            return TRUE;
        }
    }

    if (g_sdmmcWriteCh != HAL_UNKNOWN_CHANNEL)
    {
        if ((hwp_sdmmc->SDMMC_INT_STATUS & SDMMC_DAT_OVER_INT) &&
            (hwp_sysIfc->std_ch[g_sdmmcWriteCh].tc == 0))
        {
            hwp_sdmmc->SDMMC_INT_CLEAR = SDMMC_DAT_OVER_CL;
            hwp_sysIfc->std_ch[g_sdmmcWriteCh].control |= SYS_IFC_DISABLE;
            g_sdmmcWriteCh = HAL_UNKNOWN_CHANNEL;
            hwp_sdmmc->apbi_ctrl_sdmmc = SDMMC_L_ENDIAN(1);
            return TRUE;
        }
    }

    return FALSE;
}

void hal_SdmmcStopTransfer(HAL_SDMMC_TRANSFER_T *transfer)
{
    (void)transfer;
    hwp_sdmmc->SDMMC_BLOCK_CNT  = 0;
    hwp_sdmmc->SDMMC_BLOCK_SIZE = 0;
    hwp_sdmmc->apbi_ctrl_sdmmc  = 0 | SDMMC_L_ENDIAN(1);
    hwp_sdmmc->SDMMC_INT_CLEAR  = 0xFFFFFFFF;

    if (g_sdmmcReadCh != HAL_UNKNOWN_CHANNEL)
    {
        hwp_sysIfc->std_ch[g_sdmmcReadCh].control |= SYS_IFC_DISABLE;
        g_sdmmcReadCh = HAL_UNKNOWN_CHANNEL;
    }
    if (g_sdmmcWriteCh != HAL_UNKNOWN_CHANNEL)
    {
        hwp_sysIfc->std_ch[g_sdmmcWriteCh].control |= SYS_IFC_DISABLE;
        g_sdmmcWriteCh = HAL_UNKNOWN_CHANNEL;
    }
}
