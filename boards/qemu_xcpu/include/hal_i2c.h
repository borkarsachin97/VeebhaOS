/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Hardware I2C Master Driver Header for RDA8809
 */

#ifndef _HAL_I2C_H_
#define _HAL_I2C_H_

#include "cs_types.h"

// =============================================================================
//  I2C BUS IDENTIFIERS & BASE ADDRESSES
// =============================================================================
#define REG_I2C_MASTER1_BASE        0x01A07000
#define REG_I2C_MASTER2_BASE        0x01A22000
#define REG_I2C_MASTER3_BASE        0x01A23000

typedef enum {
    HAL_I2C_BUS_1 = 0,
    HAL_I2C_BUS_2 = 1,
    HAL_I2C_BUS_3 = 2,
    HAL_I2C_BUS_QTY = 3
} hal_i2c_bus_t;

typedef enum {
    HAL_I2C_SPEED_100K = 100000,
    HAL_I2C_SPEED_400K = 400000
} hal_i2c_speed_t;

typedef volatile struct
{
    REG32                          CTRL;                         //0x00000000
    REG32                          STATUS;                       //0x00000004
    REG32                          TXRX_BUFFER;                  //0x00000008
    REG32                          CMD;                          //0x0000000C
    REG32                          IRQ_CLR;                      //0x00000010
} HWP_I2C_MASTER_T;

#define hwp_i2c1                    ((HWP_I2C_MASTER_T*) KSEG1(REG_I2C_MASTER1_BASE))
#define hwp_i2c2                    ((HWP_I2C_MASTER_T*) KSEG1(REG_I2C_MASTER2_BASE))
#define hwp_i2c3                    ((HWP_I2C_MASTER_T*) KSEG1(REG_I2C_MASTER3_BASE))

// CTRL Register Bits
#define I2C_MASTER_EN               (1<<0)
#define I2C_MASTER_IRQ_MASK         (1<<8)
#define I2C_MASTER_CLOCK_PRESCALE(n) (((n)&0xFFFF)<<16)
#define I2C_MASTER_CLOCK_PRESCALE_MASK (0xFFFF<<16)

// STATUS Register Bits
#define I2C_MASTER_IRQ_CAUSE        (1<<0)
#define I2C_MASTER_IRQ_STATUS       (1<<4)
#define I2C_MASTER_TIP              (1<<8)
#define I2C_MASTER_AL               (1<<12)
#define I2C_MASTER_BUSY             (1<<16)
#define I2C_MASTER_RXACK            (1<<20)

// CMD Register Bits
#define I2C_MASTER_ACK              (1<<0)
#define I2C_MASTER_RD               (1<<4)
#define I2C_MASTER_STO              (1<<8)
#define I2C_MASTER_WR               (1<<12)
#define I2C_MASTER_STA              (1<<16)

// IRQ_CLR Register Bits
#define I2C_MASTER_IRQ_CLR          (1<<0)

// =============================================================================
//  PUBLIC I2C API
// =============================================================================

/**
 * @brief Initialize and open an I2C master bus instance.
 * @param bus I2C bus index (HAL_I2C_BUS_1, HAL_I2C_BUS_2, HAL_I2C_BUS_3).
 * @param speed Clock speed in Hz (HAL_I2C_SPEED_100K or HAL_I2C_SPEED_400K).
 * @return true on success, false on failure.
 */
bool hal_I2cOpen(hal_i2c_bus_t bus, hal_i2c_speed_t speed);

/**
 * @brief Close an I2C master bus.
 * @param bus I2C bus index.
 */
void hal_I2cClose(hal_i2c_bus_t bus);

/**
 * @brief Write a single byte to an 8-bit register on an I2C slave device.
 * @param bus I2C bus index.
 * @param slave_addr 7-bit slave address.
 * @param reg_addr 8-bit register index.
 * @param data Data byte to write.
 * @return true if ACK received, false on error/NACK.
 */
bool hal_I2cWriteReg(hal_i2c_bus_t bus, uint8_t slave_addr, uint8_t reg_addr, uint8_t data);
bool hal_I2cReadReg(hal_i2c_bus_t bus, uint8_t slave_addr, uint8_t reg_addr, uint8_t *out_data);

/**
 * @brief Write 16-bit word (big-endian) to 8-bit register on I2C slave device (e.g. BT RF 0x16).
 */
bool hal_I2cWriteReg16(hal_i2c_bus_t bus, uint8_t slave_addr, uint8_t reg_addr, uint16_t data);

/**
 * @brief Read 16-bit word (big-endian) from 8-bit register on I2C slave device (e.g. BT RF 0x16).
 */
bool hal_I2cReadReg16(hal_i2c_bus_t bus, uint8_t slave_addr, uint8_t reg_addr, uint16_t *out_data);

/**
 * @brief Write 32-bit word to 32-bit register on I2C slave device (e.g. BT Core 0x15).
 */
bool hal_I2cWriteReg32Core(hal_i2c_bus_t bus, uint8_t slave_addr, uint32_t reg_addr, uint32_t data);

/**
 * @brief Read 32-bit word from 32-bit register on I2C slave device (e.g. BT Core 0x15).
 */
bool hal_I2cReadReg32Core(hal_i2c_bus_t bus, uint8_t slave_addr, uint32_t reg_addr, uint32_t *out_data);

/**
 * @brief Scan the I2C bus for active slave addresses (0x08 .. 0x77).
 * @param bus I2C bus index.
 * @param found_addrs Output array to receive responsive slave addresses.
 * @param max_count Maximum number of entries in found_addrs.
 * @return Number of responsive slave devices found.
 */
uint32_t hal_I2cScan(hal_i2c_bus_t bus, uint8_t *found_addrs, uint32_t max_count);

#endif // _HAL_I2C_H_
