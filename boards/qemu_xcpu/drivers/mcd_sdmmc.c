/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * MCD (Medium Card Driver) Implementation for RDA8809
 * Complete SD / SDHC card initialization, identification handshake & block access
 */

#include "cs_types.h"
#include "global_macros.h"
#include "hal_sdmmc.h"
#include "mcd_sdmmc.h"
#include "timer.h"
#include "usb_cdc.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

#define MCD_CMD_TIMEOUT_LOOPS       100000
#define MCD_ACMD41_MAX_RETRIES      150
#define MCD_SECTOR_SIZE             512

static mcd_card_info_t g_cardInfo = {
    .cardType     = MCD_CARD_TYPE_UNKNOWN,
    .status       = MCD_STATUS_NOTPRESENT,
    .capacityMB   = 0,
    .sectorCount  = 0,
    .sectorSize   = MCD_SECTOR_SIZE,
    .rca          = 0,
    .productName  = {0},
    .serialNumber = 0,
    .manufYear    = 0,
    .manufMonth   = 0,
    .is4BitBus    = FALSE,
    .clockFreqHz  = 0
};

static BOOL g_mcdOpened = FALSE;
static BOOL g_isSdhc   = FALSE;

// =============================================================================
// =============================================================================
// Helper: Wait for command transmission to complete
// =============================================================================
static MCD_ERR_T mcd_wait_cmd_over(void)
{
    UINT32 start = timer_get_ms();
    while ((timer_get_ms() - start < 100) && !hal_SdmmcCmdDone());
    if (!hal_SdmmcCmdDone())
    {
        return MCD_ERR_CARD_TIMEOUT;
    }
    return MCD_ERR_NO;
}

// =============================================================================
// Helper: Wait for response to be received by hardware controller
// =============================================================================
static MCD_ERR_T mcd_wait_resp(void)
{
    UINT32 start = timer_get_ms();
    HAL_SDMMC_OP_STATUS_T status = hal_SdmmcGetOpStatus();
    while ((timer_get_ms() - start < 100) && status.fields.noResponseReceived)
    {
        status = hal_SdmmcGetOpStatus();
    }

    if (status.fields.noResponseReceived)
    {
        return MCD_ERR_CARD_NO_RESPONSE;
    }

    if (status.fields.responseCrcError)
    {
        return MCD_ERR_CARD_RESPONSE_BAD_CRC;
    }

    return MCD_ERR_NO;
}

// =============================================================================
// Helper: Send command and wait for completion
// =============================================================================
static MCD_ERR_T mcd_send_cmd(HAL_SDMMC_CMD_T cmd, UINT32 arg, UINT32 *resp)
{
    // If it's an application-specific command (ACMD), first send CMD55 (APP_CMD)
    if (cmd & HAL_SDMMC_ACMD_SEL)
    {
        hal_SdmmcSendCmd(HAL_SDMMC_CMD_APP_CMD, ((UINT32)g_cardInfo.rca << 16), FALSE);
        if (mcd_wait_cmd_over() != MCD_ERR_NO)
        {
            return MCD_ERR_CARD_TIMEOUT;
        }
        if (mcd_wait_resp() != MCD_ERR_NO)
        {
            return MCD_ERR_CARD_NO_RESPONSE;
        }

        UINT32 app_resp[4] = {0};
        hal_SdmmcGetResp(HAL_SDMMC_CMD_APP_CMD, app_resp, FALSE);
    }

    hal_SdmmcSendCmd(cmd, arg, FALSE);
    if (mcd_wait_cmd_over() != MCD_ERR_NO)
    {
        return MCD_ERR_CARD_TIMEOUT;
    }

    if (hal_SdmmcNeedResponse(cmd))
    {
        MCD_ERR_T r_err = mcd_wait_resp();
        if (r_err != MCD_ERR_NO)
        {
            return r_err;
        }
    }

    if (hal_SdmmcNeedResponse(cmd) && resp)
    {
        hal_SdmmcGetResp(cmd, resp, FALSE);
    }

    return MCD_ERR_NO;
}

