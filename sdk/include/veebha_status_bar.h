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

#ifndef SDK_INCLUDE_VEEBHA_STATUS_BAR_H
#define SDK_INCLUDE_VEEBHA_STATUS_BAR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * Global Status Bar Indicator State Structure
 */
typedef struct {
    uint8_t  hours;
    uint8_t  minutes;
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    bool     bt_enabled;
    bool     bt_connected;
    bool     tethering_active;
    uint8_t  usb_mode;           /* os_usb_mode_t */
    bool     silent_mode;
    bool     alarm_enabled;
    uint8_t  battery_level;      /* 0-100 */
    bool     is_charging;
} status_bar_global_state_t;

/**
 * Create a persistent 18px status bar adhering to the VeebhaOS Chrome specification.
 * Dedicated strictly to hardware and system indicators:
 *  - Left Tray (Zone 1): Stepped cellular signal meter + "4G" network badge + Tethering icon (LV_SYMBOL_WIFI).
 *  - Center Tray (Zone 2): Digital Clock (Montserrat 12, pinned center).
 *  - Right Tray (Zone 3 - Prioritized Max 3 optional + Battery):
 *      0. alarm_icon: Alarm Armed glyph (LV_SYMBOL_BELL, Amber).
 *      1. bt_icon: Standard Bluetooth glyph (LV_SYMBOL_BLUETOOTH).
 *      2. headset_icon: Audio headset glyph (LV_SYMBOL_AUDIO, Cyan).
 *      3. usb_icon: USB Mass Storage / Cable glyph (LV_SYMBOL_USB, Green).
 *      4. mute_icon: Silent/Mute glyph (LV_SYMBOL_MUTE, Orange).
 *      5. bat_box: Battery indicator with level bar and charging bolt (LV_SYMBOL_CHARGE).
 *
 * @param parent Parent container (typically the root screen container).
 * @param title  Legacy title argument (unused; titles are placed in dedicated sub-headers).
 * @return Pointer to the status bar lv_obj_t.
 */
lv_obj_t * status_bar_create(lv_obj_t *parent, const char *title);

/**
 * Update the title label on an active status bar (no-op; titles are displayed in sub-headers).
 *
 * @param bar   Status bar object created with status_bar_create.
 * @param title New title string.
 */
void status_bar_set_title(lv_obj_t *bar, const char *title);

/**
 * Update the digital time string on an active status bar.
 *
 * @param bar      Status bar object.
 * @param time_str Formatted time string (e.g., "12:00").
 */
void status_bar_set_time(lv_obj_t *bar, const char *time_str);

/**
 * Update status indicator icons (Bluetooth, Silent) across all status bars.
 *
 * @param bar          Status bar object (or NULL for all).
 * @param bluetooth_on Show/hide Bluetooth icon.
 * @param silent_on    Show/hide Silent/Mute icon.
 */
void status_bar_set_indicators(lv_obj_t *bar, bool bluetooth_on, bool silent_on);

/**
 * Update the digital clock with hour and minute integers (formatted as "%02d:%02d").
 *
 * @param bar   Status bar object (or NULL for all).
 * @param hours Hour (0-23).
 * @param mins  Minute (0-59).
 */
void status_bar_update_clock(lv_obj_t *bar, uint8_t hours, uint8_t mins);

/**
 * Retrieve current persistent RTC time.
 *
 * @param hours Pointer to write hour (0-23) or NULL.
 * @param mins  Pointer to write minute (0-59) or NULL.
 */
void status_bar_get_rtc_time(uint8_t *hours, uint8_t *mins);

/**
 * Set current persistent RTC time.
 *
 * @param hours Hour (0-23).
 * @param mins  Minute (0-59).
 */
void status_bar_set_rtc_time(uint8_t hours, uint8_t mins);

/**
 * Retrieve current persistent RTC date.
 *
 * @param year  Pointer to write year (e.g. 2026) or NULL.
 * @param month Pointer to write month (1-12) or NULL.
 * @param day   Pointer to write day (1-31) or NULL.
 */
void status_bar_get_rtc_date(uint16_t *year, uint8_t *month, uint8_t *day);

/**
 * Set current persistent RTC date.
 *
 * @param year  Year (2020-2099).
 * @param month Month (1-12).
 * @param day   Day (1-31).
 */
void status_bar_set_rtc_date(uint16_t year, uint8_t month, uint8_t day);

/**
 * Show or hide the floating translucent corner battery HUD on lv_layer_top().
 *
 * @param visible True to show battery HUD, false to hide.
 */
void status_bar_set_battery_hud_visible(bool visible);

/**
 * Check if the floating corner battery HUD is currently visible.
 */
bool status_bar_is_battery_hud_visible(void);

/**
 * Set global Bluetooth state and update all status bars.
 *
 * @param enabled   True if Bluetooth is turned on.
 * @param connected True if actively connected to a peripheral.
 */
void status_bar_set_bt_state(bool enabled, bool connected);

/**
 * Set global USB mode and update all status bars.
 *
 * @param mode USB connection mode (0: none, 2: mass storage, 3: tethering, etc.).
 */
void status_bar_set_usb_mode(uint8_t mode);

/**
 * Set global tethering state and update all status bars.
 *
 * @param enabled True if USB or BT tethering hotspot is active.
 */
void status_bar_set_tethering(bool enabled);

/**
 * Set global silent/mute profile state and update all status bars.
 *
 * @param silent True if profile is Silent.
 */
void status_bar_set_silent(bool silent);

/**
 * Set global alarm armed state and update all status bars.
 *
 * @param enabled True if alarm is armed.
 */
void status_bar_set_alarm(bool enabled);

/**
 * Set global battery level and charging state and update all status bars.
 *
 * @param level       Battery percentage (0-100).
 * @param is_charging True if battery is actively charging.
 */
void status_bar_set_battery(uint8_t level, bool is_charging);

/**
 * Retrieve pointer to current global status bar state.
 */
const status_bar_global_state_t * status_bar_get_global_state(void);

/**
 * Synchronize and re-render all status bar widgets on registered active screens.
 */
void status_bar_update_all(void);

#ifdef __cplusplus
}
#endif

#endif /* SDK_INCLUDE_VEEBHA_STATUS_BAR_H */
