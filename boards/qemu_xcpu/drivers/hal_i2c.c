/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Hardware I2C Master Driver Implementation for RDA8809
 */

#include "cs_types.h"
#include "global_macros.h"
#include "sys_ctrl.h"
#include "cfg_regs.h"
#include "hal_i2c.h"
#include "timer.h"

extern void os_log_printf(const char *fmt, ...);

static HWP_I2C_MASTER_T* const g_i2c_hw[HAL_I2C_BUS_QTY] = {
    hwp_i2c1,
    hwp_i2c2,
    hwp_i2c3
};

#define I2C_TIMEOUT_CYCLES  50000

static bool i2c_wait_tip(HWP_I2C_MASTER_T *hw, uint32_t timeout_ms)
{
    uint32_t start = timer_get_ms();
    while (hw->STATUS & I2C_MASTER_TIP) {
        if ((timer_get_ms() - start) > timeout_ms) {
            return false;
        }
    }
    return true;
}

bool hal_I2cOpen(hal_i2c_bus_t bus, hal_i2c_speed_t speed)
{
    if (bus >= HAL_I2C_BUS_QTY) return false;

    // 1. Configure Pin Multiplexing & GPIO Mode for the selected bus
    if (bus == HAL_I2C_BUS_1) {
        hwp_configRegs->Alt_mux_select |= CFG_REGS_I2C1_I2C1;
    } else if (bus == HAL_I2C_BUS_2) {
        hwp_configRegs->GPIO_Mode &= ~((1 << 24) | (1 << 25));
        hwp_configRegs->Alt_mux_select &= ~CFG_REGS_I2C2_MASK;
    } else if (bus == HAL_I2C_BUS_3) {
        hwp_configRegs->GPIO_Mode &= ~((1 << 14) | (1 << 16));
        hwp_configRegs->Alt_mux_select |= CFG_REGS_SPI1_SELECT_I2C_3;
    }

    // 2. Enable I2C peripheral clock and clear reset in sysCtrl (Direct assignment to write-1-to-set registers)
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_UNLOCK;
    if (bus == HAL_I2C_BUS_1) {
        hwp_sysCtrl->Clk_Per_Enable = SYS_CTRL_ENABLE_PER_I2C;
        hwp_sysCtrl->Sys_Rst_Clr = SYS_CTRL_CLR_RST_I2C;
    } else if (bus == HAL_I2C_BUS_2) {
        hwp_sysCtrl->Clk_Per_Enable = SYS_CTRL_ENABLE_PER_I2C2;
        hwp_sysCtrl->Sys_Rst_Clr = SYS_CTRL_CLR_RST_I2C2;
    } else {
        hwp_sysCtrl->Clk_Per_Enable = SYS_CTRL_ENABLE_PER_I2C3;
        hwp_sysCtrl->Sys_Rst_Clr = SYS_CTRL_CLR_RST_I2C3;
    }
    hwp_sysCtrl->REG_DBG = SYS_CTRL_PROTECT_LOCK;

    HWP_I2C_MASTER_T *hw = g_i2c_hw[bus];

    // Prescaler formula: sysFreq / (5 * speed) - 1
    // At 104 MHz: 104000000 / (5 * 100000) - 1 = 207 (0x00CF)
    uint32_t sys_freq = 104000000;
    uint32_t prescale = sys_freq / (5 * (uint32_t)speed) - 1;
    if (prescale > 0xFFFF) prescale = 0xFFFF;

    hw->CTRL = I2C_MASTER_EN | I2C_MASTER_CLOCK_PRESCALE(prescale);
    hw->CMD = I2C_MASTER_STO;

    i2c_wait_tip(hw, 10);
    hw->IRQ_CLR = I2C_MASTER_IRQ_CLR;

    return true;
}

void hal_I2cClose(hal_i2c_bus_t bus)
{
    if (bus >= HAL_I2C_BUS_QTY) return;
    HWP_I2C_MASTER_T *hw = g_i2c_hw[bus];
    hw->CMD = I2C_MASTER_STO;
    i2c_wait_tip(hw, 10);
    hw->CTRL = 0;
}