// =============================================================================
// Helper: Wait until card is in TRAN (transmission) state (state 4)
// =============================================================================
static MCD_ERR_T mcd_wait_tran_state(UINT32 timeout_ms)
{
    UINT32 resp[4] = {0};
    UINT32 start = timer_get_ms();

    while ((timer_get_ms() - start) < timeout_ms)
    {
        if (mcd_send_cmd(HAL_SDMMC_CMD_SEND_STATUS, ((UINT32)g_cardInfo.rca << 16), resp) == MCD_ERR_NO)
        {
            // Bit 8 of Card Status: READY_FOR_DATA
            // Bits 12:9: CURRENT_STATE (4 = TRAN)
            UINT32 card_state = (resp[0] >> 9) & 0x0F;
            BOOL ready = (resp[0] & (1 << 8)) != 0;
            if (ready && (card_state == 4))
            {
                return MCD_ERR_NO;
            }
        }
    }

    return MCD_ERR_CARD_TIMEOUT;
}

// =============================================================================
// Helper: Wait until card leaves PRG (programming) busy state
// =============================================================================
static MCD_ERR_T mcd_wait_ready_for_data(void)
{
    return mcd_wait_tran_state(1000);
}

// =============================================================================
// mcd_Open - Power up SD card, execute full identification handshake, set 4-bit 25MHz
// =============================================================================
MCD_ERR_T mcd_Open(void)
{
    if (g_mcdOpened)
    {
        return MCD_ERR_NO;
    }

    os_log_printf("[MCD] ================= STARTING SD CARD INITIALIZATION =================\n");

    // 1. Hardware open: enables PMU MMC LDO (vMmcEnable=1, vMmcOff=0, vMmcIs1_8=0), clocks, pin muxing
    hal_SdmmcOpen(0x01);
    timer_delay_ms(50);

    // 2. Identification clock: 200 kHz
    hal_SdmmcSetClk(200000);
    g_cardInfo.clockFreqHz = 200000;

    // 3. Mandatory 74+ power-on clocks via continuous clock mode before CMD0
    hal_SdmmcSetClkMode(FALSE);
    timer_delay_ms(20); // 20 ms at 200 kHz = 4000 clocks (> 74)
    mcd_send_cmd(HAL_SDMMC_CMD_GO_IDLE_STATE, 0, NULL);
    hal_SdmmcSetClkMode(TRUE);  // Restore automatic clock gating
    timer_delay_ms(10);

    // Send CMD0 again to guarantee idle state
    mcd_send_cmd(HAL_SDMMC_CMD_GO_IDLE_STATE, 0, NULL);
    timer_delay_ms(10);

    // 4. CMD8 (SEND_IF_COND): Check voltage (2.7-3.6V: 0x100) and pattern (0xAA)
    UINT32 resp[4] = {0};
    BOOL is_sd_v2 = FALSE;
    MCD_ERR_T cmd8_err = mcd_send_cmd(HAL_SDMMC_CMD_SEND_IF_COND, 0x000001AA, resp);
    os_log_printf("[MCD] CMD8 result: err=%d, resp=0x%08X\n", cmd8_err, resp[0]);

    if (cmd8_err == MCD_ERR_NO && (resp[0] & 0xFFF) == 0x1AA)
    {
        is_sd_v2 = TRUE;
        os_log_printf("[MCD] Card complies with SD Physical Spec Version 2.0+ (CMD8 Echo: 0x%03X)\n",
                      resp[0] & 0xFFF);
    }
    else
    {
        is_sd_v2 = FALSE;
        os_log_printf("[MCD] Card is SD Spec V1.X or MMC\n");
    }

    // 5. ACMD41 loop: Initialize card and negotiate High Capacity
    UINT32 acmd41_arg = is_sd_v2 ? 0x40FF8000 : 0x00FF8000; // Bit 30: HCS (Host Capacity Support)
    BOOL card_ready = FALSE;
    BOOL is_mmc = FALSE;
    g_isSdhc = FALSE;

    UINT32 start_time = timer_get_ms();
    UINT32 attempts = 0;

    while ((timer_get_ms() - start_time) < 2000)
    {
        attempts++;
        MCD_ERR_T err = mcd_send_cmd(HAL_SDMMC_CMD_SEND_OP_COND, acmd41_arg, resp);
        if (err == MCD_ERR_NO)
        {
            // Bit 31: Card power up status (1 = ready)
            if (resp[0] & 0x80000000)
            {
                card_ready = TRUE;
                // Bit 30: Card Capacity Status (CCS: 1 = SDHC/SDXC, 0 = SDSC)
                if (is_sd_v2 && (resp[0] & 0x40000000))
                {
                    g_isSdhc = TRUE;
                    g_cardInfo.cardType = MCD_CARD_TYPE_SDHC_V2;
                    os_log_printf("[MCD] Card identified as SDHC / SDXC (Block Addressing, OCR=0x%08X)\n", resp[0]);
                }
                else
                {
                    g_isSdhc = FALSE;
                    g_cardInfo.cardType = is_sd_v2 ? MCD_CARD_TYPE_SDSC_V2 : MCD_CARD_TYPE_SD_V1;
                    os_log_printf("[MCD] Card identified as Standard Capacity SDSC (Byte Addressing, OCR=0x%08X)\n", resp[0]);
                }
                break;
            }
        }
        timer_delay_ms(15);
    }

    // Fallback: If ACMD41 failed, probe MMC card using CMD1
    if (!card_ready)
    {
        os_log_printf("[MCD] ACMD41 timed out, probing MMC with CMD1...\n");
        UINT32 mmc_start = timer_get_ms();
        while ((timer_get_ms() - mmc_start) < 1500)
        {
            MCD_ERR_T mmc_err = mcd_send_cmd(HAL_SDMMC_CMD_MMC_SEND_OP_COND, 0x40FF8000, resp);
            if (mmc_err == MCD_ERR_NO && (resp[0] & 0x80000000))
            {
                card_ready = TRUE;
                is_mmc = TRUE;
                g_isSdhc = (resp[0] & 0x40000000) != 0;
                g_cardInfo.cardType = MCD_CARD_TYPE_MMC;
                os_log_printf("[MCD] Card identified as MMC (OCR=0x%08X)\n", resp[0]);
                break;
            }
            timer_delay_ms(15);
        }
    }

    if (!card_ready)
    {
        os_log_printf("[MCD] Error: Card failed to power up (ACMD41/CMD1 timeout, attempts=%u, last_resp=0x%08X)!\n",
                      attempts, resp[0]);
        hal_SdmmcClose();
        g_cardInfo.status = MCD_STATUS_ERROR;
        return MCD_ERR_CARD_TIMEOUT;
    }

    // 6. CMD2 (ALL_SEND_CID): Read Card Identification Register
    if (mcd_send_cmd(HAL_SDMMC_CMD_ALL_SEND_CID, 0, resp) == MCD_ERR_NO)
    {
        // Parse Product Name (PNM)
        g_cardInfo.productName[0] = (char)(resp[3] & 0xFF);
        g_cardInfo.productName[1] = (char)((resp[2] >> 24) & 0xFF);
        g_cardInfo.productName[2] = (char)((resp[2] >> 16) & 0xFF);
        g_cardInfo.productName[3] = (char)((resp[2] >> 8)  & 0xFF);
        g_cardInfo.productName[4] = (char)(resp[2] & 0xFF);
        g_cardInfo.productName[5] = '\0';

        // Serial Number
        g_cardInfo.serialNumber = ((resp[1] & 0x00FFFFFF) << 8) | ((resp[0] >> 24) & 0xFF);

        // Manufacture Date
        UINT16 mdt = (resp[0] >> 8) & 0x0FFF;
        g_cardInfo.manufYear  = 2000 + ((mdt >> 4) & 0xFF);
        g_cardInfo.manufMonth = mdt & 0x0F;

        os_log_printf("[MCD] CID: Product Name='%s' | S/N=0x%08X | Date=%u/%u\n",
                      g_cardInfo.productName, g_cardInfo.serialNumber,
                      g_cardInfo.manufMonth, g_cardInfo.manufYear);
    }

    // 7. CMD3 (SEND_RELATIVE_ADDR): Fetch card's Relative Card Address (RCA)
    if (is_mmc)
    {
        g_cardInfo.rca = 1;
        mcd_send_cmd(HAL_SDMMC_CMD_SEND_RELATIVE_ADDR, ((UINT32)g_cardInfo.rca << 16), resp);
        os_log_printf("[MCD] MMC RCA assigned: 0x%04X\n", g_cardInfo.rca);
    }
    else
    {
        if (mcd_send_cmd(HAL_SDMMC_CMD_SEND_RELATIVE_ADDR, 0, resp) == MCD_ERR_NO)
        {
            g_cardInfo.rca = (UINT16)((resp[0] >> 16) & 0xFFFF);
            os_log_printf("[MCD] RCA assigned: 0x%04X\n", g_cardInfo.rca);
        }
        else
        {
            os_log_printf("[MCD] Error fetching RCA (CMD3 failed)!\n");
            hal_SdmmcClose();
            g_cardInfo.status = MCD_STATUS_ERROR;
            return MCD_ERR_INIT_FAILED;
        }
    }

    // 8. CMD9 (SEND_CSD): Read Card Specific Data and calculate capacity
    if (mcd_send_cmd(HAL_SDMMC_CMD_SEND_CSD, ((UINT32)g_cardInfo.rca << 16), resp) == MCD_ERR_NO)
    {
        UINT8 csd_ver = (UINT8)((resp[3] >> 30) & 0x03);
        if (csd_ver == 1)
        {
            // CSD Version 2.0 (High Capacity)
            UINT32 c_size = ((resp[2] & 0x3F) << 16) | ((resp[1] >> 16) & 0xFFFF);
            g_cardInfo.capacityMB  = (c_size + 1) / 2;
            g_cardInfo.sectorCount = (c_size + 1) * 1024;
        }
        else
        {
            // CSD Version 1.0 (Standard Capacity)
            UINT32 c_size      = ((resp[2] & 0x3FF) << 2) | ((resp[1] >> 30) & 0x03);
            UINT8  c_size_mult = (UINT8)((resp[1] >> 15) & 0x07);
            UINT8  read_bl_len = (UINT8)((resp[2] >> 16) & 0x0F);
            UINT32 mult        = 1 << (c_size_mult + 2);
            UINT32 block_nr    = (c_size + 1) * mult;
            UINT32 block_len   = 1 << read_bl_len;
            g_cardInfo.sectorCount = (block_nr * (block_len / 512));
            g_cardInfo.capacityMB  = (g_cardInfo.sectorCount * 512) / (1024 * 1024);
        }

        os_log_printf("[MCD] CSD v%u: Capacity = %u MB (%u sectors)\n",
                      csd_ver + 1, g_cardInfo.capacityMB, g_cardInfo.sectorCount);
    }

    // 9. CMD7 (SELECT_CARD): Select card to enter TRANSFER state
    if (mcd_send_cmd(HAL_SDMMC_CMD_SELECT_CARD, ((UINT32)g_cardInfo.rca << 16), resp) != MCD_ERR_NO)
    {
        os_log_printf("[MCD] Error selecting card (CMD7 failed)!\n");
        hal_SdmmcClose();
        g_cardInfo.status = MCD_STATUS_ERROR;
        return MCD_ERR_INIT_FAILED;
    }
    os_log_printf("[MCD] Card selected -> TRAN state active.\n");

    // 10. CMD16 (SET_BLOCKLEN): Configure 512-byte block size
    mcd_send_cmd(HAL_SDMMC_CMD_SET_BLOCKLEN, MCD_SECTOR_SIZE, resp);

    // 11. ACMD6 (SET_BUS_WIDTH): Switch to 4-bit bus width (SD cards only)
    if (!is_mmc && mcd_send_cmd(HAL_SDMMC_CMD_SET_BUS_WIDTH, 0x02, resp) == MCD_ERR_NO)
    {
        hal_SdmmcSetDataWidth(HAL_SDMMC_DATA_BUS_WIDTH_4);
        g_cardInfo.is4BitBus = TRUE;
    }
    else
    {
        hal_SdmmcSetDataWidth(HAL_SDMMC_DATA_BUS_WIDTH_1);
        g_cardInfo.is4BitBus = FALSE;
    }

    // 12. Switch to high speed data transfer mode
    hal_SdmmcEnterDataTransferMode();
    hal_SdmmcSetClk(g_isSdhc ? 25000000 : 20000000);
    g_cardInfo.clockFreqHz = g_isSdhc ? 25000000 : 20000000;

    g_mcdOpened = TRUE;
    g_cardInfo.status = MCD_STATUS_OPEN;

    os_log_printf("[MCD] SD card successfully initialized and ready for data transfers!\n");
    os_log_printf("[MCD] ==================================================================\n");

    return MCD_ERR_NO;
}

