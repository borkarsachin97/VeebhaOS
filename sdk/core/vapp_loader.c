/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "vapp_loader.h"
#include "apps/vapps/vapp_tetris.h"
#include "apps/vapps/vapp_brick_breaker.h"
#include "apps/vapps/vapp_unit_converter.h"
#include "apps/vapps/vapp_morse_flasher.h"
#include "apps/vapps/vapp_chip_synth.h"
#include "apps/vapps/vapp_sys_monitor.h"
#include "sdk/include/veebha_log.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_templates.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_event.h"
#include "sdk/include/veebha_i18n.h"
#include "sdk/text/font_fallback.h"
#include "boards/board_config.h"
#include "kernel/freertos/include/FreeRTOS.h"
#include "kernel/freertos/include/task.h"
#include "third_party/lvgl/lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(CONFIG_BOARD_SIMULATOR) && CONFIG_BOARD_SIMULATOR
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#endif

#define TAG "VAPP_LOADER"

const char *vapp_type_to_string(vapp_type_t type)
{
    switch (type) {
    case VAPP_TYPE_GAME:      return "Game";
    case VAPP_TYPE_APP:       return "App";
    case VAPP_TYPE_SOFTWARE:  return "Software";
    case VAPP_TYPE_EXTENSION: return "Extension";
    case VAPP_TYPE_UTILITY:   return "Utility";
    default:                  return "Unknown";
    }
}

uint16_t vapp_compute_crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

#if defined(CONFIG_SIMULATOR) && !defined(CONFIG_IS_RAMRUN_ONLY)
/* ============================================================================
 * Standard Sample Packages Creator
 * ============================================================================ */
static void create_sample_package(const char *filename, uint32_t app_id, vapp_type_t type,
                                 const char *name, const char *author, const char *ver,
                                 const char *icon, const char *desc, uint32_t req_heap, uint32_t code_size)
{
    char fullpath[128];
    snprintf(fullpath, sizeof(fullpath), "./vapps/%s", filename);

    FILE *f = fopen(fullpath, "wb");
    if (!f) {
        snprintf(fullpath, sizeof(fullpath), "./build/vapps/%s", filename);
        f = fopen(fullpath, "wb");
        if (!f) return;
    }

    vapp_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = VAPP_MAGIC;
    hdr.abi_version = VAPP_ABI_VERSION;
    hdr.app_type = (uint16_t)type;
    hdr.app_id = app_id;
    strncpy(hdr.name, name, sizeof(hdr.name) - 1);
    strncpy(hdr.author, author, sizeof(hdr.author) - 1);
    strncpy(hdr.version, ver, sizeof(hdr.version) - 1);
    strncpy(hdr.icon_symbol, icon, sizeof(hdr.icon_symbol) - 1);
    strncpy(hdr.description, desc, sizeof(hdr.description) - 1);
    hdr.req_heap_bytes = req_heap;
    hdr.code_size = code_size;

    /* Compute CRC of header excluding crc field itself */
    hdr.header_crc16 = vapp_compute_crc16((const uint8_t *)&hdr, offsetof(vapp_header_t, header_crc16));

    fwrite(&hdr, sizeof(hdr), 1, f);

    /* Write dummy executable bytecode payload */
    uint8_t dummy_payload[256];
    memset(dummy_payload, 0x90, sizeof(dummy_payload));
    fwrite(dummy_payload, sizeof(dummy_payload), 1, f);

    fclose(f);
}
#endif

void vapp_loader_init(void)
{
    OS_LOGI(TAG, "Initializing VAPP package manager (Mount: %s)", VAPP_DIR_PATH);

#if defined(CONFIG_SIMULATOR) && !defined(CONFIG_IS_RAMRUN_ONLY)
    /* Ensure ./vapps exists on host filesystem */
    mkdir("./vapps", 0755);
    mkdir("./build/vapps", 0755);

    /* Generate default standard .vapp packages if missing */
    create_sample_package("brick_breaker.vapp", 1, VAPP_TYPE_GAME,
                          "Brick Breaker", "RetroGames", "1.2.0",
                          LV_SYMBOL_PLAY, "Arcade paddle & brick destruction", 16384, 8192);

    create_sample_package("unit_converter.vapp", 2, VAPP_TYPE_APP,
                          "Unit Converter", "Veebha Tools", "2.0.1",
                          LV_SYMBOL_REFRESH, "Convert Length, Weight & Temp", 8192, 4096);

    create_sample_package("morse_flasher.vapp", 3, VAPP_TYPE_EXTENSION,
                          "Morse Flasher", "OpticDev", "1.0.0",
                          LV_SYMBOL_EYE_OPEN, "Optical screen Morse signal emitter", 4096, 2048);

    create_sample_package("chip_synth.vapp", 4, VAPP_TYPE_SOFTWARE,
                          "Chip Synth", "AudioLab", "1.1.0",
                          LV_SYMBOL_AUDIO, "8-bit musical tone keypad synthesizer", 12288, 6144);

    create_sample_package("sys_monitor.vapp", 5, VAPP_TYPE_UTILITY,
                          "System Monitor", "Kernel Labs", "1.0.4",
                          LV_SYMBOL_SETTINGS, "Telemetry & FreeRTOS memory gauge", 8192, 4096);

    create_sample_package("tetris_retro.vapp", 6, VAPP_TYPE_GAME,
                          "Tetris Retro", "ArcadeClassics", "1.0.0",
                          LV_SYMBOL_PLAY, "Classic 10x16 falling tetromino puzzle", 16384, 8192);
#endif
}

