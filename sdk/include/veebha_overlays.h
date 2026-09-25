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

#ifndef SDK_INCLUDE_VEEBHA_OVERLAYS_H
#define SDK_INCLUDE_VEEBHA_OVERLAYS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "drivers/hal_input.h"
#include <stdbool.h>
#include <stdint.h>

/* ============================================================================
 * KitKat Dual-Page Shade (Notification Panel & Quick Settings)
 * ============================================================================ */

typedef enum {
    NOTIF_PAGE_DECK = 0,
    NOTIF_PAGE_QUICK_SETTINGS = 1,
    NOTIF_PAGE_COUNT
} notif_page_t;

/**
 * Open the Quick Settings / Notifications shade on lv_layer_top().
 */
void notif_panel_show(void);

/**
 * Close and destroy the shade, restoring focus and softkeys.
 */
void notif_panel_close(void);

/**
 * Destroy the cached shade completely.
 */
void notif_panel_destroy(void);

/**
 * Toggle the visibility of the shade.
 */
void notif_panel_toggle(void);

/**
 * Check if the shade is currently open.
 */
bool notif_panel_is_active(void);

/**
 * Get current active shade page (0 = Notifications, 1 = Quick Settings).
 */
uint8_t notif_panel_get_page(void);

/**
 * Switch active shade page (0 = Notifications, 1 = Quick Settings).
 */
void notif_panel_set_page(uint8_t page);

/**
 * Post an active system alert notification to the notification panel.
 */
void notif_panel_post_alert(const char *title, const char *msg);

/**
 * Clear all active notifications from the deck.
 */
void notif_panel_clear_all(void);

/**
 * Retrieve count of active notifications in the deck.
 */
uint16_t notif_panel_get_count(void);

/**
 * Retrieve the most recent alert title and message.
 */
const char * notif_panel_get_last_alert_title(void);
const char * notif_panel_get_last_alert_msg(void);

/**
 * Handle physical keypad key navigation inside the shade.
 */
void notif_panel_handle_key(veebha_key_t key);

/* ============================================================================
 * Multitasking Manager (Task Switcher)
 * ============================================================================ */

/**
 * Open the Multitasking Manager carousel on lv_layer_top().
 */
void task_mgr_show(void);

/**
 * Close and destroy the Multitasking Manager, restoring focus and softkeys.
 */
void task_mgr_close(void);

/**
 * Toggle the visibility of the Multitasking Manager.
 */
void task_mgr_toggle(void);

/**
 * Check if the Multitasking Manager is currently open.
 */
bool task_mgr_is_active(void);

#ifdef __cplusplus
}
#endif

#endif /* SDK_INCLUDE_VEEBHA_OVERLAYS_H */
