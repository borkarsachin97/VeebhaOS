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

#include "lvgl.h"
#include "boards/board_config.h"
#include "drivers/hal_display.h"
#include "drivers/hal_input.h"
#include "veebha_win_mgr.h"
#include "veebha_templates.h"
#include "veebha_softkeys.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

static lv_group_t *g_keypad_group = NULL;

/* Level 2: Settings Submenu forward declaration */
static void open_settings_menu(void);

/* Level 1: 3x3 Grid Launcher Items */
static const tpl_grid_item_t s_launcher_items[] = {
    { .icon = LV_SYMBOL_CALL,      .title = "Phone" },
    { .icon = LV_SYMBOL_ENVELOPE,  .title = "Messages" },
    { .icon = LV_SYMBOL_LIST,      .title = "Contacts" },
    { .icon = LV_SYMBOL_IMAGE,     .title = "Gallery" },
    { .icon = LV_SYMBOL_AUDIO,     .title = "Music" },
    { .icon = LV_SYMBOL_SETTINGS,  .title = "Settings" },
    { .icon = LV_SYMBOL_BELL,      .title = "Calendar" },
    { .icon = LV_SYMBOL_DIRECTORY, .title = "Files" },
    { .icon = LV_SYMBOL_WIFI,      .title = "Tools" },
};

#define LAUNCHER_ITEM_COUNT (sizeof(s_launcher_items) / sizeof(s_launcher_items[0]))

/* Level 1: Main Launcher Handlers */
static void on_launcher_select(uint16_t index)
{
    if (index < LAUNCHER_ITEM_COUNT) {
        printf("[DEMO] Launcher Item %u (%s) Selected\n", index, s_launcher_items[index].title);
    }
    if (index == 5) {
        /* "Settings" selected -> Push Level 2 Settings List */
        open_settings_menu();
    } else {
        printf("[DEMO] App '%s' opened (stub)\n",
               (index < LAUNCHER_ITEM_COUNT) ? s_launcher_items[index].title : "Unknown");
    }
}

/* Level 2: Settings Handlers */
static void on_settings_select(uint16_t index)
{
    printf("[DEMO] Settings Menu Item %u Selected\n", index);
    if (index == 0) {
        printf("[DEMO] Display Settings selected\n");
    } else if (index == 1) {
        printf("[DEMO] Sound Settings selected\n");
    } else if (index == 2) {
        printf("[DEMO] About: %s (VeebhaOS v0.1.0)\n", CONFIG_BOARD_NAME);
    }
}

static void open_settings_menu(void)
{
    static const tpl_list_item_t s_settings_items[] = {
        { .icon = LV_SYMBOL_EYE_OPEN,   .title = "Display", .subtext = "Theme, Brightness" },
        { .icon = LV_SYMBOL_VOLUME_MAX, .title = "Sound",   .subtext = "Ringtone, Alert" },
        { .icon = LV_SYMBOL_SETTINGS,   .title = "About",   .subtext = "RDA8809 / Sim" },
    };

    tpl_list_view_t desc = {
        .title = "Settings",
        .items = s_settings_items,
        .count = sizeof(s_settings_items) / sizeof(s_settings_items[0]),
        .on_select = on_settings_select,
        .on_back = NULL, /* Defaults to win_mgr_pop() */
        .lsk_label = "Select",
        .rsk_label = "Back"
    };

    lv_obj_t *settings_scr = tpl_list_create(&desc);
    if (settings_scr) {
        win_mgr_push(settings_scr, "Select", tpl_list_default_lsk, "Back", tpl_list_default_rsk);
    }
}

static void setup_demo_app(void)
{
    tpl_grid_view_t desc = {
        .title = "VeebhaOS",
        .items = s_launcher_items,
        .count = LAUNCHER_ITEM_COUNT,
        .columns = 3,
        .on_select = on_launcher_select,
        .on_back = NULL,
        .lsk_label = "Select",
        .rsk_label = "Options"
    };

    lv_obj_t *launcher_scr = tpl_grid_create(&desc);
    if (launcher_scr) {
        win_mgr_push(launcher_scr, "Select", tpl_grid_default_lsk, "Options", NULL);
    }
}

/* Automated test key injection helper */
static void test_inject_key(veebha_key_t key)
{
    hal_input_push_event(key, VEEBHA_KEY_STATE_PRESSED);
    hal_input_push_event(key, VEEBHA_KEY_STATE_RELEASED);
}

