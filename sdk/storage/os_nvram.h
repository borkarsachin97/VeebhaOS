/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */

#ifndef SDK_STORAGE_OS_NVRAM_H
#define SDK_STORAGE_OS_NVRAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define NVRAM_MAGIC         0x56454542U  /* "VEEB" in little endian */
#define NVRAM_VERSION       1U
#define NVRAM_FILE_PATH_BLD "./build/veebha_nvram.bin"
#define NVRAM_FILE_PATH_LOC "./veebha_nvram.bin"

/**
 * Non-Volatile Memory Data Structure
 */
typedef struct {
    uint32_t magic;             /* 0x56454542 ("VEEB") */
    uint16_t version;           /* 1 */
    uint16_t crc16;             /* CRC-16/CCITT of payload */

    /* Display & Theme Settings */
    uint8_t  theme_id;          /* os_theme_id_t (0: Dark Cyan, 1: OLED Black, 2: Clean Light, 3: Nokia Navy, 4: Amber) */
    uint8_t  backlight_timeout; /* in seconds (e.g. 10, 30, 60) */
    uint8_t  wallpaper;         /* 0: Dark, 1: Cyber, 2: Sunset, 3: Emerald */
    uint8_t  brightness_pct;    /* 10-100 */

    /* Sound & Audio Settings */
    uint8_t  active_profile;    /* 0: General, 1: Silent, 2: Outdoor */
    uint8_t  volume_level;      /* 1-7 */

    /* Alarm Settings */
    uint8_t  alarm_hour;        /* 1-12 */
    uint8_t  alarm_min;         /* 0-59 */
    bool     alarm_pm;          /* false = AM, true = PM */
    bool     alarm_enabled;

    /* Bluetooth & Connectivity Settings */
    bool     bt_enabled;
    bool     bt_visible;
    char     bt_device_name[32];
    uint8_t  usb_mode;          /* os_usb_mode_t */
    bool     tethering_usb;
    bool     tethering_bt;

    /* Clock & Time / Date Settings */
    uint8_t  clock_hour;        /* 0-23 */
    uint8_t  clock_min;         /* 0-59 */
    bool     clock_is_24h;
    uint16_t clock_year;        /* e.g. 2026 */
    uint8_t  clock_month;       /* 1-12 */
    uint8_t  clock_day;         /* 1-31 */

    /* Wallpaper Settings */
    uint8_t  wallpaper_mode;    /* 0: WALLPAPER_MODE_THEME_SOLID, 1: WALLPAPER_MODE_IMAGE_BMP */
    char     wallpaper_path[64];/* VFS Path e.g. "/sdcard/Wallpapers/default.bmp" */

    /* Language Settings */
    uint8_t  language_id;       /* 0: LANG_EN, 1: LANG_HI, 2: LANG_RU */

    /* Reserved for future expansion */
    uint8_t  reserved[21];
} __attribute__((packed)) os_nvram_data_t;

/**
 * Initialize NVRAM subsystem. Loads binary data from disk or sets defaults if missing/corrupt.
 */
void os_nvram_init(void);

/**
 * Retrieve the active NVRAM data structure.
 *
 * @return Pointer to internal os_nvram_data_t structure.
 */
os_nvram_data_t * os_nvram_get(void);

/**
 * Persist current NVRAM data to disk. Recalculates CRC16 checksum before writing.
 *
 * @return true on success, false on write error.
 */
bool os_nvram_save(void);

/**
 * Reset NVRAM configuration to factory defaults.
 */
void os_nvram_reset_defaults(void);

/**
 * Calculate CCITT CRC-16 checksum across a memory buffer.
 *
 * @param data Buffer pointer.
 * @param len  Length in bytes.
 * @return 16-bit CCITT checksum.
 */
uint16_t os_nvram_crc16(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* SDK_STORAGE_OS_NVRAM_H */
