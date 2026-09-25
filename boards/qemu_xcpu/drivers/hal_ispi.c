/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Internal SPI (ISPI) Master Driver for RDA8809 / RDA8955 SoC
 * Connected to SPI3 controller (0x01A13000)
 */

#include "cs_types.h"
#include "global_macros.h"
#include "sys_ctrl.h"
#include "spi.h"
#include "hal_ispi.h"
#include "timer.h"

#define REG_SPI3_BASE   0x01A13000
#define hwp_spi3        ((HWP_SPI_T*) KSEG1(REG_SPI3_BASE))

#define ISPI_TIMEOUT_COUNT  100000

// ============================================================================
// hal_IspiInit - enable SPI3 clock in sysCtrl and configure ISPI controller
// ============================================================================
void hal_IspiInit(void)
{
    static BOOL is_init = FALSE;
    if (is_init) return;

    // 1. Enable SPI3 peripheral clock and clear SPI3 reset in sysCtrl
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_UNLOCK;
    hwp_sysCtrl->Clk_Per_Enable |= SYS_CTRL_ENABLE_PERD_SPI3; // SPI3 peripheral clock (1 << 10)
    hwp_sysCtrl->Sys_Rst_Clr = SYS_CTRL_CLR_RST_SPI3;
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_LOCK;

    // 2. Configure SPI3 controller for ISPI internal bus:
    // Frame size 32 bits, continuous CS, clock divider = 4 (~13 MHz)
    hwp_spi3->ctrl        = 0x2019D821;
    hwp_spi3->cfg         = 0x00030007;
    hwp_spi3->pin_control = 0x00000000;
    hwp_spi3->irq         = 0x00000000;

    // 3. Clear FIFOs
    hwp_spi3->status = SPI_FIFO_FLUSH;

    is_init = TRUE;
}

// ============================================================================
// hal_IspiWrite - write a 16-bit value to register on specified CS
// ============================================================================
void hal_IspiWrite(HAL_ISPI_CS_T cs, UINT16 reg, UINT16 val)
{
    hal_IspiInit();

    // Command format: [31]=0 (Write), [30:29]=CS, [25]=0, [24:16]=Address, [15:0]=Data
    UINT32 cmd = SPI_CS(cs) | (((UINT32)reg & 0x1FF) << 16) | (val & 0xFFFF);

    // Wait for TX space with safety timeout
    volatile UINT32 timeout = ISPI_TIMEOUT_COUNT;
    while (GET_BITFIELD(hwp_spi3->status, SPI_TX_SPACE) == 0 && --timeout)
        ;

    // Push command into TX FIFO
    hwp_spi3->rxtx_buffer = cmd;

    // Wait until TX is complete with safety timeout
    timeout = ISPI_TIMEOUT_COUNT;
    while (--timeout)
    {
        UINT32 status = hwp_spi3->status;
        if ((status & SPI_ACTIVE_STATUS) == 0 &&
            GET_BITFIELD(status, SPI_TX_SPACE) == SPI_TX_FIFO_SIZE)
        {
            break;
        }
    }
}

// ============================================================================
// hal_IspiRead - read a 16-bit register from the specified CS
// ============================================================================
UINT16 hal_IspiRead(HAL_ISPI_CS_T cs, UINT16 reg)
{
    hal_IspiInit();

    // Command format: [31]=ReadEnable, [30:29]=CS, [25]=1, [24:16]=Address
    UINT32 cmd = SPI_READ_ENA | SPI_CS(cs) | (1 << 25) | (((UINT32)reg & 0x1FF) << 16);

    // Wait for TX space with safety timeout
    volatile UINT32 timeout = ISPI_TIMEOUT_COUNT;
    while (GET_BITFIELD(hwp_spi3->status, SPI_TX_SPACE) == 0 && --timeout)
        ;

    // Send read request
    hwp_spi3->rxtx_buffer = cmd;

    // Wait until TX is complete and RX FIFO has data with safety timeout
    timeout = ISPI_TIMEOUT_COUNT;
    while (--timeout)
    {
        UINT32 status = hwp_spi3->status;
        if ((status & SPI_ACTIVE_STATUS) == 0 &&
            GET_BITFIELD(status, SPI_TX_SPACE) == SPI_TX_FIFO_SIZE &&
            GET_BITFIELD(status, SPI_RX_LEVEL) > 0)
        {
            break;
        }
    }

    if (GET_BITFIELD(hwp_spi3->status, SPI_RX_LEVEL) > 0)
    {
        return (UINT16)(hwp_spi3->rxtx_buffer & 0xFFFF);
    }
    return 0;
}

