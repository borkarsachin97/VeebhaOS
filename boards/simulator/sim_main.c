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

/* Level 1: Main Menu Handlers */
static void on_main_menu_select(uint16_t index)
{
    printf("[DEMO] Main Menu Item %u Selected\n", index);
    if (index == 0) {
        /* "Settings" selected -> Push Level 2 */
        open_settings_menu();
    } else if (index == 1) {
        printf("[DEMO] Messages opened (stub)\n");
    } else if (index == 2) {
        printf("[DEMO] Music Player opened (stub)\n");
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
        { .icon = LV_SYMBOL_EYE_OPEN, .title = "Display", .subtext = "Theme, Brightness" },
        { .icon = LV_SYMBOL_VOLUME_MAX, .title = "Sound", .subtext = "Ringtone, Alert" },
        { .icon = LV_SYMBOL_SETTINGS, .title = "About", .subtext = "RDA8809 / Sim" },
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
    static const tpl_list_item_t s_main_items[] = {
        { .icon = LV_SYMBOL_SETTINGS, .title = "Settings", .subtext = "Config & display" },
        { .icon = LV_SYMBOL_ENVELOPE, .title = "Messages", .subtext = "Inbox (0)" },
        { .icon = LV_SYMBOL_AUDIO,    .title = "Music Player", .subtext = "Stopped" },
    };

    tpl_list_view_t desc = {
        .title = "Main Menu",
        .items = s_main_items,
        .count = sizeof(s_main_items) / sizeof(s_main_items[0]),
        .on_select = on_main_menu_select,
        .on_back = NULL,
        .lsk_label = "Options",
        .rsk_label = "Exit"
    };

    lv_obj_t *main_scr = tpl_list_create(&desc);
    if (main_scr) {
        win_mgr_push(main_scr, "Options", tpl_list_default_lsk, "Exit", tpl_list_default_rsk);
    }
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
            printf("[SIM] Running in automated Phase 2.1 verification mode\n");
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

    /* 5. Initialize Root Demo Screen */
    setup_demo_app();

    printf("[SIM] Simulator ready.\n");
    printf("      Arrow Keys: Navigate rows\n");
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

            /* Automated interaction sequence:
             * Frame 15: Select "Settings" (pushes Level 2)
             * Frame 30: Verify stack depth == 2, then trigger RSK (pops Level 2)
             * Frame 45: Verify stack depth == 1 and Level 1 restored
             * Frame 60: Terminate cleanly
             */
            if (frame_count == 15) {
                printf("[TEST] Step 1: Triggering LSK to enter 'Settings' submenu...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 30) {
                uint8_t depth = win_mgr_get_depth();
                printf("[TEST] Step 2: In submenu. Depth: %u (expected 2)\n", depth);
                if (depth != 2) {
                    fprintf(stderr, "[TEST ERROR] Expected stack depth 2, got %u\n", depth);
                    return 2;
                }
                printf("[TEST] Triggering RSK to pop submenu...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 45) {
                uint8_t depth = win_mgr_get_depth();
                printf("[TEST] Step 3: Back in root menu. Depth: %u (expected 1)\n", depth);
                if (depth != 1) {
                    fprintf(stderr, "[TEST ERROR] Expected stack depth 1 after pop, got %u\n", depth);
                    return 3;
                }
            } else if (frame_count >= 60) {
                printf("[TEST] Phase 2.1 verification passed successfully (%u frames processed)!\n", frame_count);
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
