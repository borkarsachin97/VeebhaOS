/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * BYD BF30A2 (BF30A2Serial) CMOS Image Sensor Driver Header
 */

#ifndef _CAM_BF30A2_H_
#define _CAM_BF30A2_H_

#include "cs_types.h"
#include "hal_i2c.h"
#include "hal_camera.h"

// 7-bit I2C Slave Address: 0x6E (8-bit Write: 0xDC, Read: 0xDD)
#define BF30A2_I2C_ADDR                     0x6E
#define BF30A2_I2C_BUS                      HAL_I2C_BUS_1

// Sensor Identification Registers
#define BF30A2_REG_CHIP_ID_H                0xFC    // Expected: 0x30 (or 0x3A)
#define BF30A2_REG_CHIP_ID_L                0xFD    // Expected: 0xA2 or 0x02

#define BF30A2_CHIP_ID                      0x30A2

// Resolutions
#define BF30A2_WIDTH_VGA                    640
#define BF30A2_HEIGHT_VGA                   480

#define BF30A2_WIDTH_QVGA                   320
#define BF30A2_HEIGHT_QVGA                  240

#define BF30A2_WIDTH_PREVIEW                176
#define BF30A2_HEIGHT_PREVIEW               220

#define BF30A2_WIDTH_QQVGA                  160
#define BF30A2_HEIGHT_QQVGA                 120

// Public Driver Functions
BOOL bf30a2_probe(void);
BOOL bf30a2_init(void);
void bf30a2_power_on(void);
void bf30a2_power_off(void);

BOOL bf30a2_start_stream(UINT8 *buf1, UINT8 *buf2, UINT32 bufSize);
void bf30a2_stop_stream(void);
BOOL bf30a2_capture_frame(UINT8 *frameBuffer, UINT32 bufSize);

UINT16 bf30a2_get_id(void);
UINT8 bf30a2_read_reg(UINT8 reg);
BOOL bf30a2_write_reg(UINT8 reg, UINT8 val);

#endif // _CAM_BF30A2_H_