// ============================================================================
// Convenience Wrappers for ABB and PMU
// ============================================================================
void hal_AbbWrite(UINT16 reg, UINT16 val)
{
    hal_IspiWrite(HAL_ISPI_CS_ABB, reg, val);
}

UINT16 hal_AbbRead(UINT16 reg)
{
    // Auto-select SPI read multiplexer in CODEC_RESET_CTRL (0x1F):
    // Bit 0: 0 for analog ABB (0x00-0x1F), 1 for CODEC digital (>= 0x20)
    UINT16 spiOutSel = hal_IspiRead(HAL_ISPI_CS_ABB, 0x1F);
    if (reg < 0x20)
    {
        spiOutSel &= ~1;
    }
    else
    {
        spiOutSel |= 1;
    }
    hal_IspiWrite(HAL_ISPI_CS_ABB, 0x1F, spiOutSel);
    return hal_IspiRead(HAL_ISPI_CS_ABB, reg);
}

void hal_PmuWrite(UINT16 reg, UINT16 val)
{
    hal_IspiWrite(HAL_ISPI_CS_PMU, reg, val);
}

UINT16 hal_PmuRead(UINT16 reg)
{
    return hal_IspiRead(HAL_ISPI_CS_PMU, reg);
}

void hal_PmuSetLcdPower(BOOL on)
{
    UINT16 reg02 = hal_PmuRead(PMU_REG_LDO_SETTINGS);
    UINT16 reg03 = hal_PmuRead(PMU_REG_LDO_ACTIVE1);
    UINT16 reg04 = hal_PmuRead(PMU_REG_LDO_ACTIVE2);
    UINT16 reg07 = hal_PmuRead(PMU_REG_LDO_ACTIVE5);

    if (on)
    {
        // 1. Enable vLcd in LDO settings (Reg 0x02 bit 4)
        reg02 |= (1 << 4);
        hal_PmuWrite(PMU_REG_LDO_SETTINGS, reg02);

        // 2. Clear vLcdOff in Active Profile 1 (Reg 0x03 bit 7) so rail delivers power
        reg03 &= ~(1 << 7);
        hal_PmuWrite(PMU_REG_LDO_ACTIVE1, reg03);

        // 3. Clear vLcdIs1_8 in Active Profile 2 (Reg 0x04 bit 9) for 2.8V operation
        reg04 &= ~(1 << 9);
        hal_PmuWrite(PMU_REG_LDO_ACTIVE2, reg04);

        // 4. Set vLcdIbit in Active Profile 5 (Reg 0x07 bits [8:6]) to nominal drive current (4)
        reg07 = (reg07 & ~(0x7 << 6)) | (4 << 6);
        hal_PmuWrite(PMU_REG_LDO_ACTIVE5, reg07);

        // 5. Clear vLcdOff in Low Power Profile 1 (Reg 0x08 bit 7) to avoid sleep drop
        UINT16 reg08 = hal_PmuRead(PMU_REG_LDO_LP1);
        reg08 &= ~(1 << 7);
        hal_PmuWrite(PMU_REG_LDO_LP1, reg08);
    }
    else
    {
        // Assert shutoff in Active Profile 1 (Reg 0x03 bit 7)
        reg03 |= (1 << 7);
        hal_PmuWrite(PMU_REG_LDO_ACTIVE1, reg03);

        // Disable vLcd in LDO settings (Reg 0x02 bit 4)
        reg02 &= ~(1 << 4);
        hal_PmuWrite(PMU_REG_LDO_SETTINGS, reg02);
    }

    os_log_printf("[PMU] LCD Power %s: Reg02=0x%04X, Reg03=0x%04X, Reg04=0x%04X, Reg07=0x%04X\n",
                  on ? "ON (2.8V)" : "OFF",
                  hal_PmuRead(PMU_REG_LDO_SETTINGS),
                  hal_PmuRead(PMU_REG_LDO_ACTIVE1),
                  hal_PmuRead(PMU_REG_LDO_ACTIVE2),
                  hal_PmuRead(PMU_REG_LDO_ACTIVE5));
}

BOOL hal_PmuGetLcdPower(void)
{
    UINT16 reg02 = hal_PmuRead(PMU_REG_LDO_SETTINGS);
    UINT16 reg03 = hal_PmuRead(PMU_REG_LDO_ACTIVE1);
    // ON when enabled in Reg02 and not shut off in Reg03
    return ((reg02 & (1 << 4)) != 0 && (reg03 & (1 << 7)) == 0) ? TRUE : FALSE;
}
