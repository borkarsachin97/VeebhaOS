/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Hardware SPI Flash (SFLASH) Driver Header for RDA8809
 */

#ifndef _HAL_SPI_FLASH_H_
#define _HAL_SPI_FLASH_H_

#include "cs_types.h"

// =============================================================================
//  SPI FLASH REGISTER MAP & BASE ADDRESS
// =============================================================================
#define REG_SPI_FLASH_BASE          0x01A25000
#define SPI_FLASH_MEM_MAP_BASE      0x88000000  /* Uncached physical memory-mapped window */

typedef volatile struct
{
    REG32                          spi_cmd_addr;                 //0x00000000
    REG32                          spi_block_size;               //0x00000004
    REG32                          spi_data_fifo_wo;             //0x00000008
    REG32                          spi_data_fifo_ro;             //0x0000000C
    REG32                          spi_read_back;                //0x00000010
    REG32                          spi_config;                   //0x00000014
    REG32                          spi_fifo_control;             //0x00000018
    REG32                          spi_cs_size;                  //0x0000001C
    REG32                          spi_read_cmd;                 //0x00000020
    REG32                          spi_flash_24;                 //0x00000024
    REG32                          spi_flash_28;                 //0x00000028
    REG32                          spi_flash_2c;                 //0x0000002C
    REG32                          spi_flash_30;                 //0x00000030
    REG32                          spi_flash_34;                 //0x00000034
} HWP_SPI_FLASH_T;

#define hwp_spiFlash                ((HWP_SPI_FLASH_T*) KSEG1(REG_SPI_FLASH_BASE))

// Register Bitfields
#define SPI_FLASH_SPI_FLASH_BUSY    (1<<0)
#define SPI_FLASH_TX_FIFO_EMPTY     (1<<1)
#define SPI_FLASH_TX_FIFO_FULL      (1<<2)
#define SPI_FLASH_RX_FIFO_EMPTY     (1<<3)

#define SPI_FLASH_TX_FIFO_CLR       (1<<1)
#define SPI_FLASH_RX_FIFO_CLR       (1<<0)

// Standard SPI Flash JEDEC Commands
#define SPI_FLASH_CMD_READ_ID       0x9F
#define SPI_FLASH_CMD_READ_STATUS1  0x05
#define SPI_FLASH_CMD_READ_STATUS2  0x35
#define SPI_FLASH_CMD_WRITE_ENABLE  0x06
#define SPI_FLASH_CMD_WRITE_DISABLE 0x04
#define SPI_FLASH_CMD_PAGE_PROGRAM  0x02
#define SPI_FLASH_CMD_SECTOR_ERASE  0x20  /* 4KB */
#define SPI_FLASH_CMD_BLOCK32_ERASE 0x52  /* 32KB */
#define SPI_FLASH_CMD_BLOCK64_ERASE 0xD8  /* 64KB */
#define SPI_FLASH_CMD_CHIP_ERASE    0xC7

typedef struct
{
    uint32_t jedec_id;          /* 24-bit JEDEC ID (Manufacturer | MemoryType | Capacity) */
    uint8_t  manufacturer_id;   /* 0xEF = Winbond, 0xC8 = GigaDevice, 0x20 = XM25/Micron, 0xC2 = Macronix */
    uint8_t  memory_type;       /* Flash memory architecture type */
    uint8_t  capacity_id;       /* 0x14 = 1MB, 0x15 = 2MB, 0x16 = 4MB, 0x17 = 8MB, 0x18 = 16MB */
    uint32_t size_bytes;        /* Total capacity in bytes */
    const char *manufacturer_name;
} hal_spi_flash_info_t;

// =============================================================================
//  PUBLIC SPI FLASH API
// =============================================================================

/**
 * @brief Initialize the SPI Flash controller and query JEDEC ID.
 * @return true on success.
 */
bool hal_SpiFlashInit(void);

