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

#ifndef SDK_INCLUDE_APP_REGISTRY_H
#define SDK_INCLUDE_APP_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define OS_MAX_REGISTERED_APPS 16

typedef struct {
    const char *id;
    const char *name;
    const void *icon;
    void (*launch_cb)(void);
    uint8_t priority;
} os_app_desc_t;

/**
 * Initialize the application registry and register default core apps.
 */
void os_app_registry_init(void);

/**
 * Register an application descriptor.
 *
 * @param desc Pointer to app descriptor.
 * @return true on success, false if registry full.
 */
bool os_app_register(const os_app_desc_t *desc);

/**
 * Get total number of registered apps.
 */
uint16_t os_app_get_count(void);

/**
 * Retrieve registered app by index (0-based).
 */
const os_app_desc_t * os_app_get_by_index(uint16_t index);

/**
 * Retrieve registered app by unique string identifier.
 */
const os_app_desc_t * os_app_find_by_id(const char *id);

/* ============================================================================
 * Quick Settings Tile Registry
 * ============================================================================ */

#define OS_MAX_QS_TILES 8

typedef struct {
    const char *id;
    const char *name;
    const char *icon;
    bool (*is_active_cb)(void);
    void (*get_status_cb)(char *buf, size_t buf_len);
    void (*toggle_cb)(void);
    uint8_t priority;
} os_qs_tile_desc_t;

/**
 * Register a Quick Settings tile.
 */
bool os_qs_tile_register(const os_qs_tile_desc_t *tile);

/**
 * Get total number of registered QS tiles.
 */
uint8_t os_qs_tile_get_count(void);

/**
 * Retrieve registered QS tile by index (0-based).
 */
const os_qs_tile_desc_t * os_qs_tile_get_by_index(uint8_t index);

/**
 * Retrieve registered QS tile by unique string identifier.
 */
const os_qs_tile_desc_t * os_qs_tile_find_by_id(const char *id);

#ifdef __cplusplus
}
#endif

#endif /* SDK_INCLUDE_APP_REGISTRY_H */
