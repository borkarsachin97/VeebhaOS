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
#include "veebha_t9.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

static lv_group_t *g_keypad_group = NULL;

/* Level 2 forward declarations */
static void open_settings_menu(void);
static void open_messages_editor(void);

/* Buffer for text editor */
static char s_message_buffer[128] = "";

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
    if (index == 1) {
        /* "Messages" selected -> Push Level 2 Text Editor */
        open_messages_editor();
    } else if (index == 5) {
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

/* Level 2: Text Editor & Modal Dialog Handlers */
static void on_save_confirm(void)
{
    printf("[DEMO] Save confirmed for message: '%s'\n", s_message_buffer);
    win_mgr_pop(); /* Pop editor screen, returning to launcher */
}

static void on_editor_save(const char *text)
{
    printf("[DEMO] Editor Save triggered with content: '%s'\n", text);
    static char s_dialog_msg[128];
    snprintf(s_dialog_msg, sizeof(s_dialog_msg), "Save \"%s\" to drafts?", text);

    tpl_dialog_desc_t dlg = {
        .title = "Save Message?",
        .message = s_dialog_msg,
        .icon = LV_SYMBOL_SAVE,
        .on_confirm = on_save_confirm,
        .on_cancel = NULL, /* Cancel dismisses dialog, stays in editor */
        .lsk_label = "OK",
        .rsk_label = "Cancel"
    };
    tpl_dialog_show(&dlg);
}

static void open_messages_editor(void)
{
    s_message_buffer[0] = '\0';
    tpl_editor_desc_t desc = {
        .title = "New Message",
        .buffer = s_message_buffer,
        .max_len = sizeof(s_message_buffer),
        .on_save = on_editor_save,
        .on_cancel = NULL, /* Defaults to win_mgr_pop() */
        .lsk_label = "Done",
        .rsk_label = "Clear"
    };

    lv_obj_t *editor_scr = tpl_editor_create(&desc);
    if (editor_scr) {
        win_mgr_push(editor_scr, "Done", tpl_editor_default_lsk, "Back", tpl_editor_default_rsk);
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
            printf("[SIM] Running in automated Phase 2.2 verification mode\n");
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
    printf("  Keypad Controls:\n");
    printf("    D-Pad:             [Arrow Keys] (Up, Down, Left, Right)\n");
    printf("    Center OK:         [Enter] / [Space] / [Keypad Enter]\n");
    printf("    Left Softkey (LSK): [F1] / [Left Alt] / '['\n");
    printf("    Right Softkey(RSK): [F2] / [Right Alt] / ']' / [Backspace] / [Delete]\n");
    printf("    Call Key (Green):  [C]\n");
    printf("    End/Power Key(Red):[E] / [End] / [Escape]\n");
    printf("    Number Keys:       [0-9], [*], [#]\n");
    printf("    T9 Mode Switch:    '#' Key (cycles Abc -> ABC -> 123 -> abc)\n");
    printf("========================================================\n");

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
             * Frame  5: Initial Grid check (depth=1, focused=0 "Phone")
             * Frame 10: Inject RIGHT -> focused=1 ("Messages")
             * Frame 14: Trigger LSK -> open Messages Editor (depth=2, view_type=EDITOR)
             * Frame 20: Verify in Editor, mode="Abc"
             * Frame 22: Inject '#' -> cycle mode to "ABC"
             * Frame 25: Verify mode="ABC"
             * Frame 27: Inject '#' -> cycle mode to "123"
             * Frame 30: Verify mode="123"
             * Frame 32: Inject '#' -> cycle mode to "abc"
             * Frame 35: Verify mode="abc"
             * Frame 37: Inject '#' -> cycle mode to "Abc"
             * Frame 40: Verify mode="Abc"
             * Frame 42: Test multi-tap:
             *           Inject '8', '8', '8' -> 'V' (sentence capital)
             * Frame 43: '8' tap 2
             * Frame 44: '8' tap 3
             * Frame 46: Commit 'V', inject '3', '3' -> 'e'
             * Frame 47: '3' tap 2
             * Frame 49: Commit 'e', inject '3', '3' -> 'e'
             * Frame 50: '3' tap 2
             * Frame 52: Commit 'e' -> text is "Vee"
             * Frame 54: Test Backspace (RSK) -> deletes 'e', text becomes "Ve"
             * Frame 57: Trigger LSK ("Done") -> opens Modal Dialog ("Save Message?")
             * Frame 62: Verify Dialog active (tpl_dialog_is_active() == true)
             * Frame 65: Trigger LSK ("OK") on Dialog -> confirms and pops Editor back to Launcher!
             * Frame 70: Verify back in Launcher (depth=1, view_type=GRID, focused=1 "Messages")
             * Frame 73: Test Settings submenu navigation:
             *           Inject RIGHT -> 2 ("Contacts"), DOWN -> 5 ("Settings")
             * Frame 75: Select "Settings" (LSK) -> depth=2 (LIST)
             * Frame 79: Pop Settings (RSK) -> depth=1, restored focused=5 ("Settings")
             * Frame 84: All verification checks passed!
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
                printf("[TEST] Step 2: Injecting RIGHT towards 'Messages'...\n");
                test_inject_key(VEEBHA_KEY_RIGHT);
            } else if (frame_count == 12) {
                int idx = test_get_focused_index();
                printf("[TEST] Step 2 check: Focused=%d (expected 1 'Messages')\n", idx);
                if (idx != 1) {
                    fprintf(stderr, "[TEST ERROR] Expected focused index 1, got %d\n", idx);
                    return 11;
                }
                printf("[TEST] Step 3: Triggering LSK to launch Text Editor...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 20) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                t9_input_mode_t mode = t9_engine_get_mode();
                printf("[TEST] Step 3 check: Editor Depth=%u (exp 2), ViewType=%d (exp %d), T9 Mode=%s (exp 'Abc')\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_EDITOR, t9_engine_get_mode_str(mode));
                if (depth != 2 || vt != VEEBHA_VIEW_TYPE_EDITOR || mode != T9_MODE_SENTENCE) {
                    fprintf(stderr, "[TEST ERROR] Step 3 Editor launch failed!\n");
                    return 12;
                }
            } else if (frame_count == 22) {
                printf("[TEST] Step 4: Testing T9 mode cycling with '#'...\n");
                test_inject_key(VEEBHA_KEY_HASH);
            } else if (frame_count == 25) {
                t9_input_mode_t mode = t9_engine_get_mode();
                printf("[TEST] Step 4a check: Mode is '%s' (exp 'ABC')\n", t9_engine_get_mode_str(mode));
                if (mode != T9_MODE_UPPER) {
                    fprintf(stderr, "[TEST ERROR] Expected T9_MODE_UPPER\n");
                    return 13;
                }
                test_inject_key(VEEBHA_KEY_HASH);
            } else if (frame_count == 28) {
                t9_input_mode_t mode = t9_engine_get_mode();
                printf("[TEST] Step 4b check: Mode is '%s' (exp '123')\n", t9_engine_get_mode_str(mode));
                if (mode != T9_MODE_NUMBER) {
                    fprintf(stderr, "[TEST ERROR] Expected T9_MODE_NUMBER\n");
                    return 14;
                }
                test_inject_key(VEEBHA_KEY_HASH);
            } else if (frame_count == 31) {
                t9_input_mode_t mode = t9_engine_get_mode();
                printf("[TEST] Step 4c check: Mode is '%s' (exp 'abc')\n", t9_engine_get_mode_str(mode));
                if (mode != T9_MODE_LOWER) {
                    fprintf(stderr, "[TEST ERROR] Expected T9_MODE_LOWER\n");
                    return 15;
                }
                test_inject_key(VEEBHA_KEY_HASH);
            } else if (frame_count == 34) {
                t9_input_mode_t mode = t9_engine_get_mode();
                printf("[TEST] Step 4d check: Mode is '%s' (exp 'Abc')\n", t9_engine_get_mode_str(mode));
                if (mode != T9_MODE_SENTENCE) {
                    fprintf(stderr, "[TEST ERROR] Expected T9_MODE_SENTENCE\n");
                    return 16;
                }
                printf("[TEST] Step 5: Testing multi-tap entry (Typing 'Vee')...\n");
                test_inject_key(VEEBHA_KEY_NUM_8); /* '8' tap 1 -> 'T' */
            } else if (frame_count == 36) {
                test_inject_key(VEEBHA_KEY_NUM_8); /* '8' tap 2 -> 'U' */
            } else if (frame_count == 38) {
                test_inject_key(VEEBHA_KEY_NUM_8); /* '8' tap 3 -> 'V' */
            } else if (frame_count == 41) {
                /* Commit 'V' and type 'e' via key '3' */
                t9_engine_commit();
                test_inject_key(VEEBHA_KEY_NUM_3); /* '3' tap 1 -> 'd' */
            } else if (frame_count == 43) {
                test_inject_key(VEEBHA_KEY_NUM_3); /* '3' tap 2 -> 'e' */
            } else if (frame_count == 46) {
                /* Commit 'e' and type another 'e' */
                t9_engine_commit();
                test_inject_key(VEEBHA_KEY_NUM_3); /* '3' tap 1 -> 'd' */
            } else if (frame_count == 48) {
                test_inject_key(VEEBHA_KEY_NUM_3); /* '3' tap 2 -> 'e' */
            } else if (frame_count == 51) {
                t9_engine_commit();
                printf("[TEST] Step 6: Testing Backspace (RSK short press via HAL event)...\n");
                test_inject_key(VEEBHA_KEY_RSK); /* Deletes last 'e' via HAL indev, buffer becomes 'Ve' */
            } else if (frame_count == 55) {
                printf("[TEST] Step 7: Triggering LSK ('Done') to open Modal Dialog...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 60) {
                bool active = tpl_dialog_is_active();
                printf("[TEST] Step 7 check: Modal Dialog Active: %d (exp 1)\n", (int)active);
                if (!active) {
                    fprintf(stderr, "[TEST ERROR] Modal Dialog is not active!\n");
                    return 17;
                }
                printf("[TEST] Step 8: Confirming Modal Dialog via LSK ('OK')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 66) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                int idx = test_get_focused_index();
                printf("[TEST] Step 8 check: Returned to Launcher. Depth=%u (exp 1), ViewType=%d (exp %d), Focused=%d (exp 1)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_GRID, idx);
                if (depth != 1 || vt != VEEBHA_VIEW_TYPE_GRID || idx != 1) {
                    fprintf(stderr, "[TEST ERROR] Step 8 return to launcher failed!\n");
                    return 18;
                }
                printf("[TEST] Verified saved message buffer: '%s'\n", s_message_buffer);
                if (strcmp(s_message_buffer, "Ve") != 0) {
                    fprintf(stderr, "[TEST ERROR] Expected message buffer 'Ve', got '%s'\n", s_message_buffer);
                    return 19;
                }
                printf("[TEST] Step 9: Navigating from 'Messages' to 'Settings'...\n");
                test_inject_key(VEEBHA_KEY_RIGHT); /* to 2 ("Contacts") */
            } else if (frame_count == 69) {
                test_inject_key(VEEBHA_KEY_DOWN);  /* to 5 ("Settings") */
            } else if (frame_count == 72) {
                int idx = test_get_focused_index();
                printf("[TEST] Step 9 check: Focused=%d (expected 5 'Settings')\n", idx);
                if (idx != 5) {
                    fprintf(stderr, "[TEST ERROR] Expected focused index 5, got %d\n", idx);
                    return 20;
                }
                printf("[TEST] Entering Settings submenu...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 76) {
                uint8_t depth = win_mgr_get_depth();
                printf("[TEST] In Settings. Depth=%u (exp 2). Testing END key (Power/Hangup) to reset to home...\n", depth);
                if (depth != 2) {
                    fprintf(stderr, "[TEST ERROR] Expected depth 2 for Settings\n");
                    return 21;
                }
                test_inject_key(VEEBHA_KEY_END);
            } else if (frame_count == 80) {
                uint8_t depth = win_mgr_get_depth();
                int idx = test_get_focused_index();
                printf("[TEST] Back at Launcher via END key. Depth=%u (exp 1), Restored Focused=%d (exp 5)\n", depth, idx);
                if (depth != 1 || idx != 5) {
                    fprintf(stderr, "[TEST ERROR] Expected depth 1, focused 5\n");
                    return 22;
                }
            } else if (frame_count >= 85) {
                printf("[TEST] All Phase 2.2 (Modal Dialog, T9 Engine & Editor) checks PASSED successfully (%u frames)!\n",
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