// =============================================================================
// mcd_Close - Power down SD card interface
// =============================================================================
void mcd_Close(void)
{
    if (!g_mcdOpened) return;

    hal_SdmmcClose();
    g_mcdOpened = FALSE;
    g_cardInfo.status = MCD_STATUS_NOTPRESENT;

    os_log_printf("[MCD] SD Card interface closed.\n");
}

BOOL mcd_IsOpen(void)
{
    return g_mcdOpened;
}

// Dedicated 4-byte aligned DMA bounce buffer (512 bytes)
static UINT32 g_mcdDmaBuffer[MCD_SECTOR_SIZE / 4] __attribute__((aligned(4)));

// MIPS L1 D-Cache Invalidation Helper (official RDA SDK method)
static inline void hal_SysInvalidateCache(void *buffer, UINT32 size)
{
    // Force uncached KSEG1 read into cached buffer to establish coherency
    memcpy(buffer, (const void*)((UINT32)buffer | 0x20000000), size);
}

// =============================================================================
// mcd_Read - Read 512-byte sectors from SD card using DMA IFC
// =============================================================================
MCD_ERR_T mcd_Read(UINT32 sector, UINT8 *buf, UINT32 count)
{
    if (!g_mcdOpened || !buf || count == 0) return MCD_ERR_PARAM;

    UINT32 card_addr = g_isSdhc ? sector : (sector * MCD_SECTOR_SIZE);

    if (count == 1)
    {
        HAL_SDMMC_TRANSFER_T transfer = {
            .sysMemAddr = (UINT8*)g_mcdDmaBuffer,
            .sdCardAddr = (UINT8*)card_addr,
            .blockNum   = 1,
            .blockSize  = MCD_SECTOR_SIZE,
            .direction  = HAL_SDMMC_DIRECTION_READ
        };

        if (!hal_SdmmcTransfer(&transfer))
        {
            os_log_printf("[MCD] hal_SdmmcTransfer failed for sector %u!\n", sector);
            return MCD_ERR_READ_FAILED;
        }

        UINT32 resp[4] = {0};
        if (mcd_send_cmd(HAL_SDMMC_CMD_READ_SINGLE_BLOCK, card_addr, resp) != MCD_ERR_NO)
        {
            os_log_printf("[MCD] CMD17 failed for sector %u (addr=0x%08X)!\n", sector, card_addr);
            hal_SdmmcStopTransfer(&transfer);
            return MCD_ERR_READ_FAILED;
        }

        UINT32 start = timer_get_ms();
        while (!hal_SdmmcTransferDone())
        {
            if ((timer_get_ms() - start) > 500)
            {
                os_log_printf("[MCD] Read DMA timeout on sector %u!\n", sector);
                hal_SdmmcStopTransfer(&transfer);
                return MCD_ERR_CARD_TIMEOUT;
            }
        }

        HAL_SDMMC_OP_STATUS_T op = hal_SdmmcGetOpStatus();
        if (op.fields.dataError != 0)
        {
            os_log_printf("[MCD] Read data CRC error on sector %u (op=0x%08X)!\n", sector, op.reg);
            return MCD_ERR_CARD_RESPONSE_BAD_CRC;
        }

        // Copy directly from UNCACHED alias of DMA buffer into target buffer
        // This ensures the CPU gets the fresh data from RAM, bypassing stale L1 cache
        memcpy(buf, (const void*)((UINT32)g_mcdDmaBuffer | 0x20000000), MCD_SECTOR_SIZE);

        return MCD_ERR_NO;
    }
    else
    {
        // For multiple blocks: if buf is 4-byte aligned, transfer directly
        if (((UINT32)buf & 3) == 0)
        {
            HAL_SDMMC_TRANSFER_T transfer = {
                .sysMemAddr = buf,
                .sdCardAddr = (UINT8*)card_addr,
                .blockNum   = count,
                .blockSize  = MCD_SECTOR_SIZE,
                .direction  = HAL_SDMMC_DIRECTION_READ
            };

            if (!hal_SdmmcTransfer(&transfer))
            {
                return MCD_ERR_READ_FAILED;
            }

            UINT32 resp[4] = {0};
            if (mcd_send_cmd(HAL_SDMMC_CMD_READ_MULT_BLOCK, card_addr, resp) != MCD_ERR_NO)
            {
                hal_SdmmcStopTransfer(&transfer);
                return MCD_ERR_READ_FAILED;
            }

            UINT32 start = timer_get_ms();
            while (!hal_SdmmcTransferDone())
            {
                if ((timer_get_ms() - start) > (500 * count))
                {
                    os_log_printf("[MCD] Read multi-DMA timeout (sec=%u, cnt=%u)!\n", sector, count);
                    hal_SdmmcStopTransfer(&transfer);
                    return MCD_ERR_CARD_TIMEOUT;
                }
            }

            HAL_SDMMC_OP_STATUS_T op = hal_SdmmcGetOpStatus();
            if (op.fields.dataError != 0)
            {
                os_log_printf("[MCD] Read multi CRC error (sec=%u, op=0x%08X)!\n", sector, op.reg);
                return MCD_ERR_CARD_RESPONSE_BAD_CRC;
            }

            // Invalidate CPU cache for the destination buffer
            hal_SysInvalidateCache(buf, count * MCD_SECTOR_SIZE);

            return MCD_ERR_NO;
        }
        else
        {
            // Unaligned buffer: read sector by sector through aligned bounce buffer
            for (UINT32 i = 0; i < count; i++)
            {
                MCD_ERR_T r = mcd_Read(sector + i, buf + (i * MCD_SECTOR_SIZE), 1);
                if (r != MCD_ERR_NO) return r;
            }
            return MCD_ERR_NO;
        }
    }
}