bool hal_I2cWriteReg(hal_i2c_bus_t bus, uint8_t slave_addr, uint8_t reg_addr, uint8_t data)
{
    if (bus >= HAL_I2C_BUS_QTY) return false;
    HWP_I2C_MASTER_T *hw = g_i2c_hw[bus];

    if (!i2c_wait_tip(hw, 5)) return false;

    // 1. Send Slave Address (Write: bit 0 = 0) + START
    hw->TXRX_BUFFER = (slave_addr << 1) & 0xFE;
    hw->CMD = I2C_MASTER_WR | I2C_MASTER_STA;
    if (!i2c_wait_tip(hw, 5)) return false;
    if (hw->STATUS & I2C_MASTER_RXACK) {
        hw->CMD = I2C_MASTER_STO;
        i2c_wait_tip(hw, 5);
        return false; // NACK
    }

    // 2. Send Register Address
    hw->TXRX_BUFFER = reg_addr;
    hw->CMD = I2C_MASTER_WR;
    if (!i2c_wait_tip(hw, 5)) return false;
    if (hw->STATUS & I2C_MASTER_RXACK) {
        hw->CMD = I2C_MASTER_STO;
        i2c_wait_tip(hw, 5);
        return false; // NACK
    }

    // 3. Send Data Byte + STOP
    hw->TXRX_BUFFER = data;
    hw->CMD = I2C_MASTER_WR | I2C_MASTER_STO;
    if (!i2c_wait_tip(hw, 5)) return false;
    if (hw->STATUS & I2C_MASTER_RXACK) return false;

    return true;
}

bool hal_I2cReadReg(hal_i2c_bus_t bus, uint8_t slave_addr, uint8_t reg_addr, uint8_t *out_data)
{
    if (bus >= HAL_I2C_BUS_QTY || !out_data) return false;
    HWP_I2C_MASTER_T *hw = g_i2c_hw[bus];

    if (!i2c_wait_tip(hw, 5)) return false;

    // 1. Send Slave Address (Write) + START
    hw->TXRX_BUFFER = (slave_addr << 1) & 0xFE;
    hw->CMD = I2C_MASTER_WR | I2C_MASTER_STA;
    if (!i2c_wait_tip(hw, 5)) return false;
    if (hw->STATUS & I2C_MASTER_RXACK) {
        hw->CMD = I2C_MASTER_STO;
        i2c_wait_tip(hw, 5);
        return false; // NACK
    }

    // 2. Send Register Address
    hw->TXRX_BUFFER = reg_addr;
    hw->CMD = I2C_MASTER_WR;
    if (!i2c_wait_tip(hw, 5)) return false;
    if (hw->STATUS & I2C_MASTER_RXACK) {
        hw->CMD = I2C_MASTER_STO;
        i2c_wait_tip(hw, 5);
        return false; // NACK
    }

    // 3. Repeated START + Slave Address (Read: bit 0 = 1)
    hw->TXRX_BUFFER = (slave_addr << 1) | 0x01;
    hw->CMD = I2C_MASTER_WR | I2C_MASTER_STA;
    if (!i2c_wait_tip(hw, 5)) return false;
    if (hw->STATUS & I2C_MASTER_RXACK) {
        hw->CMD = I2C_MASTER_STO;
        i2c_wait_tip(hw, 5);
        return false; // NACK
    }

    // 4. Read Data Byte + NACK + STOP
    hw->CMD = I2C_MASTER_RD | I2C_MASTER_ACK | I2C_MASTER_STO; // ACK=1 means send NACK
    if (!i2c_wait_tip(hw, 5)) return false;

    *out_data = (uint8_t)(hw->TXRX_BUFFER & 0xFF);
    return true;
}

bool hal_I2cWriteReg16(hal_i2c_bus_t bus, uint8_t slave_addr, uint8_t reg_addr, uint16_t data)
{
    if (bus >= HAL_I2C_BUS_QTY) return false;
    HWP_I2C_MASTER_T *hw = g_i2c_hw[bus];

    if (!i2c_wait_tip(hw, 5)) return false;

    // 1. Send Slave Address (Write) + START
    hw->TXRX_BUFFER = (slave_addr << 1) & 0xFE;
    hw->CMD = I2C_MASTER_WR | I2C_MASTER_STA;
    if (!i2c_wait_tip(hw, 5)) return false;
    if (hw->STATUS & I2C_MASTER_RXACK) {
        hw->CMD = I2C_MASTER_STO;
        i2c_wait_tip(hw, 5);
        return false;
    }

    // 2. Send Register Address
    hw->TXRX_BUFFER = reg_addr;
    hw->CMD = I2C_MASTER_WR;
    if (!i2c_wait_tip(hw, 5)) return false;
    if (hw->STATUS & I2C_MASTER_RXACK) {
        hw->CMD = I2C_MASTER_STO;
        i2c_wait_tip(hw, 5);
        return false;
    }

    // 3. Send High Byte
    hw->TXRX_BUFFER = (uint8_t)(data >> 8);
    hw->CMD = I2C_MASTER_WR;
    if (!i2c_wait_tip(hw, 5)) return false;
    if (hw->STATUS & I2C_MASTER_RXACK) {
        hw->CMD = I2C_MASTER_STO;
        i2c_wait_tip(hw, 5);
        return false;
    }

    // 4. Send Low Byte + STOP
    hw->TXRX_BUFFER = (uint8_t)(data & 0xFF);
    hw->CMD = I2C_MASTER_WR | I2C_MASTER_STO;
    if (!i2c_wait_tip(hw, 5)) return false;
    if (hw->STATUS & I2C_MASTER_RXACK) return false;

    return true;
}

