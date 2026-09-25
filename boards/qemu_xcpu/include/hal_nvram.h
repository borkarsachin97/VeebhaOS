/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Hardware NVRAM Subsystem Header for RDA8809
 * Persistent key/record non-volatile storage on SPI Flash
 */

#ifndef _HAL_NVRAM_H_
#define _HAL_NVRAM_H_

#include "cs_types.h"

#define NVRAM_DEFAULT_BASE_OFFSET   0x003BC000
#define NVRAM_MAGIC_HEADER1         0xbabeface
#define NVRAM_MAGIC_HEADER2         0xcafe0000
#define NVRAM_RECORD_MAGIC          0xA3BA54D0
#define NVRAM_SECTOR_SIZE           4096
#define NVRAM_MAX_DESCRIPTORS       64

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

typedef struct
{
    uint32_t magic1;           /* 0x00: 0xbabeface */
    uint32_t magic2;           /* 0x04: 0xcafe0000 */
    uint32_t total_records;    /* 0x08: Number of records registered */
    uint32_t version;          /* 0x0C: Partition version / layout */
    uint32_t valid_records;    /* 0x10: Active valid records */
    uint32_t sector_size;      /* 0x14: 4096 bytes */
    uint32_t flags;            /* 0x18: Partition flags */
    uint32_t active_bank;      /* 0x1C: Active bank index */
    char     tag[8];           /* 0x20: "NVRAM\0\0\0" */
} PACKED nvram_header_t;

typedef struct
{
    uint32_t hash_magic;       /* 0xA3BA54D0 */
    uint16_t record_id;        /* Unique Record ID */
    uint16_t status_flags;     /* 0x00FF = Active, 0x0000 = Deleted */
    uint32_t data_len;         /* Payload byte length */
    uint32_t flash_offset;     /* Absolute flash offset of data payload */
} PACKED nvram_record_desc_t;

typedef struct
{
    bool     is_mounted;
    uint32_t base_offset;
    uint32_t total_records;
    uint32_t valid_records;
    uint32_t data_pool_offset;
} nvram_status_t;

// =============================================================================
//  PUBLIC NVRAM APIS
// =============================================================================

/**
 * @brief Initialize NVRAM subsystem. Auto-detects partition or mounts default base.
 * @return true if valid NVRAM partition found or mounted.
 */
bool hal_NvramInit(void);

/**
 * @brief Format the NVRAM partition with fresh header and blank record table.
 * @return true on success.
 */
bool hal_NvramFormat(void);

/**
 * @brief Read a record by Record ID.
 * @param record_id Identifier of the record.
 * @param buf Output buffer.
 * @param max_len Capacity of output buffer.
 * @param out_len Actual bytes read.
 * @return true on success, false if not found.
 */
bool hal_NvramRead(uint16_t record_id, void *buf, uint32_t max_len, uint32_t *out_len);

/**
 * @brief Write or update a record by Record ID.
 * @param record_id Identifier of the record.
 * @param buf Data buffer to write.
 * @param len Byte count.
 * @return true on success.
 */
bool hal_NvramWrite(uint16_t record_id, const void *buf, uint32_t len);

/**
 * @brief Delete a record by marking its descriptor invalid.
 * @param record_id Identifier of the record.
 * @return true on success.
 */
bool hal_NvramDelete(uint16_t record_id);

/**
 * @brief Query current NVRAM operational status.
 */
nvram_status_t hal_NvramGetStatus(void);

/**
 * @brief Query descriptor info by index (0 .. total_records - 1).
 */
bool hal_NvramGetRecordByIndex(uint32_t idx, uint16_t *out_id, uint32_t *out_len, uint32_t *out_offset, bool *out_valid);

/**
 * @brief Dump all NVRAM records and metadata to console log.
 */
void hal_NvramDump(void);

#endif // _HAL_NVRAM_H_
