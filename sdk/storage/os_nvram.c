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

#include "sdk/storage/os_nvram.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static os_nvram_data_t s_nvram_data;
static bool s_nvram_initialized = false;

uint16_t os_nvram_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    if (!data) return crc;

    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

void os_nvram_reset_defaults(void)
{
    memset(&s_nvram_data, 0, sizeof(os_nvram_data_t));
    s_nvram_data.magic = NVRAM_MAGIC;
    s_nvram_data.version = NVRAM_VERSION;

    /* Display Defaults */
    s_nvram_data.theme_id = 0; /* THEME_DARK_CYAN */
    s_nvram_data.backlight_timeout = 30;
    s_nvram_data.wallpaper = 0;
    s_nvram_data.brightness_pct = 80;

    /* Sound Defaults */
    s_nvram_data.active_profile = 0; /* General */
    s_nvram_data.volume_level = 7;

    /* Alarm Defaults */
    s_nvram_data.alarm_hour = 7;
    s_nvram_data.alarm_min = 0;
    s_nvram_data.alarm_pm = false;
    s_nvram_data.alarm_enabled = false;

    /* Connectivity Defaults */
    s_nvram_data.bt_enabled = false;
    s_nvram_data.bt_visible = true;
    strncpy(s_nvram_data.bt_device_name, "Veebha Phone", sizeof(s_nvram_data.bt_device_name) - 1);
    s_nvram_data.usb_mode = 0; /* USB_MODE_NONE */
    s_nvram_data.tethering_usb = false;
    s_nvram_data.tethering_bt = false;

    /* Clock & Time / Date Defaults */
    s_nvram_data.clock_hour = 12;
    s_nvram_data.clock_min = 0;
    s_nvram_data.clock_is_24h = true;
    s_nvram_data.clock_year = 2026;
    s_nvram_data.clock_month = 9;
    s_nvram_data.clock_day = 23;

    /* Wallpaper Defaults */
    s_nvram_data.wallpaper_mode = 0; /* WALLPAPER_MODE_THEME_SOLID */
    strncpy(s_nvram_data.wallpaper_path, "/sdcard/Wallpapers/default.bmp", sizeof(s_nvram_data.wallpaper_path) - 1);

    /* Language Defaults */
    s_nvram_data.language_id = 0; /* LANG_EN */

    /* Compute CRC16 over payload (starting after magic, version, crc16 header) */
    const uint8_t *payload = ((const uint8_t *)&s_nvram_data) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t);
    size_t payload_len = sizeof(os_nvram_data_t) - (sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t));
    s_nvram_data.crc16 = os_nvram_crc16(payload, payload_len);

    printf("[NVRAM] Initialized default parameters (CRC16: 0x%04X)\n", (unsigned int)s_nvram_data.crc16);
}

#if defined(CONFIG_SIMULATOR) && !defined(CONFIG_IS_RAMRUN_ONLY)
static bool read_nvram_file(const char *path, os_nvram_data_t *out_data)
{
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    size_t read_bytes = fread(out_data, 1, sizeof(os_nvram_data_t), f);
    fclose(f);

    if (read_bytes != sizeof(os_nvram_data_t)) {
        printf("[NVRAM] File '%s' size mismatch (%zu vs %zu)\n", path, read_bytes, sizeof(os_nvram_data_t));
        return false;
    }

    if (out_data->magic != NVRAM_MAGIC) {
        printf("[NVRAM] File '%s' magic mismatch (0x%08X vs 0x%08X)\n", path, (unsigned int)out_data->magic, (unsigned int)NVRAM_MAGIC);
        return false;
    }

    if (out_data->version != NVRAM_VERSION) {
        printf("[NVRAM] File '%s' version mismatch (%u vs %u)\n", path, (unsigned int)out_data->version, (unsigned int)NVRAM_VERSION);
        return false;
    }

    const uint8_t *payload = ((const uint8_t *)out_data) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t);
    size_t payload_len = sizeof(os_nvram_data_t) - (sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t));
    uint16_t expected_crc = os_nvram_crc16(payload, payload_len);

    if (out_data->crc16 != expected_crc) {
        printf("[NVRAM] File '%s' CRC mismatch (0x%04X vs 0x%04X)\n", path, (unsigned int)out_data->crc16, (unsigned int)expected_crc);
        return false;
    }

    return true;
}
#endif

void os_nvram_init(void)
{
    if (s_nvram_initialized) return;
    s_nvram_initialized = true;

#if defined(CONFIG_SIMULATOR) && !defined(CONFIG_IS_RAMRUN_ONLY)
    if (read_nvram_file(NVRAM_FILE_PATH_BLD, &s_nvram_data)) {
        printf("[NVRAM] Successfully loaded persistent store from %s\n", NVRAM_FILE_PATH_BLD);
        return;
    }
    if (read_nvram_file(NVRAM_FILE_PATH_LOC, &s_nvram_data)) {
        printf("[NVRAM] Successfully loaded persistent store from %s\n", NVRAM_FILE_PATH_LOC);
        return;
    }
    printf("[NVRAM] No valid store found on disk. Initializing factory defaults.\n");
#endif

    os_nvram_reset_defaults();

#if defined(CONFIG_SIMULATOR) && !defined(CONFIG_IS_RAMRUN_ONLY)
    os_nvram_save();
#endif
}

os_nvram_data_t * os_nvram_get(void)
{
    if (!s_nvram_initialized) {
        os_nvram_init();
    }
    return &s_nvram_data;
}

bool os_nvram_save(void)
{
    if (!s_nvram_initialized) {
        os_nvram_init();
    }
    s_nvram_data.magic = NVRAM_MAGIC;
    s_nvram_data.version = NVRAM_VERSION;

    const uint8_t *payload = ((const uint8_t *)&s_nvram_data) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t);
    size_t payload_len = sizeof(os_nvram_data_t) - (sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t));
    s_nvram_data.crc16 = os_nvram_crc16(payload, payload_len);

#if defined(CONFIG_SIMULATOR) && !defined(CONFIG_IS_RAMRUN_ONLY)
    FILE *f = fopen(NVRAM_FILE_PATH_BLD, "wb");
    if (!f) {
        f = fopen(NVRAM_FILE_PATH_LOC, "wb");
    }

    if (!f) {
        fprintf(stderr, "[NVRAM ERROR] Failed to open file for writing\n");
        return false;
    }
    size_t written = fwrite(&s_nvram_data, 1, sizeof(os_nvram_data_t), f);
    fclose(f);
    if (written != sizeof(os_nvram_data_t)) {
        fprintf(stderr, "[NVRAM ERROR] Incomplete write (%zu of %zu bytes)\n", written, sizeof(os_nvram_data_t));
        return false;
    }
    printf("[NVRAM] Successfully saved %zu bytes (CRC16: 0x%04X)\n", written, (unsigned int)s_nvram_data.crc16);
#endif

    return true;
}