// =============================================================================
// mcd_Write - Write 512-byte sectors to SD card using DMA IFC
// =============================================================================
MCD_ERR_T mcd_Write(UINT32 sector, const UINT8 *buf, UINT32 count)
{
    if (!g_mcdOpened || !buf || count == 0) return MCD_ERR_PARAM;

    UINT32 card_addr = g_isSdhc ? sector : (sector * MCD_SECTOR_SIZE);

    if (count == 1)
    {
        // Copy to UNCACHED alias of DMA buffer so DMA controller reads fresh data from RAM
        memcpy((void*)((UINT32)g_mcdDmaBuffer | 0x20000000), buf, MCD_SECTOR_SIZE);

        HAL_SDMMC_TRANSFER_T transfer = {
            .sysMemAddr = (UINT8*)g_mcdDmaBuffer,
            .sdCardAddr = (UINT8*)card_addr,
            .blockNum   = 1,
            .blockSize  = MCD_SECTOR_SIZE,
            .direction  = HAL_SDMMC_DIRECTION_WRITE
        };

        if (!hal_SdmmcTransfer(&transfer))
        {
            return MCD_ERR_WRITE_FAILED;
        }

        UINT32 resp[4] = {0};
        if (mcd_send_cmd(HAL_SDMMC_CMD_WRITE_SINGLE_BLOCK, card_addr, resp) != MCD_ERR_NO)
        {
            hal_SdmmcStopTransfer(&transfer);
            return MCD_ERR_WRITE_FAILED;
        }

        UINT32 start = timer_get_ms();
        while (!hal_SdmmcTransferDone())
        {
            if ((timer_get_ms() - start) > 500)
            {
                os_log_printf("[MCD] Write DMA timeout on sector %u!\n", sector);
                hal_SdmmcStopTransfer(&transfer);
                return MCD_ERR_CARD_TIMEOUT;
            }
        }

        HAL_SDMMC_OP_STATUS_T op = hal_SdmmcGetOpStatus();
        if (op.fields.crcStatus != 2) // 2 = transmission OK in hardware
        {
            os_log_printf("[MCD] Write CRC status error on sector %u (crcStatus=%u)!\n", sector, op.fields.crcStatus);
            return MCD_ERR_CARD_RESPONSE_BAD_CRC;
        }

        return mcd_wait_ready_for_data();
    }
    else
    {
        for (UINT32 i = 0; i < count; i++)
        {
            MCD_ERR_T w = mcd_Write(sector + i, buf + (i * MCD_SECTOR_SIZE), 1);
            if (w != MCD_ERR_NO) return w;
        }
        return MCD_ERR_NO;
    }
}

BOOL mcd_GetCardInfo(mcd_card_info_t *info)
{
    if (info)
    {
        *info = g_cardInfo;
    }
    return g_mcdOpened;
}
