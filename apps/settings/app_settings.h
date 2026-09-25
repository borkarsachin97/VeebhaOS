/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef APPS_SETTINGS_APP_SETTINGS_H
#define APPS_SETTINGS_APP_SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SOUND_PROFILE_GENERAL = 0,
    SOUND_PROFILE_SILENT = 1,
    SOUND_PROFILE_OUTDOOR = 2,
    SOUND_PROFILE_COUNT
} sound_profile_t;

/**
 * Initialize settings subsystem and load parameters from NVRAM.
 */
void app_settings_init(void);

/**
 * Open the main Settings application view and push it onto the window manager stack.
 */
void app_settings_open(void);

/**
 * Invalidate cached settings screen (e.g. after language or theme switch).
 */
void app_settings_invalidate(void);

/**
 * Retrieve the active sound profile.
 */
sound_profile_t app_settings_get_profile(void);

/**
 * Set the active sound profile and synchronize with status bar and NVRAM.
 */
void app_settings_set_profile(sound_profile_t profile);

/**
 * Toggle between General and Silent profile (used by Standby '#' shortcut).
 */
void app_settings_toggle_silent(void);

/**
 * Check if silent profile is currently active.
 */
bool app_settings_is_silent(void);

/**
 * Retrieve master volume level (1-7).
 */
uint8_t app_settings_get_volume(void);

/**
 * Set master volume level (1-7).
 */
void app_settings_set_volume(uint8_t vol);

#ifdef __cplusplus
}
#endif

#endif /* APPS_SETTINGS_APP_SETTINGS_H */