/**
 * @brief Query 24-bit JEDEC ID from the SPI Flash chip.
 * @return 24-bit ID (e.g. 0xEF4016 for Winbond 4MB).
 */
uint32_t hal_SpiFlashReadId(void);

/**
 * @brief Read Status Register 1 (WIP, WEL, BP bits).
 */
uint8_t hal_SpiFlashReadStatus1(void);

/**
 * @brief Read Status Register 2 (QE, CMP bits).
 */
uint8_t hal_SpiFlashReadStatus2(void);

/**
 * @brief Query parsed Flash chip information and manufacturer.
 * @param info Pointer to structure to populate.
 * @return true if valid chip identified.
 */
bool hal_SpiFlashGetInfo(hal_spi_flash_info_t *info);

/**
 * @brief Read bytes from SPI Flash via memory-mapped window.
 * @param offset Flash address offset (0 .. capacity-1).
 * @param buf Destination buffer.
 * @param len Number of bytes to read.
 * @return true on success.
 */
bool hal_SpiFlashRead(uint32_t offset, void *buf, uint32_t len);

/**
 * @brief Send Write Enable (WREN) command to SPI Flash.
 * @return true on success.
 */
bool hal_SpiFlashWriteEnable(void);

/**
 * @brief Send Write Disable (WRDI) command to SPI Flash.
 * @return true on success.
 */
bool hal_SpiFlashWriteDisable(void);

/**
 * @brief Wait for Write In Progress (WIP) bit to clear.
 * @param timeout_ms Maximum time in milliseconds to wait.
 * @return true if ready, false on timeout.
 */
bool hal_SpiFlashWaitBusy(uint32_t timeout_ms);

/**
 * @brief Erase a 4KB sector.
 * @param offset Flash address aligned to 4KB (0x1000).
 * @return true on success.
 */
bool hal_SpiFlashEraseSector(uint32_t offset);

/**
 * @brief Erase a 32KB block.
 * @param offset Flash address aligned to 32KB (0x8000).
 * @return true on success.
 */
bool hal_SpiFlashEraseBlock32K(uint32_t offset);

/**
 * @brief Erase a 64KB block.
 * @param offset Flash address aligned to 64KB (0x10000).
 * @return true on success.
 */
bool hal_SpiFlashEraseBlock64K(uint32_t offset);

/**
 * @brief Erase entire SPI Flash chip (Opcode 0xC7).
 * @return true on success.
 */
bool hal_SpiFlashEraseChip(void);

/**
 * @brief Full Flash Erase with guaranteed Sector 0 priority.
 *        Erases Sector 0 first so BootROM falls back to USB bootloader,
 *        then proceeds through all subsequent 64KB blocks.
 * @param force_sector0 Explicit safety confirmation for erasing Sector 0.
 * @return true on success.
 */
bool hal_SpiFlashEraseAll(bool force_sector0);

/**
 * @brief Program up to 256 bytes into a single Flash page.
 * @param offset Flash address. Cannot cross 256-byte page boundary.
 * @param data Data buffer to write.
 * @param len Byte count (1 .. 256).
 * @return true on success.
 */
bool hal_SpiFlashPageProgram(uint32_t offset, const uint8_t *data, uint32_t len);

/**
 * @brief High-level write function: splits writes across page boundaries.
 * @param offset Flash target address.
 * @param data Data buffer to write.
 * @param len Number of bytes to write.
 * @param allow_sector0 If false, writing into Sector 0 (0..0xFFF) is rejected for safety.
 * @return true on success.
 */
bool hal_SpiFlashWrite(uint32_t offset, const uint8_t *data, uint32_t len, bool allow_sector0);

/**
 * @brief Check if a Flash region is completely blank (all 0xFF).
 * @param offset Starting offset.
 * @param len Length in bytes.
 * @return true if all bytes are 0xFF.
 */
bool hal_SpiFlashIsBlank(uint32_t offset, uint32_t len);

#endif // _HAL_SPI_FLASH_H_