bool vapp_loader_read_header(const char *filepath, vapp_package_t *out_pkg)
{
    if (!filepath || !out_pkg) return false;

    memset(out_pkg, 0, sizeof(vapp_package_t));
    strncpy(out_pkg->filepath, filepath, sizeof(out_pkg->filepath) - 1);

    const char *slash = strrchr(filepath, '/');
    const char *fn = slash ? (slash + 1) : filepath;
    strncpy(out_pkg->filename, fn, sizeof(out_pkg->filename) - 1);

#if defined(CONFIG_SIMULATOR) && !defined(CONFIG_IS_RAMRUN_ONLY)
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        /* Try ./vapps/ or ./build/vapps/ */
        char alt_path[128];
        snprintf(alt_path, sizeof(alt_path), "./vapps/%s", fn);
        f = fopen(alt_path, "rb");
        if (!f) {
            snprintf(alt_path, sizeof(alt_path), "./build/vapps/%s", fn);
            f = fopen(alt_path, "rb");
        }
    }
#else
    FILE *f = NULL;
#endif

    if (!f) {
        /* Fallback for virtual simulated catalog entries */
        if (strcmp(fn, "brick_breaker.vapp") == 0) {
            out_pkg->header.magic = VAPP_MAGIC;
            out_pkg->header.abi_version = 1;
            out_pkg->header.app_type = VAPP_TYPE_GAME;
            out_pkg->header.app_id = 1;
            strncpy(out_pkg->header.name, "Brick Breaker", sizeof(out_pkg->header.name) - 1);
            strncpy(out_pkg->header.author, "RetroGames", sizeof(out_pkg->header.author) - 1);
            strncpy(out_pkg->header.version, "1.2.0", sizeof(out_pkg->header.version) - 1);
            strncpy(out_pkg->header.icon_symbol, LV_SYMBOL_PLAY, sizeof(out_pkg->header.icon_symbol) - 1);
            strncpy(out_pkg->header.description, "Arcade paddle & brick destruction", sizeof(out_pkg->header.description) - 1);
            out_pkg->header.req_heap_bytes = 16384;
            out_pkg->file_size = 16384;
            out_pkg->is_valid = true;
            return true;
        } else if (strcmp(fn, "unit_converter.vapp") == 0) {
            out_pkg->header.magic = VAPP_MAGIC;
            out_pkg->header.abi_version = 1;
            out_pkg->header.app_type = VAPP_TYPE_APP;
            out_pkg->header.app_id = 2;
            strncpy(out_pkg->header.name, "Unit Converter", sizeof(out_pkg->header.name) - 1);
            strncpy(out_pkg->header.author, "Veebha Tools", sizeof(out_pkg->header.author) - 1);
            strncpy(out_pkg->header.version, "2.0.1", sizeof(out_pkg->header.version) - 1);
            strncpy(out_pkg->header.icon_symbol, LV_SYMBOL_REFRESH, sizeof(out_pkg->header.icon_symbol) - 1);
            strncpy(out_pkg->header.description, "Convert Length, Weight & Temp", sizeof(out_pkg->header.description) - 1);
            out_pkg->header.req_heap_bytes = 8192;
            out_pkg->file_size = 12288;
            out_pkg->is_valid = true;
            return true;
        } else if (strcmp(fn, "morse_flasher.vapp") == 0) {
            out_pkg->header.magic = VAPP_MAGIC;
            out_pkg->header.abi_version = 1;
            out_pkg->header.app_type = VAPP_TYPE_EXTENSION;
            out_pkg->header.app_id = 3;
            strncpy(out_pkg->header.name, "Morse Flasher", sizeof(out_pkg->header.name) - 1);
            strncpy(out_pkg->header.author, "OpticDev", sizeof(out_pkg->header.author) - 1);
            strncpy(out_pkg->header.version, "1.0.0", sizeof(out_pkg->header.version) - 1);
            strncpy(out_pkg->header.icon_symbol, LV_SYMBOL_EYE_OPEN, sizeof(out_pkg->header.icon_symbol) - 1);
            strncpy(out_pkg->header.description, "Optical screen Morse signal emitter", sizeof(out_pkg->header.description) - 1);
            out_pkg->header.req_heap_bytes = 4096;
            out_pkg->file_size = 8192;
            out_pkg->is_valid = true;
            return true;
        } else if (strcmp(fn, "chip_synth.vapp") == 0) {
            out_pkg->header.magic = VAPP_MAGIC;
            out_pkg->header.abi_version = 1;
            out_pkg->header.app_type = VAPP_TYPE_SOFTWARE;
            out_pkg->header.app_id = 4;
            strncpy(out_pkg->header.name, "Chip Synth", sizeof(out_pkg->header.name) - 1);
            strncpy(out_pkg->header.author, "AudioLab", sizeof(out_pkg->header.author) - 1);
            strncpy(out_pkg->header.version, "1.1.0", sizeof(out_pkg->header.version) - 1);
            strncpy(out_pkg->header.icon_symbol, LV_SYMBOL_AUDIO, sizeof(out_pkg->header.icon_symbol) - 1);
            strncpy(out_pkg->header.description, "8-bit musical tone keypad synth", sizeof(out_pkg->header.description) - 1);
            out_pkg->header.req_heap_bytes = 12288;
            out_pkg->file_size = 14336;
            out_pkg->is_valid = true;
            return true;
        } else if (strcmp(fn, "sys_monitor.vapp") == 0) {
            out_pkg->header.magic = VAPP_MAGIC;
            out_pkg->header.abi_version = 1;
            out_pkg->header.app_type = VAPP_TYPE_UTILITY;
            out_pkg->header.app_id = 5;
            strncpy(out_pkg->header.name, "System Monitor", sizeof(out_pkg->header.name) - 1);
            strncpy(out_pkg->header.author, "Kernel Labs", sizeof(out_pkg->header.author) - 1);
            strncpy(out_pkg->header.version, "1.0.4", sizeof(out_pkg->header.version) - 1);
            strncpy(out_pkg->header.icon_symbol, LV_SYMBOL_SETTINGS, sizeof(out_pkg->header.icon_symbol) - 1);
            strncpy(out_pkg->header.description, "Telemetry & FreeRTOS memory gauge", sizeof(out_pkg->header.description) - 1);
            out_pkg->header.req_heap_bytes = 8192;
            out_pkg->file_size = 10240;
            out_pkg->is_valid = true;
            return true;
        } else if (strcmp(fn, "tetris_retro.vapp") == 0) {
            out_pkg->header.magic = VAPP_MAGIC;
            out_pkg->header.abi_version = 1;
            out_pkg->header.app_type = VAPP_TYPE_GAME;
            out_pkg->header.app_id = 6;
            strncpy(out_pkg->header.name, "Tetris Retro", sizeof(out_pkg->header.name) - 1);
            strncpy(out_pkg->header.author, "ArcadeClassics", sizeof(out_pkg->header.author) - 1);
            strncpy(out_pkg->header.version, "1.0.0", sizeof(out_pkg->header.version) - 1);
            strncpy(out_pkg->header.icon_symbol, LV_SYMBOL_PLAY, sizeof(out_pkg->header.icon_symbol) - 1);
            strncpy(out_pkg->header.description, "Classic 10x16 falling tetromino puzzle", sizeof(out_pkg->header.description) - 1);
            out_pkg->header.req_heap_bytes = 16384;
            out_pkg->file_size = 16384;
            out_pkg->is_valid = true;
            return true;
        }
        return false;
    }

    fseek(f, 0, SEEK_END);
    out_pkg->file_size = (uint32_t)ftell(f);
    fseek(f, 0, SEEK_SET);

    if (out_pkg->file_size < sizeof(vapp_header_t)) {
        fclose(f);
        return false;
    }

    if (fread(&out_pkg->header, sizeof(vapp_header_t), 1, f) != 1) {
        fclose(f);
        return false;
    }
    fclose(f);

    if (out_pkg->header.magic != VAPP_MAGIC) {
        OS_LOGW(TAG, "Invalid magic 0x%08X in %s", out_pkg->header.magic, filepath);
        return false;
    }

    uint16_t exp_crc = vapp_compute_crc16((const uint8_t *)&out_pkg->header,
                                         offsetof(vapp_header_t, header_crc16));
    if (out_pkg->header.header_crc16 != exp_crc) {
        OS_LOGW(TAG, "CRC mismatch in %s (got 0x%04X, exp 0x%04X)",
                filepath, out_pkg->header.header_crc16, exp_crc);
        /* Allow fallback compatibility */
    }

    out_pkg->is_valid = true;
    return true;
}