bool hal_I2cReadReg16(hal_i2c_bus_t bus, uint8_t slave_addr, uint8_t reg_addr, uint16_t *out_data)
{
    if (bus >= HAL_I2C_BUS_QTY || !out_data) return false;
    HWP_I2C_MASTER_T *hw = g_i2c_hw[bus];

    if (!i2c_wait_tip(hw, 5)) return false;

    // 1. Send Slave Address (Write) + START
    hw->TXRX_BUFFER = (slave_addr << 1) & 0xFE;
    hw->CMD = I2C_MASTER_WR | I2C_MASTER_STA;
    if (!i2c_wait_tip(hw, 5)) return false;
    if (hw->STATUS & I2C_MASTER_RXACK) {
        hw->CMD = I2C_MASTER_STO;
        i2c_wait_tip(hw, 5);
        return false;
    }

    // 2. Send Register Address
    hw->TXRX_BUFFER = reg_addr;
    hw->CMD = I2C_MASTER_WR;
    if (!i2c_wait_tip(hw, 5)) return false;
    if (hw->STATUS & I2C_MASTER_RXACK) {
        hw->CMD = I2C_MASTER_STO;
        i2c_wait_tip(hw, 5);
        return false;
    }

    // 3. Repeated START + Slave Address (Read)
    hw->TXRX_BUFFER = (slave_addr << 1) | 0x01;
    hw->CMD = I2C_MASTER_WR | I2C_MASTER_STA;
    if (!i2c_wait_tip(hw, 5)) return false;
    if (hw->STATUS & I2C_MASTER_RXACK) {
        hw->CMD = I2C_MASTER_STO;
        i2c_wait_tip(hw, 5);
        return false;
    }

    // 4. Read High Byte with ACK
    hw->CMD = I2C_MASTER_RD;
    if (!i2c_wait_tip(hw, 5)) return false;
    uint8_t hi = (uint8_t)hw->TXRX_BUFFER;

    // 5. Read Low Byte with NACK + STOP
    hw->CMD = I2C_MASTER_RD | I2C_MASTER_ACK | I2C_MASTER_STO;
    if (!i2c_wait_tip(hw, 5)) return false;
    uint8_t lo = (uint8_t)hw->TXRX_BUFFER;

    *out_data = ((uint16_t)hi << 8) | lo;
    return true;
}

bool hal_I2cWriteReg32Core(hal_i2c_bus_t bus, uint8_t slave_addr, uint32_t reg_addr, uint32_t data)
{
    if (bus >= HAL_I2C_BUS_QTY) return false;
    HWP_I2C_MASTER_T *hw = g_i2c_hw[bus];

    if (!i2c_wait_tip(hw, 5)) return false;

    // 1. Send Slave Address (Write) + START
    hw->TXRX_BUFFER = (slave_addr << 1) & 0xFE;
    hw->CMD = I2C_MASTER_WR | I2C_MASTER_STA;
    if (!i2c_wait_tip(hw, 5)) return false;
    if (hw->STATUS & I2C_MASTER_RXACK) {
        hw->CMD = I2C_MASTER_STO;
        i2c_wait_tip(hw, 5);
        return false;
    }

    // 2. Send 4 bytes of 32-bit register address (MSB first)
    uint8_t addr_bytes[4] = {
        (uint8_t)(reg_addr >> 24),
        (uint8_t)(reg_addr >> 16),
        (uint8_t)(reg_addr >> 8),
        (uint8_t)(reg_addr & 0xFF)
    };
    for (int i = 0; i < 4; i++) {
        hw->TXRX_BUFFER = addr_bytes[i];
        hw->CMD = I2C_MASTER_WR;
        if (!i2c_wait_tip(hw, 5)) return false;
        if (hw->STATUS & I2C_MASTER_RXACK) {
            hw->CMD = I2C_MASTER_STO;
            i2c_wait_tip(hw, 5);
            return false;
        }
    }

    // 3. Repeated START + Slave Address (Write)
    hw->TXRX_BUFFER = (slave_addr << 1) & 0xFE;
    hw->CMD = I2C_MASTER_WR | I2C_MASTER_STA;
    if (!i2c_wait_tip(hw, 5)) return false;
    if (hw->STATUS & I2C_MASTER_RXACK) {
        hw->CMD = I2C_MASTER_STO;
        i2c_wait_tip(hw, 5);
        return false;
    }

    // 4. Send 4 bytes of 32-bit data (MSB first)
    uint8_t data_bytes[4] = {
        (uint8_t)(data >> 24),
        (uint8_t)(data >> 16),
        (uint8_t)(data >> 8),
        (uint8_t)(data & 0xFF)
    };
    for (int i = 0; i < 3; i++) {
        hw->TXRX_BUFFER = data_bytes[i];
        hw->CMD = I2C_MASTER_WR;
        if (!i2c_wait_tip(hw, 5)) return false;
        if (hw->STATUS & I2C_MASTER_RXACK) {
            hw->CMD = I2C_MASTER_STO;
            i2c_wait_tip(hw, 5);
            return false;
        }
    }
    // Last byte with STOP
    hw->TXRX_BUFFER = data_bytes[3];
    hw->CMD = I2C_MASTER_WR | I2C_MASTER_STO;
    if (!i2c_wait_tip(hw, 5)) return false;
    if (hw->STATUS & I2C_MASTER_RXACK) return false;

    return true;
}

