/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef SDK_CORE_VAPP_LOADER_H
#define SDK_CORE_VAPP_LOADER_H

#include "sdk/include/veebha_vapp.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_VAPPS_CAPACITY 64

/**
 * Initialize VAPP subsystem and ensure package directory exists.
 */
void vapp_loader_init(void);

/**
 * Scan /vapps directory strictly for .vapp application packages.
 *
 * @param out_packages Array of vapp_package_t to populate.
 * @param max_packages Capacity of output array.
 * @param out_count Output pointer receiving number of packages found.
 * @return true if scan succeeded, false on error.
 */
bool vapp_loader_scan_dir(vapp_package_t *out_packages, size_t max_packages, size_t *out_count);

/**
 * Parse and validate a .vapp package header.
 *
 * @param filepath Full path to .vapp file (must start with /vapps or ./vapps).
 * @param out_pkg Output structure populated with package info.
 * @return true if header is valid, false otherwise.
 */
bool vapp_loader_read_header(const char *filepath, vapp_package_t *out_pkg);

/**
 * Launch and execute a .vapp application package into the VeebhaOS Window Manager.
 *
 * @param pkg Pointer to valid vapp_package_t.
 * @return true if launched successfully, false on error.
 */
bool vapp_loader_launch(const vapp_package_t *pkg);

/**
 * Helper to compute CCITT CRC16 for package validation.
 */
uint16_t vapp_compute_crc16(const uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* SDK_CORE_VAPP_LOADER_H */
