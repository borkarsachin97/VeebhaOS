/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef APPS_BROWSER_APP_BROWSER_H
#define APPS_BROWSER_APP_BROWSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdbool.h>

/**
 * Initialize Web Browser subsystem.
 */
void app_browser_init(void);

/**
 * Open Web Browser application screen.
 */
void app_browser_open(void);

/**
 * Load a specific URL into the active browser screen.
 */
void app_browser_load_url(const char *url);

/**
 * Check if the browser screen is currently active on top.
 */
bool app_browser_is_active(void);

#ifdef __cplusplus
}
#endif

#endif /* APPS_BROWSER_APP_BROWSER_H */
