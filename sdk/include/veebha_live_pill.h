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

#ifndef SDK_INCLUDE_VEEBHA_LIVE_PILL_H
#define SDK_INCLUDE_VEEBHA_LIVE_PILL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * Live Pill Priority Hierarchy
 * Higher numeric values take precedence over lower values.
 */
typedef enum {
    LIVE_PILL_PRIO_NONE = 0,
    LIVE_PILL_PRIO_INFO,       /* Generic system alert or status text */
    LIVE_PILL_PRIO_UTILITY,    /* Stopwatch, Timer, or Voice Recorder */
    LIVE_PILL_PRIO_RADIO,      /* FM Radio station & RDS info */
    LIVE_PILL_PRIO_MUSIC,      /* Walkman active track title */
    LIVE_PILL_PRIO_CALL,       /* Active background telephony call */
    LIVE_PILL_PRIO_COUNT
} live_pill_priority_t;

/**
 * Live Pill Click Callback
 */
typedef void (*live_pill_click_cb_t)(void);

/**
 * Initialize the Live Pill subsystem.
 */
void live_pill_init(void);

/**
 * Publish or update a live pill event at the specified priority level.
 *
 * @param prio     Priority tier of the event.
 * @param icon     Symbol icon string (e.g. LV_SYMBOL_CALL, LV_SYMBOL_AUDIO).
 * @param text     Display text (e.g. track title, call duration).
 * @param accent   Accent / border color for the pill capsule.
 * @param on_click Callback executed when the pill is clicked / OK pressed.
 */
void live_pill_publish(live_pill_priority_t prio, const char *icon, const char *text, lv_color_t accent, live_pill_click_cb_t on_click);

/**
 * Clear a publisher slot at the specified priority level.
 * Automatically falls back to the next highest active publisher, or hides the pill if none remain.
 *
 * @param prio Priority tier to clear.
 */
void live_pill_clear(live_pill_priority_t prio);

/**
 * Bind the Idle screen UI widgets to the live pill arbiter.
 *
 * @param container Capsule container object (160x24px).
 * @param icon_lbl  Icon label widget.
 * @param text_lbl  Text marquee label widget.
 */
void live_pill_bind_ui(lv_obj_t *container, lv_obj_t *icon_lbl, lv_obj_t *text_lbl);

/**
 * Unbind the Idle screen UI widgets (e.g. when Idle screen is deleted).
 */
void live_pill_unbind_ui(void);

/**
 * Refresh and synchronize the bound UI with the current highest priority publisher.
 */
void live_pill_refresh(void);

/**
 * Trigger the click action for the currently active live pill.
 */
void live_pill_trigger_click(void);

/**
 * Check if any live pill publisher is currently active.
 *
 * @return true if a live pill is active and visible.
 */
bool live_pill_is_active(void);

/**
 * Get the current active priority level.
 *
 * @return Highest active live_pill_priority_t.
 */
live_pill_priority_t live_pill_get_active_priority(void);

/**
 * Query current active text.
 */
const char * live_pill_get_active_text(void);

#ifdef __cplusplus
}
#endif

#endif /* SDK_INCLUDE_VEEBHA_LIVE_PILL_H */
