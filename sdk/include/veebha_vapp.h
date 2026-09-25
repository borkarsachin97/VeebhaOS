/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef VEEBHA_VAPP_H
#define VEEBHA_VAPP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VAPP_MAGIC             0x50504156 /* "VAPP" in Little-Endian (0x56, 0x41, 0x50, 0x50) */
#define VAPP_ABI_VERSION       1
#define VAPP_DIR_PATH          "/vapps"

typedef enum {
    VAPP_TYPE_GAME = 0,
    VAPP_TYPE_APP = 1,
    VAPP_TYPE_SOFTWARE = 2,
    VAPP_TYPE_EXTENSION = 3,
    VAPP_TYPE_UTILITY = 4,
    VAPP_TYPE_COUNT
} vapp_type_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;              /* 0x50504156 ("VAPP") */
    uint16_t abi_version;       /* Current = 1 */
    uint16_t app_type;          /* vapp_type_t (Game, App, Software, Extension, Utility) */
    char     name[32];          /* Display Name, e.g. "Brick Breaker" */
    char     author[24];        /* Author Name, e.g. "Veebha Team" */
    char     version[12];       /* e.g. "1.0.0" */
    char     icon_symbol[8];    /* FontAwesome/LVGL symbol string, e.g. LV_SYMBOL_PLAY */
    char     description[64];   /* Short description */
    uint32_t req_heap_bytes;    /* Requested heap boundary, e.g. 16384 (16 KB) */
    uint32_t code_size;         /* Executable / bytecode payload size */
    uint32_t app_id;            /* Built-in or runtime app identifier */
    uint16_t header_crc16;      /* Header CCITT CRC16 */
} vapp_header_t;

typedef struct {
    char          filename[64];  /* Filename in /vapps, e.g. "brick_breaker.vapp" */
    char          filepath[128]; /* Full path, e.g. "/vapps/brick_breaker.vapp" */
    vapp_header_t header;        /* Parsed package header */
    uint32_t      file_size;     /* Total package size in bytes */
    bool          is_valid;      /* True if header validated successfully */
} vapp_package_t;

const char *vapp_type_to_string(vapp_type_t type);

#ifdef __cplusplus
}
#endif

#endif /* VEEBHA_VAPP_H */
