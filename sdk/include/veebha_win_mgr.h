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
#define WIN_MGR_LABEL_MAX 32
#define OS_MAX_TASKS 5

typedef enum {
    VEEBHA_VIEW_TYPE_GENERIC = 0,
    VEEBHA_VIEW_TYPE_LIST,
    VEEBHA_VIEW_TYPE_GRID,
    VEEBHA_VIEW_TYPE_EDITOR,
    VEEBHA_VIEW_TYPE_MEDIA,
    VEEBHA_VIEW_TYPE_DIALER,
    VEEBHA_VIEW_TYPE_IDLE,
    VEEBHA_VIEW_TYPE_CALC,
    VEEBHA_VIEW_TYPE_CALENDAR,
} veebha_view_type_t;

/**
 * Screen Fullscreen Display Modes
 */
typedef enum {
    OS_FULLSCREEN_NONE = 0,    /* 18px Top Bar + 184px Viewport + 20px Softkeys */
    OS_FULLSCREEN_PARTIAL = 1, /* Hide Top Status Bar (200px Viewport + 20px Softkeys) */
    OS_FULLSCREEN_FULL = 2     /* Hide Top Bar & Softkey Bar (Full 176x220 Viewport) */
} os_fullscreen_mode_t;

/**
 * Standard Header for Screen user_data structure
 * All screen contexts must place this header at offset 0.
 */
typedef struct {
    lv_obj_t            *softkey_bar;
    lv_obj_t            *first_item;
    veebha_view_type_t   view_type;
    char                 title[WIN_MGR_LABEL_MAX];
    os_fullscreen_mode_t fullscreen_mode;
    bool                 show_battery_hud;
    bool                 keep_alive;
} win_mgr_screen_hdr_t;


/**
 * Window Manager Stack Entry Descriptor
 */
typedef struct {
    lv_obj_t            *screen;
    lv_obj_t            *focused_obj;
    veebha_view_type_t   view_type;
    char                 title[WIN_MGR_LABEL_MAX];
    char                 lsk_label[WIN_MGR_LABEL_MAX];
    softkey_callback_t   lsk_cb;
    char                 rsk_label[WIN_MGR_LABEL_MAX];
    softkey_callback_t   rsk_cb;
    os_fullscreen_mode_t fullscreen_mode;
    bool                 show_battery_hud;
} win_mgr_entry_t;

/**
 * Operating System Task Entry
 */
typedef struct {
    uint8_t   id;
    char      name[24];
    lv_obj_t *screen_obj;
    bool      is_backgrounded;
} os_task_entry_t;

/**
 * Task Pool Management APIs
 */
uint8_t win_mgr_get_task_count(void);
os_task_entry_t * win_mgr_get_task(uint8_t index);
const char * win_mgr_get_task_title(uint8_t index);
os_task_entry_t * win_mgr_find_task(const char *name);
bool win_mgr_task_resume(const char *name);
bool win_mgr_task_activate(uint8_t task_idx);
bool win_mgr_task_kill(uint8_t task_idx);
os_task_entry_t * win_mgr_get_active_task(void);

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
 * Return directly to the Home screen (Launcher) without popping background apps.
 * Dismisses active overlays and dialogs, and brings Launcher to the foreground.
 */
void win_mgr_show_home(void);

/**
 * Check if the stack entry at index corresponds to the Root / Home Launcher.
 *
 * @param index Stack index.
 * @return true if entry is Home/Launcher.
 */
bool win_mgr_is_home(uint8_t index);

/**
 * Retrieve the current stack depth (0 if uninitialized, 1 = root screen only).
 */
uint8_t win_mgr_get_depth(void);

/**
 * Retrieve the top entry on the stack.
 */
win_mgr_entry_t * win_mgr_get_top(void);

/**
 * Retrieve stack entry by index (0 = Home/Root).
 */
win_mgr_entry_t * win_mgr_get_entry(uint8_t index);

/**
 * Switch active focus to a stack entry by index, re-ordering it to the top.
 *
 * @param index Index of the stack entry to bring to the front.
 * @return true on success, false if index invalid.
 */
bool win_mgr_switch_to(uint8_t index);

/**
 * Close and terminate a stack entry by index, freeing its resources.
 * Cannot close root screen (index 0).
 *
 * @param index Index of the stack entry to close.
 * @return true on success, false if root or index invalid.
 */
bool win_mgr_close_at(uint8_t index);

/**
 * Set the human-readable title of the active top screen.
 */
void win_mgr_set_title(const char *title);

/**
 * Retrieve the title of a screen entry by index.
 */
const char * win_mgr_get_title(uint8_t index);

/**
 * Retrieve the active screen's view type (List, Grid, Generic).
 */
veebha_view_type_t win_mgr_get_active_view_type(void);

/**
 * Set the view type of the active top screen.
 */
void win_mgr_set_active_view_type(veebha_view_type_t type);

/**
 * Retrieve the global LVGL keypad input group.
 */
lv_group_t * win_mgr_get_group(void);

/**
 * Iterate across all registered screens (Home screen + all task screen stacks).
 *
 * @param cb Callback function called for each active screen.
 * @param user_data Optional user pointer passed to callback.
 */
void win_mgr_for_each_screen(void (*cb)(lv_obj_t *screen, void *user_data), void *user_data);

/**
 * Configure the fullscreen mode and battery HUD visibility for a screen.
 *
 * @param screen Target screen object.
 * @param mode Fullscreen mode (NONE, PARTIAL, FULL).
 * @param show_battery_hud True to display floating top-right battery HUD.
 */
void win_mgr_set_fullscreen_mode(lv_obj_t *screen, os_fullscreen_mode_t mode, bool show_battery_hud);

/**
 * Retrieve the active screen's fullscreen mode.
 */
os_fullscreen_mode_t win_mgr_get_fullscreen_mode(void);

/**
 * Check if the active screen has floating battery HUD enabled.
 */
bool win_mgr_is_battery_hud_enabled(void);

/**
 * Register the singleton persistent Launcher (Main Menu) screen.
 */
void win_mgr_set_launcher_screen(lv_obj_t *screen);

/**
 * Retrieve the singleton persistent Launcher screen, if registered.
 */
lv_obj_t * win_mgr_get_launcher_screen(void);

/**
 * Check if a screen is currently part of any task stack in Window Manager.
 */
bool win_mgr_is_screen_in_stack(lv_obj_t *screen);

#ifdef __cplusplus
}
#endif

#endif /* SDK_INCLUDE_VEEBHA_WIN_MGR_H */
