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

#ifndef SDK_INCLUDE_VEEBHA_SOFTKEYS_H
#define SDK_INCLUDE_VEEBHA_SOFTKEYS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

typedef void (*softkey_callback_t)(void);

/**
 * Configure the active Left and Right Softkey text labels and execution callbacks.
 *
 * @param lsk_label Text for Left Softkey (e.g., "Select", "Options", "OK"). NULL to hide.
 * @param lsk_cb    Callback executed on physical LSK press. NULL for no-op.
 * @param rsk_label Text for Right Softkey (e.g., "Back", "Clear", "Exit"). NULL to hide.
 * @param rsk_cb    Callback executed on physical RSK press. NULL for no-op.
 */
void softkey_set_actions(const char *lsk_label, softkey_callback_t lsk_cb,
                         const char *rsk_label, softkey_callback_t rsk_cb);

/**
 * Trigger the registered Left Softkey callback (called by input driver).
 */
void softkey_trigger_lsk(void);

/**
 * Trigger the registered Right Softkey callback (called by input driver).
 */
void softkey_trigger_rsk(void);

/**
 * Configure an optional long-press callback for the Right Softkey (e.g. Clear All).
 */
void softkey_set_rsk_long_action(softkey_callback_t rsk_long_cb);

/**
 * Trigger the registered Right Softkey long-press callback.
 */
void softkey_trigger_rsk_long(void);

/**
 * Retrieve the current Right Softkey long-press callback.
 */
softkey_callback_t softkey_get_rsk_long_action(void);

/**
 * Create the persistent 20px fixed Softkey Bar within a parent container.
 *
 * @param parent Parent container (typically the root screen object).
 * @param lsk_label Initial Left Softkey label text.
 * @param rsk_label Initial Right Softkey label text.
 * @return Pointer to created softkey bar container.
 */
lv_obj_t * softkey_bar_create(lv_obj_t *parent, const char *lsk_label, const char *rsk_label);

/**
 * Set the currently active softkey bar widget for label updates.
 */
void softkey_bar_set_active_widget(lv_obj_t *bar);

#ifdef __cplusplus
}
#endif

#endif /* SDK_INCLUDE_VEEBHA_SOFTKEYS_H */
