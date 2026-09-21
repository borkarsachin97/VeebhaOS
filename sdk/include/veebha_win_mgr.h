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

#ifndef SDK_INCLUDE_VEEBHA_WIN_MGR_H
#define SDK_INCLUDE_VEEBHA_WIN_MGR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "veebha_softkeys.h"
#include <stdbool.h>
#include <stdint.h>

#define WIN_MGR_MAX_DEPTH 8
#define WIN_MGR_LABEL_MAX 16

/**
 * Window Manager Stack Entry Descriptor
 */
typedef struct {
    lv_obj_t           *screen;
    lv_obj_t           *focused_obj;
    char                lsk_label[WIN_MGR_LABEL_MAX];
    softkey_callback_t  lsk_cb;
    char                rsk_label[WIN_MGR_LABEL_MAX];
    softkey_callback_t  rsk_cb;
} win_mgr_entry_t;

/**
 * Initialize the Window Navigation Stack with a global keypad focus group.
 *
 * @param group Global LVGL input group associated with the keypad driver.
 */
void win_mgr_init(lv_group_t *group);

/**
 * Push a new screen onto the LIFO navigation stack.
 * Saves the current screen's focus state, loads the new screen, and applies its softkey actions.
 *
 * @param screen Root lv_obj_t pointer of the new screen.
 * @param lsk    Left Softkey label (or NULL).
 * @param lsk_cb Left Softkey action callback (or NULL).
 * @param rsk    Right Softkey label (or NULL).
 * @param rsk_cb Right Softkey action callback (or NULL).
 * @return true on success, false if stack depth exceeded.
 */
bool win_mgr_push(lv_obj_t *screen,
                  const char *lsk, softkey_callback_t lsk_cb,
                  const char *rsk, softkey_callback_t rsk_cb);

/**
 * Pop the active screen from the LIFO navigation stack.
 * Destroys the popped screen (freeing all LVGL heap allocations), restores the previous
 * screen, re-applies its softkey actions, and restores the previously focused widget.
 *
 * @return true if popped successfully, false if at root level (depth <= 1).
 */
bool win_mgr_pop(void);

/**
 * Clear the entire stack down to index 0 (Home/Root screen).
 * Pops and frees all intermediate screens.
 */
void win_mgr_reset_to_home(void);

/**
 * Retrieve the current stack depth (0 if uninitialized, 1 = root screen only).
 */
uint8_t win_mgr_get_depth(void);

/**
 * Retrieve the top entry on the stack.
 */
win_mgr_entry_t * win_mgr_get_top(void);

/**
 * Retrieve the global LVGL keypad input group.
 */
lv_group_t * win_mgr_get_group(void);

#ifdef __cplusplus
}
#endif

#endif /* SDK_INCLUDE_VEEBHA_WIN_MGR_H */