bool hal_I2cReadReg32Core(hal_i2c_bus_t bus, uint8_t slave_addr, uint32_t reg_addr, uint32_t *out_data)
{
    if (bus >= HAL_I2C_BUS_QTY || !out_data) return false;
    HWP_I2C_MASTER_T *hw = g_i2c_hw[bus];

    if (!i2c_wait_tip(hw, 5)) return false;

    // 1. Send Slave Address (Write) + START
    hw->TXRX_BUFFER = (slave_addr << 1) & 0xFE;
    hw->CMD = I2C_MASTER_WR | I2C_MASTER_STA;
    if (!i2c_wait_tip(hw, 5)) return false;
    if (hw->STATUS & I2C_MASTER_RXACK) {
        hw->CMD = I2C_MASTER_STO;
        i2c_wait_tip(hw, 5);
        return false;
    }

    // 2. Send 4 bytes of 32-bit register address (MSB first)
    uint8_t addr_bytes[4] = {
        (uint8_t)(reg_addr >> 24),
        (uint8_t)(reg_addr >> 16),
        (uint8_t)(reg_addr >> 8),
        (uint8_t)(reg_addr & 0xFF)
    };
    for (int i = 0; i < 4; i++) {
        hw->TXRX_BUFFER = addr_bytes[i];
        hw->CMD = I2C_MASTER_WR;
        if (!i2c_wait_tip(hw, 5)) return false;
        if (hw->STATUS & I2C_MASTER_RXACK) {
            hw->CMD = I2C_MASTER_STO;
            i2c_wait_tip(hw, 5);
            return false;
        }
    }

    // 3. Repeated START + Slave Address (Read: bit 0 = 1)
    hw->TXRX_BUFFER = (slave_addr << 1) | 0x01;
    hw->CMD = I2C_MASTER_WR | I2C_MASTER_STA;
    if (!i2c_wait_tip(hw, 5)) return false;
    if (hw->STATUS & I2C_MASTER_RXACK) {
        hw->CMD = I2C_MASTER_STO;
        i2c_wait_tip(hw, 5);
        return false;
    }

    // 4. Read 4 bytes of 32-bit data (MSB first)
    uint8_t data_bytes[4];
    for (int i = 0; i < 3; i++) {
        hw->CMD = I2C_MASTER_RD;
        if (!i2c_wait_tip(hw, 5)) return false;
        data_bytes[i] = (uint8_t)hw->TXRX_BUFFER;
    }
    // Last byte with NACK + STOP
    hw->CMD = I2C_MASTER_RD | I2C_MASTER_ACK | I2C_MASTER_STO;
    if (!i2c_wait_tip(hw, 5)) return false;
    data_bytes[3] = (uint8_t)hw->TXRX_BUFFER;

    *out_data = ((uint32_t)data_bytes[0] << 24) |
                ((uint32_t)data_bytes[1] << 16) |
                ((uint32_t)data_bytes[2] << 8)  |
                ((uint32_t)data_bytes[3]);
    return true;
}

uint32_t hal_I2cScan(hal_i2c_bus_t bus, uint8_t *found_addrs, uint32_t max_count)
{
    if (bus >= HAL_I2C_BUS_QTY || !found_addrs || max_count == 0) return 0;
    HWP_I2C_MASTER_T *hw = g_i2c_hw[bus];

    uint32_t found = 0;

    for (uint8_t addr = 0x08; addr <= 0x77; addr++)
    {
        if (!i2c_wait_tip(hw, 5)) continue;

        // Probe address with write bit
        hw->TXRX_BUFFER = (addr << 1) & 0xFE;
        hw->CMD = I2C_MASTER_WR | I2C_MASTER_STA | I2C_MASTER_STO;
        if (!i2c_wait_tip(hw, 5)) continue;

        // If RXACK is 0, slave ACKed the address!
        if ((hw->STATUS & I2C_MASTER_RXACK) == 0)
        {
            found_addrs[found++] = addr;
            if (found >= max_count) break;
        }
    }

    return found;
}