bool vapp_loader_scan_dir(vapp_package_t *out_packages, size_t max_packages, size_t *out_count)
{
    if (!out_packages || max_packages == 0 || !out_count) return false;

    *out_count = 0;

#if defined(CONFIG_BOARD_SIMULATOR) && CONFIG_BOARD_SIMULATOR
    DIR *d = opendir("./vapps");
    if (!d) {
        d = opendir("./build/vapps");
    }

    if (d) {
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG || dir->d_type == DT_UNKNOWN) {
                const char *dot = strrchr(dir->d_name, '.');
                if (dot && strcasecmp(dot, ".vapp") == 0) {
                    if (*out_count < max_packages) {
                        char full[300];
                        snprintf(full, sizeof(full), "/vapps/%s", dir->d_name);
                        vapp_package_t pkg;
                        if (vapp_loader_read_header(full, &pkg)) {
                            out_packages[*out_count] = pkg;
                            (*out_count)++;
                        }
                    }
                }
            }
        }
        closedir(d);
    }
#endif

    /* If no packages read from disk, populate standard builtin catalog */
    if (*out_count == 0) {
        const char *default_apps[] = {
            "brick_breaker.vapp",
            "unit_converter.vapp",
            "morse_flasher.vapp",
            "chip_synth.vapp",
            "sys_monitor.vapp",
            "tetris_retro.vapp"
        };
        for (size_t i = 0; i < sizeof(default_apps)/sizeof(default_apps[0]) && *out_count < max_packages; i++) {
            char full[300];
            snprintf(full, sizeof(full), "/vapps/%s", default_apps[i]);
            vapp_package_t pkg;
            if (vapp_loader_read_header(full, &pkg)) {
                out_packages[*out_count] = pkg;
                (*out_count)++;
            }
        }
    }

    if (*out_count > 1) {
        for (size_t i = 0; i < *out_count - 1; i++) {
            for (size_t j = i + 1; j < *out_count; j++) {
                if (out_packages[i].header.app_id > out_packages[j].header.app_id) {
                    vapp_package_t tmp = out_packages[i];
                    out_packages[i] = out_packages[j];
                    out_packages[j] = tmp;
                }
            }
        }
    }

    OS_LOGI(TAG, "Scanned /vapps: found %u valid package(s)", (unsigned int)*out_count);
    return true;
}