/* Automated test helper to query current focused index */
static int test_get_focused_index(void)
{
    if (!g_keypad_group) return -1;
    lv_obj_t *focused = lv_group_get_focused(g_keypad_group);
    if (!focused) return -1;
    return (int)(uintptr_t)lv_obj_get_user_data(focused);
}

int main(int argc, char *argv[])
{
    printf("========================================================\n");
    printf("  %s\n", CONFIG_BOARD_NAME);
    printf("  Resolution: %dx%d (RGB565)\n", CONFIG_DISP_HOR_RES, CONFIG_DISP_VER_RES);
    printf("  Memory Pool: %u KB fence\n", (unsigned int)(CONFIG_LV_MEM_SIZE / 1024));
    printf("========================================================\n");

    bool automated_test = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--test") == 0 || strcmp(argv[i], "--headless") == 0) {
            automated_test = true;
            printf("[SIM] Running in automated Phase 2 verification mode\n");
        }
    }

    /* 1. Core LVGL Initialization */
    lv_init();
    lv_tick_set_cb(SDL_GetTicks);

    /* 2. Display HAL */
    if (!hal_display_init()) {
        fprintf(stderr, "[SIM] Failed to initialize display HAL\n");
        return 1;
    }

    /* 3. Input HAL */
    if (!hal_input_init()) {
        fprintf(stderr, "[SIM] Failed to initialize input HAL\n");
        hal_display_deinit();
        return 1;
    }

    /* 4. Setup Keypad Navigation Group & Window Manager */
    g_keypad_group = lv_group_create();
    lv_group_set_default(g_keypad_group);
    lv_indev_set_group(hal_input_get_lv_indev(), g_keypad_group);

    win_mgr_init(g_keypad_group);

    /* 5. Initialize Root Demo Screen (3x3 Grid Launcher) */
    setup_demo_app();

    printf("[SIM] Simulator ready.\n");
    printf("      Arrow Keys: 4-way D-pad navigation\n");
    printf("      Enter / LSK (F1 / Left Alt): Select item\n");
    printf("      RSK (F2 / Esc / Backspace): Back / Pop screen\n");

    /* 6. Main Interactive / Verification Loop */
    bool running = true;
    uint32_t frame_count = 0;

    while (running) {
        if (!hal_input_poll()) {
            running = false;
            break;
        }

        uint32_t sleep_ms = lv_timer_handler();
        if (sleep_ms < 5) sleep_ms = 5;
        if (sleep_ms > 33) sleep_ms = 33;

        SDL_Delay(sleep_ms);

        if (automated_test) {
            frame_count++;

            /* Automated verification sequence:
             * Frame  5: Verify initial Grid launcher (depth=1, focused=0 "Phone")
             * Frame 10: Inject RIGHT -> focus moves to 1 ("Messages")
             * Frame 15: Verify focused=1
             * Frame 17: Inject DOWN -> focus moves to 4 ("Music")
             * Frame 22: Verify focused=4
             * Frame 24: Inject RIGHT -> focus moves to 5 ("Settings")
             * Frame 29: Verify focused=5
             * Frame 32: Trigger LSK (Select Settings) -> pushes Level 2 (depth=2)
             * Frame 38: Verify in submenu (depth=2, view_type=LIST, focused=0 "Display")
             * Frame 42: Inject DOWN in list -> focus moves to 1 ("Sound")
             * Frame 47: Verify focused=1 in list
             * Frame 50: Trigger RSK to pop Settings submenu
             * Frame 56: Verify back in root menu (depth=1, view_type=GRID, focused=5 "Settings")
             * Frame 60: Test wrapping: Inject RIGHT on item 5 (col 2, row 1) -> wraps to item 6 (col 0, row 2)
             * Frame 65: Verify focused=6 ("Calendar")
             * Frame 68: Test wrapping: Inject DOWN on item 6 (col 0, row 2) -> wraps to item 0 (col 0, row 0)
             * Frame 73: Verify focused=0 ("Phone")
             * Frame 78: Complete and exit successfully
             */
            if (frame_count == 5) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                int idx = test_get_focused_index();
                printf("[TEST] Step 1: Initial Grid. Depth=%u (exp 1), ViewType=%d (exp %d), Focused=%d (exp 0)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_GRID, idx);
                if (depth != 1 || vt != VEEBHA_VIEW_TYPE_GRID || idx != 0) {
                    fprintf(stderr, "[TEST ERROR] Step 1 failed!\n");
                    return 10;
                }
            } else if (frame_count == 10) {
                printf("[TEST] Step 2: Injecting RIGHT...\n");
                test_inject_key(VEEBHA_KEY_RIGHT);
            } else if (frame_count == 15) {
                int idx = test_get_focused_index();
                printf("[TEST] Step 2 check: Focused=%d (expected 1 'Messages')\n", idx);
                if (idx != 1) {
                    fprintf(stderr, "[TEST ERROR] Expected focused index 1, got %d\n", idx);
                    return 11;
                }
            } else if (frame_count == 17) {
                printf("[TEST] Step 3: Injecting DOWN...\n");
                test_inject_key(VEEBHA_KEY_DOWN);
            } else if (frame_count == 22) {
                int idx = test_get_focused_index();
                printf("[TEST] Step 3 check: Focused=%d (expected 4 'Music')\n", idx);
                if (idx != 4) {
                    fprintf(stderr, "[TEST ERROR] Expected focused index 4, got %d\n", idx);
                    return 12;
                }
            } else if (frame_count == 24) {
                printf("[TEST] Step 4: Injecting RIGHT...\n");
                test_inject_key(VEEBHA_KEY_RIGHT);
            } else if (frame_count == 29) {
                int idx = test_get_focused_index();
                printf("[TEST] Step 4 check: Focused=%d (expected 5 'Settings')\n", idx);
                if (idx != 5) {
                    fprintf(stderr, "[TEST ERROR] Expected focused index 5, got %d\n", idx);
                    return 13;
                }
            } else if (frame_count == 32) {
                printf("[TEST] Step 5: Triggering LSK to enter 'Settings' submenu...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 38) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                int idx = test_get_focused_index();
                printf("[TEST] Step 5 check: Submenu Depth=%u (exp 2), ViewType=%d (exp %d), Focused=%d (exp 0)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_LIST, idx);
                if (depth != 2 || vt != VEEBHA_VIEW_TYPE_LIST || idx != 0) {
                    fprintf(stderr, "[TEST ERROR] Step 5 failed!\n");
                    return 14;
                }
            } else if (frame_count == 42) {
                printf("[TEST] Step 6: Injecting DOWN in list...\n");
                test_inject_key(VEEBHA_KEY_DOWN);
            } else if (frame_count == 47) {
                int idx = test_get_focused_index();
                printf("[TEST] Step 6 check: List Focused=%d (expected 1 'Sound')\n", idx);
                if (idx != 1) {
                    fprintf(stderr, "[TEST ERROR] Expected list focused index 1, got %d\n", idx);
                    return 15;
                }
            } else if (frame_count == 50) {
                printf("[TEST] Step 7: Triggering RSK to pop submenu...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 56) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                int idx = test_get_focused_index();
                printf("[TEST] Step 7 check: Root Menu Depth=%u (exp 1), ViewType=%d (exp %d), Restored Focused=%d (exp 5)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_GRID, idx);
                if (depth != 1 || vt != VEEBHA_VIEW_TYPE_GRID || idx != 5) {
                    fprintf(stderr, "[TEST ERROR] Step 7 failed (focus restoration)!\n");
                    return 16;
                }
            } else if (frame_count == 60) {
                printf("[TEST] Step 8: Testing row wrap: Injecting RIGHT on item 5...\n");
                test_inject_key(VEEBHA_KEY_RIGHT);
            } else if (frame_count == 65) {
                int idx = test_get_focused_index();
                printf("[TEST] Step 8 check: Focused=%d (expected 6 'Calendar')\n", idx);
                if (idx != 6) {
                    fprintf(stderr, "[TEST ERROR] Expected focused index 6 after row wrap, got %d\n", idx);
                    return 17;
                }
            } else if (frame_count == 68) {
                printf("[TEST] Step 9: Testing col wrap: Injecting DOWN on item 6...\n");
                test_inject_key(VEEBHA_KEY_DOWN);
            } else if (frame_count == 73) {
                int idx = test_get_focused_index();
                printf("[TEST] Step 9 check: Focused=%d (expected 0 'Phone')\n", idx);
                if (idx != 0) {
                    fprintf(stderr, "[TEST ERROR] Expected focused index 0 after col wrap, got %d\n", idx);
                    return 18;
                }
            } else if (frame_count >= 78) {
                printf("[TEST] All Phase 2 (Grid View, Navigation & Softkeys) checks PASSED successfully (%u frames)!\n",
                       frame_count);
                break;
            }
        }
    }

    /* 7. Teardown */
    win_mgr_reset_to_home();
    hal_input_deinit();
    hal_display_deinit();
    lv_deinit();
    SDL_Quit();

    printf("[SIM] Clean shutdown complete.\n");
    return 0;
}