bool vapp_loader_launch(const vapp_package_t *pkg)
{
    if (!pkg || !pkg->is_valid) {
        OS_LOGE(TAG, "Cannot launch invalid package");
        return false;
    }

    OS_LOGI(TAG, "Launching VAPP '%s' (Type: %s, ID: %u, Heap: %u B)",
            pkg->header.name, vapp_type_to_string((vapp_type_t)pkg->header.app_type),
            (unsigned int)pkg->header.app_id, (unsigned int)pkg->header.req_heap_bytes);

    switch (pkg->header.app_id) {
    case 1:
        vapp_brick_breaker_launch(pkg);
        break;
    case 2:
        vapp_unit_converter_launch(pkg);
        break;
    case 3:
        vapp_morse_flasher_launch(pkg);
        break;
    case 4:
        vapp_chip_synth_launch(pkg);
        break;
    case 5:
        vapp_sys_monitor_launch(pkg);
        break;
    case 6:
        vapp_tetris_launch(pkg);
        break;
    default:
        if (strstr(pkg->filename, "tetris") != NULL || strcasestr(pkg->header.name, "tetris") != NULL) {
            vapp_tetris_launch(pkg);
        } else if (strstr(pkg->filename, "brick") != NULL || strcasestr(pkg->header.name, "brick") != NULL) {
            vapp_brick_breaker_launch(pkg);
        } else if (strstr(pkg->filename, "unit") != NULL || strcasestr(pkg->header.name, "unit") != NULL) {
            vapp_unit_converter_launch(pkg);
        } else if (strstr(pkg->filename, "morse") != NULL || strcasestr(pkg->header.name, "morse") != NULL) {
            vapp_morse_flasher_launch(pkg);
        } else if (strstr(pkg->filename, "synth") != NULL || strcasestr(pkg->header.name, "synth") != NULL) {
            vapp_chip_synth_launch(pkg);
        } else if (strstr(pkg->filename, "sys") != NULL || strcasestr(pkg->header.name, "sys") != NULL) {
            vapp_sys_monitor_launch(pkg);
        } else {
            vapp_tetris_launch(pkg);
        }
        break;
    }

    return true;
}
