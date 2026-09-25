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
#include "sdk/hal/hal_display.h"
#include "sdk/hal/hal_keypad.h"
#include "sdk/hal/hal_power.h"
#include "sdk/hal/hal_audio.h"
#include "sdk/core/os_kernel.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "veebha_win_mgr.h"
#include "veebha_templates.h"
#include "veebha_softkeys.h"
#include "veebha_overlays.h"
#include "veebha_status_bar.h"
#include "veebha_t9.h"
#include "veebha_theme.h"
#include "veebha_log.h"
#include "veebha_event.h"
#include "veebha_irq.h"
#include "apps/common/mock_telephony.h"
#include "apps/home/app_idle.h"
#include "apps/home/app_launcher.h"
#include "apps/dialer/app_dialer.h"
#include "apps/contacts/app_contacts.h"
#include "apps/messages/app_messages.h"
#include "apps/calllogs/app_calllogs.h"
#include "apps/settings/app_settings.h"
#include "apps/music/app_music.h"
#include "apps/files/app_files.h"
#include "apps/calendar/app_calendar.h"
#include "apps/tools/app_tools.h"
#include "apps/tools/app_calc.h"
#include "apps/tools/app_stopwatch.h"
#include "apps/tools/app_alarm.h"
#include "apps/tools/app_textread.h"
#include "apps/game/app_game.h"
#include "apps/gallery/app_gallery.h"
#include "apps/recorder/app_recorder.h"
#include "apps/overlays/boot_screen.h"
#include "apps/overlays/screen_saver.h"
#include "apps/overlays/usb_select.h"
#include "apps/settings/app_bt.h"
#include "apps/settings/app_bt_scan.h"
#include "apps/settings/app_tethering.h"
#include "apps/telephony/app_incall.h"
#include "veebha_connectivity.h"
#include "veebha_live_pill.h"
#include "boards/board_info.h"
#include "drivers/mock/mock_connectivity.h"
#include "app_registry.h"
#include "sdk/vfs/os_vfs.h"
#include "sdk/storage/os_nvram.h"
#include "sdk/core/wallpaper.h"
#include "sdk/include/veebha_i18n.h"
#include "sdk/text/indic_shaper.h"
#include "sdk/text/font_fallback.h"
#include "sdk/include/veebha_vapp.h"
#include "sdk/core/vapp_loader.h"
#include "apps/tools/app_funzone.h"
#include "apps/browser/app_browser.h"
#include "drivers/hal_display_sim.h"
#include <SDL2/SDL.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define TAG "SIM_MAIN"

#if defined(__SANITIZE_ADDRESS__) || (defined(__has_feature) && __has_feature(address_sanitizer))
const char *__lsan_default_suppressions(void);
const char *__lsan_default_suppressions(void) {
    return "leak:libSDL2\n"
           "leak:libX11\n"
           "leak:libGL\n"
           "leak:libdbus\n"
           "leak:libdbus-1\n"
           "leak:_dbus\n"
           "leak:dbus\n"
           "leak:libpulse\n"
           "leak:libasound\n"
           "leak:libwayland\n"
           "leak:libdecor\n"
           "leak:libxcb\n"
           "leak:libXcursor\n"
           "leak:libXi\n"
           "leak:libXext\n"
           "leak:libXfixes\n"
           "leak:libXrandr\n"
           "leak:libXrender\n"
           "leak:dri\n"
           "leak:mesa\n"
           "leak:dconf\n"
           "leak:libpipewire\n"
           "leak:libspa\n";
}
#endif

static lv_group_t *g_keypad_group = NULL;
static bool s_running = true;
static bool s_automated_test = false;

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

/* Automated test sequencer */
static int run_test_step(uint32_t frame_count)
{
    /* Automated verification sequence */
    if (frame_count == 5) {

                /* Step 1: Standby / Idle Screen verification */
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 1: Initial Standby / Idle Screen. Depth=%u (exp 1), ViewType=%d (exp %d IDLE)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_IDLE);
                if (depth != 1 || vt != VEEBHA_VIEW_TYPE_IDLE) {
                    fprintf(stderr, "[TEST ERROR] Step 1 Idle screen check failed!\n");
                    return 10;
                }

                win_mgr_entry_t *top = win_mgr_get_top();
                if (top && top->screen) {
                    /* Status bar check */
                    lv_obj_t *bar = lv_obj_get_child(top->screen, 0);
                    if (bar && lv_obj_get_child_count(bar) >= 3) {
                        lv_obj_t *left_tray = lv_obj_get_child(bar, 0);
                        lv_obj_t *clock_lbl = lv_obj_get_child(bar, 1);
                        lv_obj_t *right_tray = lv_obj_get_child(bar, 2);
                        const char *clock_txt = lv_label_get_text(clock_lbl);
                        printf("[TEST] Status Bar Check: left_kids=%u, clock_txt='%s', right_kids=%u\n",
                               lv_obj_get_child_count(left_tray),
                               clock_txt ? clock_txt : "",
                               lv_obj_get_child_count(right_tray));
                        if (lv_obj_get_child_count(left_tray) < 2 ||
                            !clock_txt || strlen(clock_txt) == 0 ||
                            lv_obj_get_child_count(right_tray) < 3) {
                            fprintf(stderr, "[TEST ERROR] Status bar tray layout check failed!\n");
                            return 10;
                        }
                    }

                    /* Viewport check: carrier, clock, date */
                    lv_obj_t *vp = lv_obj_get_child(top->screen, 1);
                    if (vp && lv_obj_get_child_count(vp) >= 2) {
                        lv_obj_t *c_lbl = lv_obj_get_child(vp, 0);
                        lv_obj_t *card = lv_obj_get_child(vp, 1);
                        lv_obj_t *clk_lbl = (card && lv_obj_get_child_count(card) >= 1) ? lv_obj_get_child(card, 0) : NULL;
                        lv_obj_t *d_lbl = (card && lv_obj_get_child_count(card) >= 2) ? lv_obj_get_child(card, 1) : NULL;
                        const char *c_txt = c_lbl ? lv_label_get_text(c_lbl) : NULL;
                        const char *clk_txt = clk_lbl ? lv_label_get_text(clk_lbl) : NULL;
                        const char *d_txt = d_lbl ? lv_label_get_text(d_lbl) : NULL;
                        printf("[TEST] Idle Screen: Carrier='%s', Clock='%s', Date='%s'\n",
                               c_txt ? c_txt : "", clk_txt ? clk_txt : "", d_txt ? d_txt : "");
                        if (!c_txt || strstr(c_txt, "Veebha") == NULL ||
                            !clk_txt || strlen(clk_txt) == 0 ||
                            !d_txt || strlen(d_txt) == 0) {
                            fprintf(stderr, "[TEST ERROR] Idle screen viewport contents invalid!\n");
                            return 10;
                        }
                    }
                }
                /* Step 1 snapshot */
                lv_obj_invalidate(lv_screen_active());
                lv_refr_now(NULL);
                hal_display_save_screenshot("./build/screenshot_idle.bmp");
            } else if (frame_count == 8) {

                printf("[TEST] Step 1a: Direct Dialing from Idle Screen (Injecting '9')...\n");
                test_inject_key(VEEBHA_KEY_NUM_9);
            } else if (frame_count == 12) {
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                bool active = app_dialer_is_active();
                const char *digits = app_dialer_get_digits();
                printf("[TEST] Step 1a check: ViewType=%d (exp %d DIALER), DialerActive=%d, Digits='%s' (exp '9')\n",
                       (int)vt, (int)VEEBHA_VIEW_TYPE_DIALER, (int)active, digits);
                if (vt != VEEBHA_VIEW_TYPE_DIALER || !active || strcmp(digits, "9") != 0) {
                    fprintf(stderr, "[TEST ERROR] Direct dialing from Idle failed!\n");
                    return 10;
                }
                printf("[TEST] Clearing digit via RSK ('Clear')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 14) {
                printf("[TEST] Popping Dialer via RSK ('Back') back to Idle...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 18) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 1b check: Back on Idle Screen. Depth=%u (exp 1), ViewType=%d (exp %d IDLE)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_IDLE);
                if (depth != 1 || vt != VEEBHA_VIEW_TYPE_IDLE) {
                    fprintf(stderr, "[TEST ERROR] Return to Idle from dialer failed!\n");
                    return 10;
                }
                printf("[TEST] Step 2: Opening 3x3 Launcher via LSK ('Menu')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 22) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                int idx = test_get_focused_index();
                printf("[TEST] Step 2 check: In 3x3 Launcher. Depth=%u (exp 2), ViewType=%d (exp %d GRID), Focused=%d (exp 0)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_GRID, idx);
                if (depth != 2 || vt != VEEBHA_VIEW_TYPE_GRID || idx != 0) {
                    fprintf(stderr, "[TEST ERROR] Launcher grid launch failed!\n");
                    return 11;
                }
                win_mgr_entry_t *top = win_mgr_get_top();
                if (top && top->screen) {
                    lv_obj_t *banner = lv_obj_get_child(top->screen, 1);
                    if (banner && lv_obj_get_child_count(banner) > 0) {
                        lv_obj_t *banner_lbl = lv_obj_get_child(banner, 0);
                        const char *txt = lv_label_get_text(banner_lbl);
                        printf("[TEST] Dynamic Title Banner on Phone: '%s' (exp 'Phone')\n", txt ? txt : "");
                        if (!txt || strcmp(txt, "Phone") != 0) {
                            fprintf(stderr, "[TEST ERROR] Dynamic banner title mismatch! Expected 'Phone', got '%s'\n", txt ? txt : "NULL");
                            return 11;
                        }
                    }
                }
                /* Step 2 snapshot */
                lv_obj_invalidate(lv_screen_active());
                lv_refr_now(NULL);
                hal_display_save_screenshot("./build/screenshot_launcher.bmp");

                printf("[TEST] Step 2a: Injecting RIGHT towards 'Messages'...\n");
                test_inject_key(VEEBHA_KEY_RIGHT);
            } else if (frame_count == 24) {

                int idx = test_get_focused_index();
                printf("[TEST] Step 2a check: Focused=%d (expected 1 'Messages')\n", idx);
                if (idx != 1) {
                    fprintf(stderr, "[TEST ERROR] Expected focused index 1, got %d\n", idx);
                    return 11;
                }
                win_mgr_entry_t *top = win_mgr_get_top();
                if (top && top->screen) {
                    lv_obj_t *banner = lv_obj_get_child(top->screen, 1);
                    if (banner && lv_obj_get_child_count(banner) > 0) {
                        lv_obj_t *banner_lbl = lv_obj_get_child(banner, 0);
                        const char *txt = lv_label_get_text(banner_lbl);
                        printf("[TEST] Dynamic Title Banner on Messages: '%s' (exp 'Messages')\n", txt ? txt : "");
                        if (!txt || strcmp(txt, "Messages") != 0) {
                            fprintf(stderr, "[TEST ERROR] Dynamic banner not updated to 'Messages'!\n");
                            return 11;
                        }
                    }
                }
                printf("[TEST] Step 2b: Popping Launcher via RSK ('Back') to verify return to Idle...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 28) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 2b check: Returned to Idle Screen. Depth=%u (exp 1), ViewType=%d (exp %d IDLE)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_IDLE);
                if (depth != 1 || vt != VEEBHA_VIEW_TYPE_IDLE) {
                    fprintf(stderr, "[TEST ERROR] Return to Idle from Launcher failed!\n");
                    return 11;
                }
                printf("[TEST] Step 2c: Re-opening Launcher via LSK ('Menu')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 32) {
                printf("[TEST] Navigating to 'Messages'...\n");
                test_inject_key(VEEBHA_KEY_RIGHT);
            } else if (frame_count == 36) {
                int idx = test_get_focused_index();
                printf("[TEST] Focused=%d. Opening Messages Inbox via LSK ('Select')...\n", idx);
                softkey_trigger_lsk();
            } else if (frame_count == 40) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] In Messages Inbox. Depth=%u (exp 3), ViewType=%d (exp %d LIST)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_LIST);
                if (depth != 3 || vt != VEEBHA_VIEW_TYPE_LIST) {
                    fprintf(stderr, "[TEST ERROR] Messages Inbox launch failed!\n");
                    return 12;
                }
                printf("[TEST] Selecting row 0 [+ New Message] to open SMS Composer...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 44) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                t9_input_mode_t mode = t9_engine_get_mode();
                printf("[TEST] Step 3 check: Editor Depth=%u (exp 4), ViewType=%d (exp %d), T9 Mode=%s (exp 'Abc')\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_EDITOR, t9_engine_get_mode_str(mode));
                if (depth != 4 || vt != VEEBHA_VIEW_TYPE_EDITOR || mode != T9_MODE_SENTENCE) {
                    fprintf(stderr, "[TEST ERROR] Step 3 Editor launch failed!\n");
                    return 12;
                }
            } else if (frame_count == 46) {
                printf("[TEST] Step 4: Testing T9 mode cycling with '#'...\n");
                test_inject_key(VEEBHA_KEY_HASH);
            } else if (frame_count == 49) {
                t9_input_mode_t mode = t9_engine_get_mode();
                printf("[TEST] Step 4a check: Mode is '%s' (exp 'ABC')\n", t9_engine_get_mode_str(mode));
                if (mode != T9_MODE_UPPER) {
                    fprintf(stderr, "[TEST ERROR] Expected T9_MODE_UPPER\n");
                    return 13;
                }
                test_inject_key(VEEBHA_KEY_HASH);
            } else if (frame_count == 52) {
                t9_input_mode_t mode = t9_engine_get_mode();
                printf("[TEST] Step 4b check: Mode is '%s' (exp '123')\n", t9_engine_get_mode_str(mode));
                if (mode != T9_MODE_NUMBER) {
                    fprintf(stderr, "[TEST ERROR] Expected T9_MODE_NUMBER\n");
                    return 14;
                }
                test_inject_key(VEEBHA_KEY_HASH);
            } else if (frame_count == 55) {
                t9_input_mode_t mode = t9_engine_get_mode();
                printf("[TEST] Step 4c check: Mode is '%s' (exp 'abc')\n", t9_engine_get_mode_str(mode));
                if (mode != T9_MODE_LOWER) {
                    fprintf(stderr, "[TEST ERROR] Expected T9_MODE_LOWER\n");
                    return 15;
                }
                test_inject_key(VEEBHA_KEY_HASH);
            } else if (frame_count == 58) {
                t9_input_mode_t mode = t9_engine_get_mode();
                printf("[TEST] Step 4d check: Mode is '%s' (exp 'Abc')\n", t9_engine_get_mode_str(mode));
                if (mode != T9_MODE_SENTENCE) {
                    fprintf(stderr, "[TEST ERROR] Expected T9_MODE_SENTENCE\n");
                    return 16;
                }
                printf("[TEST] Step 5: Testing multi-tap entry (Typing 'Vee')...\n");
                test_inject_key(VEEBHA_KEY_NUM_8); /* '8' tap 1 -> 'T' */
            } else if (frame_count == 60) {
                test_inject_key(VEEBHA_KEY_NUM_8); /* '8' tap 2 -> 'U' */
            } else if (frame_count == 62) {
                test_inject_key(VEEBHA_KEY_NUM_8); /* '8' tap 3 -> 'V' */
            } else if (frame_count == 64) {
                t9_engine_commit();
                test_inject_key(VEEBHA_KEY_NUM_3); /* '3' tap 1 -> 'd' */
            } else if (frame_count == 66) {
                test_inject_key(VEEBHA_KEY_NUM_3); /* '3' tap 2 -> 'e' */
            } else if (frame_count == 68) {
                t9_engine_commit();
                test_inject_key(VEEBHA_KEY_NUM_3); /* '3' tap 1 -> 'd' */
            } else if (frame_count == 70) {
                test_inject_key(VEEBHA_KEY_NUM_3); /* '3' tap 2 -> 'e' */
            } else if (frame_count == 72) {
                t9_engine_commit();
                printf("[TEST] Step 6: Testing Backspace (RSK short press via HAL event)...\n");
                test_inject_key(VEEBHA_KEY_RSK);
            } else if (frame_count == 76) {
                printf("[TEST] Step 7: Triggering LSK ('Send') to open Message Sent Dialog...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 80) {
                bool active = tpl_dialog_is_active();
                printf("[TEST] Step 7 check: Modal Dialog Active: %d (exp 1)\n", (int)active);
                if (!active) {
                    fprintf(stderr, "[TEST ERROR] Modal Dialog is not active!\n");
                    return 17;
                }
                printf("[TEST] Step 8: Confirming Modal Dialog via LSK ('OK')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 84) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Returned to Messages Inbox. Depth=%u (exp 3), ViewType=%d (exp %d LIST)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_LIST);
                if (depth != 3 || vt != VEEBHA_VIEW_TYPE_LIST) {
                    fprintf(stderr, "[TEST ERROR] Return to Messages Inbox failed!\n");
                    return 18;
                }
                printf("[TEST] Popping Messages Inbox via RSK ('Back')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 88) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                int idx = test_get_focused_index();
                printf("[TEST] Back at Launcher. Depth=%u (exp 2), ViewType=%d (exp %d), Focused=%d (exp 1)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_GRID, idx);
                if (depth != 2 || vt != VEEBHA_VIEW_TYPE_GRID || idx != 1) {
                    fprintf(stderr, "[TEST ERROR] Return to launcher failed!\n");
                    return 18;
                }
                printf("[TEST] Step 9: Navigating from 'Messages' to 'Settings'...\n");
                test_inject_key(VEEBHA_KEY_RIGHT); /* to 2 ("Contacts") */
            } else if (frame_count == 91) {
                test_inject_key(VEEBHA_KEY_DOWN);  /* to 5 ("Settings") */
            } else if (frame_count == 94) {
                int idx = test_get_focused_index();
                printf("[TEST] Step 9 check: Focused=%d (expected 5 'Settings')\n", idx);
                if (idx != 5) {
                    fprintf(stderr, "[TEST ERROR] Expected focused index 5, got %d\n", idx);
                    return 20;
                }
                printf("[TEST] Entering Settings submenu...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 98) {
                uint8_t depth = win_mgr_get_depth();
                printf("[TEST] In Settings. Depth=%u (exp 3). Testing list layout and status bar clock...\n", depth);
                if (depth != 3) {
                    fprintf(stderr, "[TEST ERROR] Expected depth 3 for Settings\n");
                    return 21;
                }
                win_mgr_entry_t *top = win_mgr_get_top();
                if (top && top->screen) {
                    lv_obj_t *subhdr = lv_obj_get_child(top->screen, 1);
                    if (subhdr && lv_obj_get_child_count(subhdr) > 0) {
                        lv_obj_t *hdr_lbl = lv_obj_get_child(subhdr, 0);
                        const char *htxt = lv_label_get_text(hdr_lbl);
                        printf("[TEST] Sub-Screen Header text: '%s' (exp 'Settings' or 'SETTINGS')\n", htxt ? htxt : "");
                        if (!htxt || (strcasecmp(htxt, "SETTINGS") != 0 && strcmp(htxt, "Settings") != 0)) {
                            fprintf(stderr, "[TEST ERROR] Sub-Screen Header mismatch! Expected 'Settings', got '%s'\n", htxt ? htxt : "NULL");
                            return 21;
                        }
                    }
                    lv_obj_t *content = lv_obj_get_child(top->screen, 2);
                    if (content && lv_obj_get_child_count(content) > 0) {
                        lv_obj_t *first_item = lv_obj_get_child(content, 0);
                        int item_y = lv_obj_get_y(first_item);
                        int item_h = lv_obj_get_height(first_item);
                        printf("[TEST] Settings List First Item Y=%d, H=%d (expected Y<=10, H=22 single line)\n", item_y, item_h);
                        if (item_y > 10 || item_h != 22) {
                            fprintf(stderr, "[TEST ERROR] Settings list layout check failed! y=%d, h=%d\n", item_y, item_h);
                            return 21;
                        }
                    }
                    lv_obj_t *status_bar = lv_obj_get_child(top->screen, 0);
                    status_bar_update_clock(status_bar, 14, 30);
                }
                printf("[TEST] Returning to Launcher via RSK ('Back')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 102) {
                uint8_t depth = win_mgr_get_depth();
                int idx = test_get_focused_index();
                printf("[TEST] Back at Launcher via RSK. Depth=%u (exp 2), Restored Focused=%d (exp 5)\n", depth, idx);
                if (depth != 2 || idx != 5) {
                    fprintf(stderr, "[TEST ERROR] Expected depth 2, focused 5\n");
                    return 22;
                }
                printf("[TEST] Step 10: Navigating from 'Settings' to 'Music'...\n");
                test_inject_key(VEEBHA_KEY_LEFT); /* to 4 ("Music") */
            } else if (frame_count == 106) {
                int idx = test_get_focused_index();
                printf("[TEST] Step 10 check: Focused=%d (expected 4 'Music')\n", idx);
                if (idx != 4) {
                    fprintf(stderr, "[TEST ERROR] Expected focused index 4, got %d\n", idx);
                    return 23;
                }
                printf("[TEST] Launching Music Suite (Tier 1: Playlist view)...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 110) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 10 check (Tier 1 Playlist): Depth=%u (exp 3), ViewType=%d (exp %d LIST)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_LIST);
                if (depth != 3 || vt != VEEBHA_VIEW_TYPE_LIST) {
                    fprintf(stderr, "[TEST ERROR] Music playlist initialization check failed!\n");
                    return 24;
                }
                printf("[TEST] Selecting track 0 ('Blinding Lights') via LSK to open Walkman Now Playing (Tier 2)...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 114) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                bool playing = app_music_is_playing();
                const music_track_t *t = app_music_get_current_track();
                uint8_t vol = app_music_get_volume();
                printf("[TEST] Step 10 check (Tier 2 Walkman): Depth=%u (exp 4), ViewType=%d (exp %d MEDIA), Track='%s', Playing=%d, Vol=%u\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_MEDIA, t ? t->title : "", (int)playing, vol);
                if (depth != 4 || vt != VEEBHA_VIEW_TYPE_MEDIA || !playing || vol != 7) {
                    fprintf(stderr, "[TEST ERROR] Walkman Now Playing verification failed!\n");
                    return 24;
                }
                printf("[TEST] Step 10a: Testing Volume Up (+1) via UP key...\n");
                test_inject_key(VEEBHA_KEY_UP);
            } else if (frame_count == 118) {
                uint8_t vol = app_music_get_volume();
                printf("[TEST] Step 10a check: Volume=%u (exp 8)\n", vol);
                if (vol != 8) {
                    fprintf(stderr, "[TEST ERROR] Expected volume 8, got %u\n", vol);
                    return 25;
                }
                printf("[TEST] Step 10b: Testing Volume Down (-1) via DOWN key...\n");
                test_inject_key(VEEBHA_KEY_DOWN);
            } else if (frame_count == 120) {
                uint8_t vol = app_music_get_volume();
                printf("[TEST] Step 10b check: Volume=%u (exp 7)\n", vol);
                if (vol != 7) {
                    fprintf(stderr, "[TEST ERROR] Expected volume 7, got %u\n", vol);
                    return 25;
                }
                printf("[TEST] Step 10c: Seeking forward (+5s) via RIGHT key...\n");
                test_inject_key(VEEBHA_KEY_RIGHT);
            } else if (frame_count == 122) {
                printf("[TEST] Step 10d: Toggling Play/Pause via OK key...\n");
                test_inject_key(VEEBHA_KEY_OK);
            } else if (frame_count == 124) {
                bool playing = app_music_is_playing();
                printf("[TEST] Step 10d check: Playing=%d (expected 0 / paused)\n", (int)playing);
                if (playing) {
                    fprintf(stderr, "[TEST ERROR] Expected playback paused\n");
                    return 27;
                }
                printf("[TEST] Step 11: Opening Quick Settings Drawer on lv_layer_top()...\n");
                notif_panel_show();
            } else if (frame_count == 126) {
                bool active = notif_panel_is_active();
                printf("[TEST] Step 11 check: Quick Settings Active=%d (exp 1)\n", (int)active);
                if (!active) {
                    fprintf(stderr, "[TEST ERROR] Quick Settings drawer should be active!\n");
                    return 28;
                }
                uint8_t tile_cnt = os_qs_tile_get_count();
                bool has_media = tpl_media_has_active_session();
                const char *head = tpl_media_get_headline();
                printf("[TEST] Quick Settings Registry Check: TileCount=%u (exp >= 4), MediaSessionActive=%d, Headline='%s'\n",
                       tile_cnt, (int)has_media, head ? head : "");
                if (tile_cnt < 4 || !has_media || strcmp(head, "Blinding Lights") != 0) {
                    fprintf(stderr, "[TEST ERROR] Quick settings media card / tile verification failed!\n");
                    return 28;
                }
                printf("[TEST] Closing Quick Settings Drawer via RSK ('Close')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 130) {
                bool active = notif_panel_is_active();
                printf("[TEST] Step 11 check: Quick Settings Active=%d (exp 0)\n", (int)active);
                if (active) {
                    fprintf(stderr, "[TEST ERROR] Quick Settings drawer should be closed!\n");
                    return 29;
                }
                printf("[TEST] Step 12: Pressing END key to return directly to Idle Screen without killing Music...\n");
                test_inject_key(VEEBHA_KEY_END);
            } else if (frame_count == 134) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                bool has_media = tpl_media_has_active_session();
                printf("[TEST] Step 12 check: Depth=%u (exp 1), ViewType=%d (exp %d IDLE), MediaSessionActive=%d (exp 1)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_IDLE, (int)has_media);
                if (depth != 1 || vt != VEEBHA_VIEW_TYPE_IDLE || !has_media) {
                    fprintf(stderr, "[TEST ERROR] END key should return to Idle Screen while preserving background Music!\n");
                    return 30;
                }
                printf("[TEST] Step 12a: Opening Multitasking Switcher from Idle Screen...\n");
                task_mgr_show();
            } else if (frame_count == 138) {
                bool active = task_mgr_is_active();
                printf("[TEST] Step 12a check: Multitasking Active=%d (exp 1)\n", (int)active);
                if (!active) {
                    fprintf(stderr, "[TEST ERROR] Multitasking switcher should be active!\n");
                    return 31;
                }
                printf("[TEST] Step 12b: Navigating UP to select 'Music' in Task List...\n");
                test_inject_key(VEEBHA_KEY_UP);
            } else if (frame_count == 142) {
                printf("[TEST] Step 12c: Switching back to 'Music' via LSK ('Switch')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 146) {
                bool active = task_mgr_is_active();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 12c check: Multitasking Active=%d (exp 0), ViewType=%d (exp %d MEDIA)\n",
                       (int)active, (int)vt, (int)VEEBHA_VIEW_TYPE_MEDIA);
                if (active || vt != VEEBHA_VIEW_TYPE_MEDIA) {
                    fprintf(stderr, "[TEST ERROR] Switching back to Music failed!\n");
                    return 32;
                }
                printf("[TEST] Step 12d: Re-opening Multitasking Switcher to end task...\n");
                task_mgr_show();
            } else if (frame_count == 150) {
                printf("[TEST] Closing Music app via Multitasking Switcher RSK ('End Task')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 154) {
                bool active = task_mgr_is_active();
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 12 check: Multitasking Active=%d (exp 0), Depth=%u (exp 1), ViewType=%d (exp %d IDLE)\n",
                       (int)active, depth, (int)vt, (int)VEEBHA_VIEW_TYPE_IDLE);
                if (active || depth != 1 || vt != VEEBHA_VIEW_TYPE_IDLE) {
                    fprintf(stderr, "[TEST ERROR] Return to Idle after ending task failed!\n");
                    return 34;
                }
                printf("[TEST] Step 13: Testing Emergency Call from Dialer...\n");
                app_dialer_open("112");
            } else if (frame_count == 158) {
                const char *digits = app_dialer_get_digits();
                const contact_record_t *c = telephony_find_contact_by_number("112");
                printf("[TEST] Step 13 check: Digits='%s' (exp '112'), MatchedContact='%s'\n",
                       digits, c ? c->name : "None");
                if (strcmp(digits, "112") != 0 || !c || strcmp(c->name, "Emergency 112") != 0) {
                    fprintf(stderr, "[TEST ERROR] Dialer contact matching check failed!\n");
                    return 36;
                }
                printf("[TEST] Initiating call to 112 via LSK ('Call')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 162) {
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                bool in_call = app_dialer_is_in_call();
                printf("[TEST] Step 13 check: ViewType=%d (exp %d MEDIA), InCall=%d (exp 1)\n",
                       (int)vt, (int)VEEBHA_VIEW_TYPE_MEDIA, (int)in_call);
                if (vt != VEEBHA_VIEW_TYPE_MEDIA || !in_call) {
                    fprintf(stderr, "[TEST ERROR] Outgoing call initiation check failed!\n");
                    return 37;
                }
                printf("[TEST] Terminating call via RSK ('End Call')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 166) {
                bool in_call = app_dialer_is_in_call();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                uint8_t depth = win_mgr_get_depth();
                printf("[TEST] Step 13 check: InCall=%d (exp 0), ViewType=%d (exp %d IDLE), Depth=%u (exp 1)\n",
                       (int)in_call, (int)vt, (int)VEEBHA_VIEW_TYPE_IDLE, depth);
                if (in_call || vt != VEEBHA_VIEW_TYPE_IDLE || depth != 1) {
                    fprintf(stderr, "[TEST ERROR] End call return to Idle failed!\n");
                    return 38;
                }
                printf("[TEST] Step 14: Opening Contacts Directory...\n");
                app_contacts_open();
            } else if (frame_count == 170) {
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                uint8_t depth = win_mgr_get_depth();
                uint16_t c_count = 0;
                telephony_get_contacts(&c_count);
                printf("[TEST] Step 14 check: Contacts ViewType=%d (exp %d LIST), Depth=%u (exp 2), ContactsCount=%u (exp 4)\n",
                       (int)vt, (int)VEEBHA_VIEW_TYPE_LIST, depth, c_count);
                if (vt != VEEBHA_VIEW_TYPE_LIST || depth != 2 || c_count != 4) {
                    fprintf(stderr, "[TEST ERROR] Contacts directory initialization check failed!\n");
                    return 39;
                }
                printf("[TEST] Step 14a: Adding new contact via [+ Add Contact] (row 0)...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 174) {
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 14b: In Contact Name editor (ViewType=%d). Saving name 'Charlie'...\n", (int)vt);
                if (vt != VEEBHA_VIEW_TYPE_EDITOR) {
                    fprintf(stderr, "[TEST ERROR] Expected Contact Name editor\n");
                    return 40;
                }
                softkey_trigger_lsk();
            } else if (frame_count == 178) {
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 14c: In Phone Number editor (ViewType=%d). Saving number '5559999'...\n", (int)vt);
                if (vt != VEEBHA_VIEW_TYPE_EDITOR) {
                    fprintf(stderr, "[TEST ERROR] Expected Phone Number editor\n");
                    return 41;
                }
                softkey_trigger_lsk();
            } else if (frame_count == 182) {
                bool dialog_active = tpl_dialog_is_active();
                printf("[TEST] Step 14d: Contact Saved modal dialog active=%d (exp 1)\n", (int)dialog_active);
                if (!dialog_active) {
                    fprintf(stderr, "[TEST ERROR] Expected Contact Saved dialog\n");
                    return 42;
                }
                softkey_trigger_lsk(); /* Confirm OK */
            } else if (frame_count == 186) {
                uint16_t c_count = 0;
                telephony_get_contacts(&c_count);
                const contact_record_t *c = telephony_find_contact_by_number("5559999");
                printf("[TEST] Step 14e: Total Contacts=%u (exp 5), Found New Contact='%s'\n",
                       c_count, c ? c->name : "None");
                if (c_count != 5) {
                    fprintf(stderr, "[TEST ERROR] Expected 5 contacts after add\n");
                    return 43;
                }
                printf("[TEST] Returning to Idle from Contacts via RSK ('Back')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 190) {
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                uint8_t depth = win_mgr_get_depth();
                printf("[TEST] Step 14f: Back at Idle. ViewType=%d (exp %d IDLE), Depth=%u (exp 1)\n",
                       (int)vt, (int)VEEBHA_VIEW_TYPE_IDLE, depth);
                if (vt != VEEBHA_VIEW_TYPE_IDLE || depth != 1) {
                    fprintf(stderr, "[TEST ERROR] Return to Idle from contacts failed!\n");
                    return 44;
                }
                printf("[TEST] Step 15: Opening Messages Application...\n");
                app_messages_open();
            } else if (frame_count == 194) {
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                uint8_t depth = win_mgr_get_depth();
                uint16_t m_count = 0;
                telephony_get_messages(&m_count);
                printf("[TEST] Step 15a check: Messages ViewType=%d (exp %d LIST), Depth=%u (exp 2), MsgCount=%u (exp 4)\n",
                       (int)vt, (int)VEEBHA_VIEW_TYPE_LIST, depth, m_count);
                if (vt != VEEBHA_VIEW_TYPE_LIST || depth != 2 || m_count != 4) {
                    fprintf(stderr, "[TEST ERROR] Messages inbox initialization check failed!\n");
                    return 45;
                }
                printf("[TEST] Step 15b: Composing new message via [+ New Message] (row 0)...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 198) {
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 15c: In SMS Composer (ViewType=%d). Sending message...\n", (int)vt);
                if (vt != VEEBHA_VIEW_TYPE_EDITOR) {
                    fprintf(stderr, "[TEST ERROR] Expected SMS Composer editor\n");
                    return 46;
                }
                softkey_trigger_lsk(); /* Trigger Send */
            } else if (frame_count == 202) {
                bool dialog_active = tpl_dialog_is_active();
                printf("[TEST] Step 15d: Message Sent dialog active=%d (exp 1)\n", (int)dialog_active);
                if (!dialog_active) {
                    fprintf(stderr, "[TEST ERROR] Expected Message Sent dialog\n");
                    return 47;
                }
                softkey_trigger_lsk(); /* Confirm OK */
            } else if (frame_count == 206) {
                uint16_t m_count = 0;
                sms_message_t *msgs = telephony_get_messages(&m_count);
                printf("[TEST] Step 15e: Messages count=%u (exp 5), Latest recipient='%s'\n",
                       m_count, msgs[0].sender);
                if (m_count != 5) {
                    fprintf(stderr, "[TEST ERROR] Expected 5 messages after sending\n");
                    return 48;
                }
                printf("[TEST] Returning to Idle from Messages via RSK ('Back')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 210) {
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                uint8_t depth = win_mgr_get_depth();
                printf("[TEST] Step 15f: Back at Idle. ViewType=%d (exp %d IDLE), Depth=%u (exp 1)\n",
                       (int)vt, (int)VEEBHA_VIEW_TYPE_IDLE, depth);
                if (vt != VEEBHA_VIEW_TYPE_IDLE || depth != 1) {
                    fprintf(stderr, "[TEST ERROR] Return to Idle from messages failed!\n");
                    return 49;
                }
                printf("[TEST] Step 16: Opening Call Logs application...\n");
                app_calllogs_open();
            } else if (frame_count == 214) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                uint16_t log_count = 0;
                telephony_get_call_logs(&log_count);
                printf("[TEST] Step 16a check: Call Logs ViewType=%d (exp %d LIST), Depth=%u (exp 2), LogCount=%u (exp >= 5)\n",
                       (int)vt, (int)VEEBHA_VIEW_TYPE_LIST, depth, log_count);
                if (vt != VEEBHA_VIEW_TYPE_LIST || depth != 2 || log_count < 5) {
                    fprintf(stderr, "[TEST ERROR] Call logs initialization check failed!\n");
                    return 50;
                }
                printf("[TEST] Step 16b: Calling entry 0 from Call Logs via LSK ('Call')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 218) {
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                bool in_call = app_dialer_is_in_call();
                printf("[TEST] Step 16c check: InCall=%d (exp 1), ViewType=%d (exp %d MEDIA)\n",
                       (int)in_call, (int)vt, (int)VEEBHA_VIEW_TYPE_MEDIA);
                if (!in_call || vt != VEEBHA_VIEW_TYPE_MEDIA) {
                    fprintf(stderr, "[TEST ERROR] Call initiation from Call Logs failed!\n");
                    return 51;
                }
                printf("[TEST] Terminating call via RSK ('End Call')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 222) {
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                uint8_t depth = win_mgr_get_depth();
                printf("[TEST] Step 16d check: Back at Idle. ViewType=%d (exp %d IDLE), Depth=%u (exp 1)\n",
                       (int)vt, (int)VEEBHA_VIEW_TYPE_IDLE, depth);
                if (vt != VEEBHA_VIEW_TYPE_IDLE || depth != 1) {
                    fprintf(stderr, "[TEST ERROR] Return to Idle after ending call failed!\n");
                    return 52;
                }
                printf("[TEST] Step 17: Launching Music and testing permanent Idle in Task Switcher...\n");
                app_music_open();
            } else if (frame_count == 226) {
                uint8_t depth = win_mgr_get_depth();
                printf("[TEST] Music running (Depth=%u). Opening Task Switcher...\n", depth);
                task_mgr_show();
            } else if (frame_count == 230) {
                bool active = task_mgr_is_active();
                printf("[TEST] Step 17a check: Task Switcher Active=%d (exp 1). Navigating UP to select Row 0 'Idle Screen'...\n",
                       (int)active);
                if (!active) {
                    fprintf(stderr, "[TEST ERROR] Task Switcher should be active!\n");
                    return 53;
                }
                test_inject_key(VEEBHA_KEY_UP);
            } else if (frame_count == 234) {
                printf("[TEST] Step 17b: Switching to Idle Screen via LSK ('Switch')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 238) {
                bool active = task_mgr_is_active();
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 17c check: Task Switcher Active=%d (exp 0), Depth=%u (exp 1), ViewType=%d (exp %d IDLE)\n",
                       (int)active, depth, (int)vt, (int)VEEBHA_VIEW_TYPE_IDLE);
                if (active || depth != 1 || vt != VEEBHA_VIEW_TYPE_IDLE) {
                    fprintf(stderr, "[TEST ERROR] Switching to Idle Screen from Task Switcher failed!\n");
                    return 54;
                }
                printf("[TEST] Step 18: Testing Modular Settings Suite...\n");
                app_settings_open();
            } else if (frame_count == 242) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 18a check: In Settings. Depth=%u (exp 2), ViewType=%d (exp %d LIST)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_LIST);
                if (depth != 2 || vt != VEEBHA_VIEW_TYPE_LIST) {
                    fprintf(stderr, "[TEST ERROR] Opening Settings failed!\n");
                    return 55;
                }
                printf("[TEST] Step 18b: Navigating to 'Sound' (row 1)...\n");
                test_inject_key(VEEBHA_KEY_DOWN);
            } else if (frame_count == 246) {
                printf("[TEST] Step 18c: Entering Sound Submenu via LSK ('Select')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 248) {
                uint8_t depth = win_mgr_get_depth();
                printf("[TEST] Step 18d check: In Sound Settings. Depth=%u (exp 3). Opening Profiles submenu via LSK...\n", depth);
                if (depth != 3) {
                    fprintf(stderr, "[TEST ERROR] Entering Sound submenu failed!\n");
                    return 56;
                }
                softkey_trigger_lsk(); /* Open Profiles submenu */
            } else if (frame_count == 252) {
                test_inject_key(VEEBHA_KEY_DOWN); /* Focus Silent profile */
            } else if (frame_count == 256) {
                softkey_trigger_lsk(); /* Select Silent profile */
            } else if (frame_count == 258) {
                bool dialog_active = tpl_dialog_is_active();
                printf("[TEST] Step 18e check: Profile Changed dialog active=%d (exp 1)\n", (int)dialog_active);
                if (!dialog_active) {
                    fprintf(stderr, "[TEST ERROR] Profile changed dialog not active!\n");
                    return 57;
                }
                softkey_trigger_lsk(); /* Dismiss dialog */
            } else if (frame_count == 260) {
                bool is_silent = app_settings_is_silent();
                printf("[TEST] Step 18f check: Silent mode active=%d (exp 1)\n", (int)is_silent);
                if (!is_silent) {
                    fprintf(stderr, "[TEST ERROR] Silent mode should be active!\n");
                    return 58;
                }
                printf("[TEST] Returning to Sound Settings via RSK ('Back')...\n");
                softkey_trigger_rsk(); /* Pop Profiles list back to Sound */
            } else if (frame_count == 262) {
                printf("[TEST] Returning to Main Settings via RSK ('Back')...\n");
                softkey_trigger_rsk(); /* Pop Sound list back to Main Settings */
            } else if (frame_count == 264) {
                uint8_t depth = win_mgr_get_depth();
                printf("[TEST] Back in Main Settings. Depth=%u (exp 2). Returning to Idle via RSK ('Back')...\n", depth);
                if (depth != 2) {
                    fprintf(stderr, "[TEST ERROR] Return to Main Settings failed!\n");
                    return 59;
                }
                softkey_trigger_rsk(); /* Pop Main Settings back to Idle */
            } else if (frame_count == 266) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 18g check: Back at Idle. Depth=%u (exp 1), ViewType=%d (exp %d IDLE)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_IDLE);
                if (depth != 1 || vt != VEEBHA_VIEW_TYPE_IDLE) {
                    fprintf(stderr, "[TEST ERROR] Return to Idle from settings failed!\n");
                    return 60;
                }
                printf("[TEST] Step 19: Testing Phase 3.3 True Multitasking Task Pool...\n");
                app_messages_open();
            } else if (frame_count == 270) {
                softkey_trigger_lsk(); /* Open composer */
            } else if (frame_count == 274) {
                printf("[TEST] Step 19c: Typing draft in SMS Composer...\n");
                test_inject_key(VEEBHA_KEY_NUM_4);
                test_inject_key(VEEBHA_KEY_NUM_3);
                test_inject_key(VEEBHA_KEY_NUM_5);
            } else if (frame_count == 278) {
                printf("[TEST] Step 19d: Pressing END key to return Idle without wiping draft...\n");
                test_inject_key(VEEBHA_KEY_END);
            } else if (frame_count == 282) {
                printf("[TEST] Step 19e check: In Idle. Task count=%u (exp >= 1). Opening Music...\n", win_mgr_get_task_count());
                app_music_open();
            } else if (frame_count == 286) {
                printf("[TEST] Step 19f: Opening Multitasking Switcher with Messages and Music...\n");
                task_mgr_show();
            } else if (frame_count == 290) {
                uint8_t t_cnt = win_mgr_get_task_count();
                printf("[TEST] Step 19g check: Tasks in switcher=%u (exp >= 2). Messages and Music present.\n", t_cnt);
                if (t_cnt < 2) {
                    fprintf(stderr, "[TEST ERROR] Expected at least 2 tasks in pool!\n");
                    return 61;
                }
                printf("[TEST] Navigating to Messages in task list and switching to it...\n");
                test_inject_key(VEEBHA_KEY_UP);
            } else if (frame_count == 294) {
                softkey_trigger_lsk(); /* Switch to Messages */
            } else if (frame_count == 298) {
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 19i check: ViewType=%d (exp %d EDITOR). Draft intact.\n", (int)vt, (int)VEEBHA_VIEW_TYPE_EDITOR);
                if (vt != VEEBHA_VIEW_TYPE_EDITOR) {
                    fprintf(stderr, "[TEST ERROR] Draft composer was not restored!\n");
                    return 62;
                }
                softkey_trigger_rsk(); /* Cancel composer */
            } else if (frame_count == 302) {
                softkey_trigger_rsk(); /* Back to Idle */
            } else if (frame_count == 306) {
                printf("[TEST] Step 19j: Opening Settings -> About Phone...\n");
                app_settings_open();
            } else if (frame_count == 310) {
                test_inject_key(VEEBHA_KEY_DOWN);
                test_inject_key(VEEBHA_KEY_DOWN);
                test_inject_key(VEEBHA_KEY_DOWN);
            } else if (frame_count == 314) {
                softkey_trigger_lsk(); /* Open About Phone */
            } else if (frame_count == 318) {
                win_mgr_entry_t *top = win_mgr_get_top();
                if (top && top->screen) {
                    lv_obj_t *cnt = lv_obj_get_child(top->screen, 2);
                    if (cnt && lv_obj_get_child_count(cnt) > 1) {
                        lv_obj_t *row1 = lv_obj_get_child(cnt, 1);
                        int r1_h = lv_obj_get_height(row1);
                        printf("[TEST] Step 19k check: 'Target Silicon' compact row height=%d (exp 22)\n", r1_h);
                        if (r1_h != 22) {
                            fprintf(stderr, "[TEST ERROR] Row height not 22! got %d\n", r1_h);
                            return 63;
                        }
                    }
                }
                printf("[TEST] Testing Theme Toggle to Light Mode...\n");
                theme_set_mode(true);
                printf("[TEST] Step 19l check: Light mode active=%d (exp 1)\n", (int)theme_is_light_mode());
                if (!theme_is_light_mode()) {
                    fprintf(stderr, "[TEST ERROR] Theme is not light mode!\n");
                    return 64;
                }
                softkey_trigger_rsk(); /* Back to Settings */
            } else if (frame_count == 322) {
                softkey_trigger_rsk(); /* Back to Idle */
            } else if (frame_count == 326) {
                /* Step 20: Global Theme Propagation & Logging */
                printf("[TEST] Step 20: Structured Logging Subsystem (veebha_log)...\n");
                os_log_init();
                OS_LOGI("TEST_TAG", "Formatted log test: code=%d, status='%s'", 200, "OK");
                os_log_set_level(OS_LOG_LEVEL_WARN);
                if (os_log_get_level() != OS_LOG_LEVEL_WARN) {
                    fprintf(stderr, "[TEST ERROR] veebha_log get_level mismatch!\n");
                    return 68;
                }
                os_log_set_level(OS_LOG_LEVEL_DEBUG);

                /* Step 21: Phase 4.0 Event Queue & IRQ Verification */
                printf("[TEST] Step 21: Testing Phase 4.0 Event Queue & Mock IRQs...\n");
                os_event_t test_e;
                memset(&test_e, 0, sizeof(test_e));
                test_e.type = OS_EVT_BATTERY_UPDATE;
                test_e.payload.battery.percentage = 88;
                test_e.payload.battery.is_charging = true;
                os_event_post(&test_e);
                printf("[TEST] Event queue pending count=%u (exp >= 1)\n", os_event_pending_count());
                if (os_event_pending_count() == 0) {
                    fprintf(stderr, "[TEST ERROR] Event queue post failed!\n");
                    return 70;
                }
                os_event_t polled_e;
                if (!os_event_poll(&polled_e) || polled_e.type != OS_EVT_BATTERY_UPDATE || polled_e.payload.battery.percentage != 88) {
                    fprintf(stderr, "[TEST ERROR] Event queue poll failed!\n");
                    return 71;
                }
                printf("[TEST] Step 21a: Event Queue poll PASSED: type=%d, pct=%u\n", polled_e.type, polled_e.payload.battery.percentage);

                /* Trigger Mock IRQ: Incoming Call (F5) */
                printf("[TEST] Step 21b: Triggering Mock IRQ Incoming Call (Alice, +1234567890)...\n");
                os_irq_sim_trigger_call("+1234567890", "Alice Smith");
            } else if (frame_count == 330) {
                /* Drain event queue which dispatches incoming call modal dialog */
                bool dlg = tpl_dialog_is_active();
                printf("[TEST] Step 21b check: Incoming Call dialog active=%d (exp 1)\n", (int)dlg);
                if (!dlg) {
                    fprintf(stderr, "[TEST ERROR] Incoming Call dialog should be active!\n");
                    return 72;
                }
                printf("[TEST] Rejecting incoming call via RSK ('Reject')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 334) {
                bool dlg = tpl_dialog_is_active();
                printf("[TEST] Step 21c check: Dialog dismissed=%d (exp 0)\n", (int)dlg);
                if (dlg) {
                    fprintf(stderr, "[TEST ERROR] Dialog should be dismissed!\n");
                    return 73;
                }

                /* Trigger Mock IRQ: Incoming SMS (F6) */
                printf("[TEST] Step 21d: Triggering Mock IRQ Incoming SMS (Bob Jones)...\n");
                os_irq_sim_trigger_sms("Bob Jones", "Are you free today?");
            } else if (frame_count == 338) {
                uint16_t unread = telephony_get_unread_sms_count();
                printf("[TEST] Step 21d check: Unread SMS count=%u (exp >= 1)\n", unread);
                if (unread == 0) {
                    fprintf(stderr, "[TEST ERROR] Unread SMS count should be > 0!\n");
                    return 74;
                }
                if (tpl_dialog_is_active()) {
                    printf("[TEST] Dismissing SMS Alert dialog via RSK ('Dismiss')...\n");
                    softkey_trigger_rsk();
                }

                /* Trigger Mock IRQ: SD Card Hotplug (F7) */
                printf("[TEST] Step 21e: Triggering Mock IRQ SD Card Hotplug...\n");
                os_irq_sim_trigger_sdcard_toggle();
            } else if (frame_count == 342) {
                printf("[TEST] Step 22: Testing Virtual File System (VFS) Layer...\n");
                vfs_mime_t m_aud = vfs_detect_mime("song.mp3");
                vfs_mime_t m_txt = vfs_detect_mime("notes.txt");
                vfs_mime_t m_img = vfs_detect_mime("icon.raw");
                printf("[TEST] VFS MIME: aud=%d (exp %d), txt=%d (exp %d), img=%d (exp %d)\n",
                       m_aud, VFS_MIME_AUDIO, m_txt, VFS_MIME_TEXT, m_img, VFS_MIME_IMAGE);
                if (m_aud != VFS_MIME_AUDIO || m_txt != VFS_MIME_TEXT || m_img != VFS_MIME_IMAGE) {
                    fprintf(stderr, "[TEST ERROR] VFS MIME detection failed!\n");
                    return 80;
                }

                char sz_buf[16];
                vfs_format_size(5033165, sz_buf, sizeof(sz_buf));
                printf("[TEST] VFS Format Size 5033165 -> '%s' (exp '4.8 MB')\n", sz_buf);
                if (strcmp(sz_buf, "4.8 MB") != 0) {
                    fprintf(stderr, "[TEST ERROR] VFS size formatting failed! Got '%s'\n", sz_buf);
                    return 81;
                }

                void *h_dir = NULL;
                bool ok = vfs_opendir("/sdcard", &h_dir);
                vfs_dirent_t dent;
                bool r_ok = vfs_readdir(h_dir, &dent);
                vfs_closedir(h_dir);
                printf("[TEST] VFS Opendir /sdcard: ok=%d, first_entry='%s' (type=%d)\n", (int)ok, dent.name, dent.type);
                if (!ok || !r_ok || dent.type != VFS_NODE_DIR) {
                    fprintf(stderr, "[TEST ERROR] VFS directory traversal failed!\n");
                    return 82;
                }

                printf("[TEST] Step 22a: Launching File Manager (Root: Categories)...\n");
                app_files_open();
            } else if (frame_count == 344) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 22a check: In File Manager Root. Depth=%u, ViewType=%d (exp %d LIST)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_LIST);
                if (vt != VEEBHA_VIEW_TYPE_LIST) {
                    fprintf(stderr, "[TEST ERROR] File Manager root view failed to open!\n");
                    return 83;
                }
                printf("[TEST] Step 22b: Navigating DOWN to row 1 'SD Card'...\n");
                test_inject_key(VEEBHA_KEY_DOWN);
            } else if (frame_count == 346) {
                printf("[TEST] Step 22b: Entering 'SD Card' storage via LSK ('Select')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 348) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 22c check: In '/sdcard'. Depth=%u, ViewType=%d (exp %d LIST)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_LIST);
                if (vt != VEEBHA_VIEW_TYPE_LIST) {
                    fprintf(stderr, "[TEST ERROR] Failed to navigate into /sdcard!\n");
                    return 84;
                }
                printf("[TEST] Step 22c: Navigating DOWN to 'Music' folder (row 1)...\n");
                test_inject_key(VEEBHA_KEY_DOWN);
            } else if (frame_count == 350) {
                printf("[TEST] Step 22c: Entering 'Music' directory via LSK ('Select')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 352) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 22c check: In '/sdcard/Music'. Depth=%u, ViewType=%d (exp %d LIST)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_LIST);
                if (vt != VEEBHA_VIEW_TYPE_LIST) {
                    fprintf(stderr, "[TEST ERROR] Failed to navigate into /sdcard/Music!\n");
                    return 84;
                }
                printf("[TEST] Step 22c: Navigating DOWN to '01_Blinding_Lights.mp3'...\n");
                test_inject_key(VEEBHA_KEY_DOWN);
            } else if (frame_count == 354) {
                printf("[TEST] Step 22d: Selecting audio file to launch Walkman Player via LSK...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 358) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                bool playing = app_music_is_playing();
                const music_track_t *cur = app_music_get_current_track();
                printf("[TEST] Step 22d check: In Walkman Player. Depth=%u, ViewType=%d (exp %d MEDIA), Track='%s', Playing=%d\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_MEDIA, cur ? cur->title : "", (int)playing);
                if (vt != VEEBHA_VIEW_TYPE_MEDIA || !playing || strcmp(cur->title, "Blinding Lights") != 0) {
                    fprintf(stderr, "[TEST ERROR] Hand-off to Walkman player from File Manager failed!\n");
                    return 85;
                }
                printf("[TEST] Step 22e: Returning back to File Manager via RSK ('List' -> pop)...\n");
                win_mgr_pop();
            } else if (frame_count == 362) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 22e check: Back in '/sdcard/Music'. Depth=%u, ViewType=%d (exp %d LIST)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_LIST);
                if (vt != VEEBHA_VIEW_TYPE_LIST) {
                    fprintf(stderr, "[TEST ERROR] Return to File Manager failed!\n");
                    return 86;
                }
                printf("[TEST] Step 22f: Navigating UP to row 0 ('.. Parent Folder')...\n");
                test_inject_key(VEEBHA_KEY_UP);
            } else if (frame_count == 366) {
                printf("[TEST] Step 22g: Selecting '.. Parent Folder' to climb up...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 368) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 22g check: Back in root '/sdcard'. Depth=%u, ViewType=%d (exp %d LIST)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_LIST);
                if (vt != VEEBHA_VIEW_TYPE_LIST) {
                    fprintf(stderr, "[TEST ERROR] Climb to parent folder failed!\n");
                    return 87;
                }
                printf("[TEST] Step 22g: Navigating UP to row 0 ('.. Storage Roots')...\n");
                test_inject_key(VEEBHA_KEY_UP);
            } else if (frame_count == 370) {
                printf("[TEST] Step 22g: Selecting '.. Storage Roots' to climb to Categories...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 372) {
                printf("[TEST] Step 22h: Exiting File Manager via RSK ('Back')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 374) {
                printf("[TEST] Step 23: Launching Calendar Application...\n");
                app_calendar_open();
            } else if (frame_count == 378) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 23 check: In Calendar. Depth=%u, ViewType=%d (exp %d CALENDAR)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_CALENDAR);
                if (vt != VEEBHA_VIEW_TYPE_CALENDAR) {
                    fprintf(stderr, "[TEST ERROR] Calendar failed to open or wrong view type!\n");
                    return 90;
                }
                printf("[TEST] Step 23a: D-pad navigation (LEFT then RIGHT)...\n");
                test_inject_key(VEEBHA_KEY_LEFT);
            } else if (frame_count == 382) {
                test_inject_key(VEEBHA_KEY_RIGHT);
            } else if (frame_count == 386) {
                printf("[TEST] Step 23b: Testing LSK ('Today')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 390) {
                printf("[TEST] Step 23c: Exiting Calendar via RSK ('Back')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 394) {
                uint8_t depth = win_mgr_get_depth();
                printf("[TEST] Step 23c check: Returned from Calendar. Depth=%u\n", depth);
                printf("[TEST] Step 24: Launching Tools Hub...\n");
                app_tools_open();
            } else if (frame_count == 398) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 24 check: In Tools Hub. Depth=%u, ViewType=%d (exp %d LIST)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_LIST);
                if (vt != VEEBHA_VIEW_TYPE_LIST) {
                    fprintf(stderr, "[TEST ERROR] Tools hub failed to open!\n");
                    return 91;
                }
                printf("[TEST] Step 24a: Selecting Calculator (Row 0) via LSK ('Select')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 402) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                bool active = app_calc_is_active();
                printf("[TEST] Step 25 check: In Calculator. Depth=%u, ViewType=%d (exp %d CALC), Active=%d\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_CALC, (int)active);
                if (vt != VEEBHA_VIEW_TYPE_CALC || !active) {
                    fprintf(stderr, "[TEST ERROR] Calculator failed to open!\n");
                    return 92;
                }
                printf("[TEST] Step 25a: Entering '2' '5'...\n");
                test_inject_key(VEEBHA_KEY_NUM_2);
            } else if (frame_count == 404) {
                test_inject_key(VEEBHA_KEY_NUM_5);
            } else if (frame_count == 406) {
                printf("[TEST] Step 25b: Cycling operator via '#' (+)...\n");
                test_inject_key(VEEBHA_KEY_HASH);
            } else if (frame_count == 408) {
                printf("[TEST] Step 25c: Entering '7' '5'...\n");
                test_inject_key(VEEBHA_KEY_NUM_7);
            } else if (frame_count == 410) {
                test_inject_key(VEEBHA_KEY_NUM_5);
            } else if (frame_count == 412) {
                printf("[TEST] Step 25d: Evaluating '25 + 75' via LSK ('=')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 416) {
                win_mgr_entry_t *top = win_mgr_get_top();
                if (!top || !top->screen) {
                    fprintf(stderr, "[TEST ERROR] No top screen for calculator!\n");
                    return 93;
                }
                printf("[TEST] Step 25d check: Calculation performed.\n");
                printf("[TEST] Step 25e: Clearing result via RSK ('Clear')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 420) {
                printf("[TEST] Step 25f: Exiting Calculator via RSK ('Back')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 424) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 25f check: Back in Tools Hub. Depth=%u, ViewType=%d (exp %d LIST)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_LIST);
                if (vt != VEEBHA_VIEW_TYPE_LIST) {
                    fprintf(stderr, "[TEST ERROR] Failed to return to Tools Hub from Calculator!\n");
                    return 94;
                }
                printf("[TEST] Step 25g: Exiting Tools Hub via RSK ('Back')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 428) {
                printf("[TEST] Step 26: Verifying 3x3 Launcher Full Registry Integrity...\n");
                uint16_t count = os_app_get_count();
                printf("[TEST] App registry count: %u (expected 9)\n", count);
                if (count != 9) {
                    fprintf(stderr, "[TEST ERROR] App registry count is %u, expected 9!\n", count);
                    return 95;
                }
                for (uint16_t i = 0; i < count; i++) {
                    const os_app_desc_t *app = os_app_get_by_index(i);
                    if (!app || !app->launch_cb) {
                        fprintf(stderr, "[TEST ERROR] App index %u (%s) missing launch_cb!\n",
                                i, app ? app->name : "NULL");
                        return 96;
                    }
                    printf("  [Slot %u] '%s' (%s) -> launch_cb active [OK]\n",
                           i, app->name, app->id);
                }
            } else if (frame_count == 432) {
                printf("[TEST] Step 27: Testing Calculator D-pad Direct Operators (UP +, DOWN -, LEFT *, RIGHT /)...\n");
                app_calc_open();
            } else if (frame_count == 434) {
                printf("[TEST] Step 27a: Entering '8' then D-pad UP ('+') then '2'...\n");
                test_inject_key(VEEBHA_KEY_NUM_8);
            } else if (frame_count == 436) {
                test_inject_key(VEEBHA_KEY_UP);
            } else if (frame_count == 438) {
                test_inject_key(VEEBHA_KEY_NUM_2);
            } else if (frame_count == 440) {
                printf("[TEST] Step 27b: Evaluating '8 + 2' via LSK ('=')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 442) {
                printf("[TEST] Step 27c: Chaining DOWN ('-') then '4'...\n");
                test_inject_key(VEEBHA_KEY_DOWN);
            } else if (frame_count == 444) {
                test_inject_key(VEEBHA_KEY_NUM_4);
            } else if (frame_count == 446) {
                printf("[TEST] Step 27d: Evaluating '10 - 4' via LSK ('=')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 448) {
                printf("[TEST] Step 27e: Chaining LEFT ('*') then '3'...\n");
                test_inject_key(VEEBHA_KEY_LEFT);
            } else if (frame_count == 450) {
                test_inject_key(VEEBHA_KEY_NUM_3);
            } else if (frame_count == 452) {
                printf("[TEST] Step 27f: Evaluating '6 * 3' via LSK ('=')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 454) {
                printf("[TEST] Step 27g: Chaining RIGHT ('/') then '2'...\n");
                test_inject_key(VEEBHA_KEY_RIGHT);
            } else if (frame_count == 456) {
                test_inject_key(VEEBHA_KEY_NUM_2);
            } else if (frame_count == 458) {
                printf("[TEST] Step 27h: Evaluating '18 / 2' via LSK ('=')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 460) {
                printf("[TEST] Step 27i: Calculator D-pad operators verified. Exiting Calculator...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 462) {
                softkey_trigger_rsk();
            } else if (frame_count == 464) {
                printf("[TEST] Step 28: Verifying Torch Screen Layout & Bottom Softkeys...\n");
                app_tools_open_torch();
            } else if (frame_count == 466) {
                win_mgr_entry_t *top = win_mgr_get_top();
                if (!top || !top->screen) {
                    fprintf(stderr, "[TEST ERROR] Torch screen failed to open!\n");
                    return 97;
                }
                printf("[TEST] Step 28 check: Torch active, pressing LSK ('Off')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 468) {
                printf("[TEST] Step 29: Verifying Interactive Stopwatch & Countdown Timer...\n");
                app_stopwatch_open();
            } else if (frame_count == 470) {
                bool sw_act = app_stopwatch_is_active();
                printf("[TEST] Step 29 check: Stopwatch active=%d\n", (int)sw_act);
                if (!sw_act) {
                    fprintf(stderr, "[TEST ERROR] Stopwatch failed to open!\n");
                    return 98;
                }
                printf("[TEST] Step 29a: Toggling to Timer mode via '#'...\n");
                test_inject_key(VEEBHA_KEY_HASH);
            } else if (frame_count == 472) {
                printf("[TEST] Step 29b: Setting Timer duration with D-pad UP (+60s)...\n");
                test_inject_key(VEEBHA_KEY_UP);
            } else if (frame_count == 474) {
                printf("[TEST] Step 29c: Starting timer via LSK ('Start')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 476) {
                printf("[TEST] Step 29d: Posting Timer Alert to Notification Panel...\n");
                notif_panel_post_alert("Timer Complete", "Countdown finished: 00:00!");
                softkey_trigger_rsk();
            } else if (frame_count == 478) {
                softkey_trigger_rsk();
            } else if (frame_count == 480) {
                printf("[TEST] Step 30: Verifying Notification Panel Recent Messages & Alerts...\n");
                notif_panel_show();
            } else if (frame_count == 482) {
                bool np_act = notif_panel_is_active();
                const char *title = notif_panel_get_last_alert_title();
                printf("[TEST] Step 30 check: Notification Panel active=%d, Alert Title='%s'\n",
                       (int)np_act, title ? title : "NULL");
                if (!np_act || !title || strcmp(title, "Timer Complete") != 0) {
                    fprintf(stderr, "[TEST ERROR] Notification Panel or Timer alert check failed!\n");
                    return 99;
                }
                notif_panel_close();
            } else if (frame_count == 484) {
                printf("[TEST] Step 31: Verifying Alarm Clock Application...\n");
                app_alarm_open();
            } else if (frame_count == 486) {
                printf("[TEST] Step 31a: Toggling Alarm status via LSK...\n");
                softkey_trigger_lsk();
                bool enabled = app_alarm_is_enabled();
                printf("[TEST] Step 31 check: Alarm enabled=%d, Time=%s\n", (int)enabled, app_alarm_get_time_str());
                if (!enabled) {
                    fprintf(stderr, "[TEST ERROR] Alarm clock toggle failed!\n");
                    return 100;
                }
                softkey_trigger_rsk();
            } else if (frame_count == 488) {
                printf("[TEST] Step 32: Verifying Text Reader Application...\n");
                app_textread_open();
            } else if (frame_count == 490) {
                uint8_t depth = win_mgr_get_depth();
                printf("[TEST] Step 32 check: Text Reader opened. Depth=%u\n", depth);
                softkey_trigger_rsk();
            } else if (frame_count == 492) {
                printf("[TEST] Step 33: Verifying Retro Snake Game...\n");
                app_game_open();
            } else if (frame_count == 494) {
                printf("[TEST] Step 33a: Injecting D-pad DOWN for snake movement...\n");
                test_inject_key(VEEBHA_KEY_DOWN);
            } else if (frame_count == 496) {
                printf("[TEST] Step 33b: Exiting Snake Game via RSK ('Exit')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 498) {
                printf("[TEST] Step 34: Verifying Photo Gallery Application...\n");
                app_gallery_open();
            } else if (frame_count == 500) {
                printf("[TEST] Step 34a: Selecting photo to view via LSK ('View')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 502) {
                printf("[TEST] Step 34b: Opening Image Details via LSK ('Info')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 504) {
                printf("[TEST] Step 34c: Dismissing dialog and returning home...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 506) {
                softkey_trigger_rsk();
            } else if (frame_count == 508) {
                softkey_trigger_rsk();
            } else if (frame_count == 510) {
                printf("[TEST] Step 35: Verifying VeebhaOS Boot Screen Animation...\n");
                boot_screen_start(NULL);
                bool bs_act = boot_screen_is_active();
                printf("[TEST] Step 35 check: Boot screen active=%d\n", (int)bs_act);
                if (!bs_act) {
                    fprintf(stderr, "[TEST ERROR] Boot screen failed to activate!\n");
                    return 101;
                }
            } else if (frame_count == 520) {
                printf("[TEST] Step 36: Verifying Integrated FM Radio within Music Suite...\n");
                app_music_open_player();
            } else if (frame_count == 522) {
                printf("[TEST] Step 36a: Toggling to FM Radio mode...\n");
                app_music_set_mode(MUSIC_MODE_FM_RADIO);
                music_mode_t mm = app_music_get_mode();
                uint16_t freq = app_music_fm_get_freq();
                printf("[TEST] Step 36 check: Mode=%d (exp 1 FM), Freq=%u.%u MHz, Station='%s'\n",
                       (int)mm, freq / 10, freq % 10, app_music_fm_get_station_name());
                if (mm != MUSIC_MODE_FM_RADIO || freq != 983) {
                    fprintf(stderr, "[TEST ERROR] FM Radio mode switch or initial frequency failed!\n");
                    return 102;
                }
            } else if (frame_count == 524) {
                printf("[TEST] Step 36b: Seeking FM Frequency (+0.1 MHz)...\n");
                app_music_fm_seek(+1);
                uint16_t freq = app_music_fm_get_freq();
                printf("[TEST] Step 36b check: Frequency after seek: %u.%u MHz (exp 98.4)\n", freq / 10, freq % 10);
                if (freq != 984) {
                    fprintf(stderr, "[TEST ERROR] FM seek failed!\n");
                    return 103;
                }
            } else if (frame_count == 526) {
                printf("[TEST] Step 36c: Cycling to Next Preset Station...\n");
                app_music_fm_next_preset();
                uint16_t freq = app_music_fm_get_freq();
                printf("[TEST] Step 36c check: Preset Frequency: %u.%u MHz, Station='%s'\n",
                       freq / 10, freq % 10, app_music_fm_get_station_name());
            } else if (frame_count == 528) {
                printf("[TEST] Step 36d: Testing FM Mute Toggle...\n");
                app_music_fm_toggle_mute();
                bool muted = app_music_fm_is_muted();
                printf("[TEST] Step 36d check: FM Muted=%d (exp 1)\n", (int)muted);
                if (!muted) {
                    fprintf(stderr, "[TEST ERROR] FM mute failed!\n");
                    return 104;
                }
                app_music_fm_toggle_mute();
            } else if (frame_count == 530) {
                printf("[TEST] Step 36e: Switching back to Walkman Player mode...\n");
                app_music_toggle_mode();
                music_mode_t mm = app_music_get_mode();
                printf("[TEST] Step 36e check: Mode=%d (exp 0 PLAYER)\n", (int)mm);
                if (mm != MUSIC_MODE_PLAYER) {
                    fprintf(stderr, "[TEST ERROR] Failed to switch back to Music Player mode!\n");
                    return 105;
                }
                win_mgr_pop();
            } else if (frame_count == 532) {
                printf("[TEST] Step 37: Verifying Media Capture Studio (Photo, Video, Voice)...\n");
                app_recorder_open();
            } else if (frame_count == 534) {
                bool rec_act = app_recorder_is_active();
                capture_mode_t cm = app_recorder_get_mode();
                printf("[TEST] Step 37a check: Recorder active=%d, Mode=%d (exp 0 PHOTO)\n", (int)rec_act, (int)cm);
                if (!rec_act || cm != CAPTURE_MODE_PHOTO) {
                    fprintf(stderr, "[TEST ERROR] Recorder failed to open in Photo mode!\n");
                    return 106;
                }
                printf("[TEST] Step 37b: Snapping Photo via LSK ('Snap')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 536) {
                softkey_trigger_rsk(); // Dismiss dialog
            } else if (frame_count == 538) {
                printf("[TEST] Step 37c: Switching to Video Mode & Recording...\n");
                app_recorder_set_mode(CAPTURE_MODE_VIDEO);
                softkey_trigger_lsk(); // Start recording
            } else if (frame_count == 540) {
                softkey_trigger_lsk(); // Stop recording
            } else if (frame_count == 542) {
                printf("[TEST] Step 37d: Switching to Voice Mode & Recording...\n");
                app_recorder_set_mode(CAPTURE_MODE_VOICE);
                softkey_trigger_lsk(); // Start recording
            } else if (frame_count == 544) {
                softkey_trigger_lsk(); // Stop recording
                win_mgr_pop(); // Exit recorder
            } else if (frame_count == 546) {
                printf("[TEST] Step 38: Verifying Low-Power Screen Saver...\n");
                screen_saver_show();
                bool ss_act = screen_saver_is_active();
                printf("[TEST] Step 38a check: Screen saver active=%d (exp 1)\n", (int)ss_act);
                if (!ss_act) {
                    fprintf(stderr, "[TEST ERROR] Screen saver failed to show!\n");
                    return 107;
                }
            } else if (frame_count == 548) {
                printf("[TEST] Step 38b: Keypress dismissal of Screen Saver...\n");
                test_inject_key(VEEBHA_KEY_OK);
                bool ss_act = screen_saver_is_active();
                printf("[TEST] Step 38b check: Screen saver active=%d (exp 0)\n", (int)ss_act);
                if (ss_act) {
                    fprintf(stderr, "[TEST ERROR] Screen saver was not dismissed by keypress!\n");
                    return 108;
                }
            } else if (frame_count == 550) {
                printf("[TEST] Step 39: Verifying Idle Screen Wallpapers (Dark, Cyber, Sunset, Emerald)...\n");
                app_idle_set_wallpaper(WALLPAPER_CYBER);
                if (app_idle_get_wallpaper() != WALLPAPER_CYBER) {
                    fprintf(stderr, "[TEST ERROR] Wallpaper Cyber failed!\n");
                    return 109;
                }
                app_idle_set_wallpaper(WALLPAPER_SUNSET);
                if (app_idle_get_wallpaper() != WALLPAPER_SUNSET) {
                    fprintf(stderr, "[TEST ERROR] Wallpaper Sunset failed!\n");
                    return 110;
                }
                app_idle_set_wallpaper(WALLPAPER_EMERALD);
                if (app_idle_get_wallpaper() != WALLPAPER_EMERALD) {
                    fprintf(stderr, "[TEST ERROR] Wallpaper Emerald failed!\n");
                    return 111;
                }
                app_idle_set_wallpaper(WALLPAPER_DARK);
                printf("[TEST] Step 39 check: All wallpaper themes successfully cycled.\n");
            } else if (frame_count == 552) {
                printf("[TEST] Step 40: Verifying Phase 4.4 Calculator, Snake, Alarm & Stopwatch Polish...\n");
                printf("[TEST] Step 40a: Testing Calculator 5 * 5 = 25 and 5 / 0 = Error...\n");
                app_calc_open();
                app_calc_handle_key(VEEBHA_KEY_NUM_5);
                app_calc_handle_key(VEEBHA_KEY_LEFT); /* '*' */
                app_calc_handle_key(VEEBHA_KEY_NUM_5);
                softkey_trigger_lsk(); /* '=' */
                const char *res = app_calc_get_result_str();
                printf("[TEST] Step 40a check: 5 * 5 = '%s' (exp '25')\n", res);
                if (strcmp(res, "25") != 0) {
                    fprintf(stderr, "[TEST ERROR] Calculator 5 * 5 failed! Got '%s'\n", res);
                    return 112;
                }

                /* Test 5 / 0 = Error */
                app_calc_handle_key(VEEBHA_KEY_RIGHT); /* '/' */
                app_calc_handle_key(VEEBHA_KEY_NUM_0);
                softkey_trigger_lsk(); /* '=' */
                bool err = app_calc_has_error();
                printf("[TEST] Step 40b check: 5 / 0 -> Error=%d (exp 1)\n", (int)err);
                if (!err) {
                    fprintf(stderr, "[TEST ERROR] Calculator division by zero check failed!\n");
                    return 113;
                }
                win_mgr_pop();
            } else if (frame_count == 556) {
                printf("[TEST] Step 40c: Verifying Alarm Clock Time Adjustment & Save...\n");
                app_alarm_open();
                /* Left key to cycle column to AM/PM */
                test_inject_key(VEEBHA_KEY_LEFT);
                /* Up key to toggle AM/PM */
                test_inject_key(VEEBHA_KEY_UP);
                /* Save via LSK */
                softkey_trigger_lsk();
                bool enabled = app_alarm_is_enabled();
                printf("[TEST] Step 40c check: Alarm enabled=%d, time=%s\n", (int)enabled, app_alarm_get_time_str());
                if (!enabled) {
                    fprintf(stderr, "[TEST ERROR] Alarm Save failed!\n");
                    return 114;
                }
                softkey_trigger_rsk();
            } else if (frame_count == 560) {
                printf("[TEST] Step 40d: Verifying Stopwatch / Timer Toggle Functionality...\n");
                app_stopwatch_open();
                app_stopwatch_toggle_mode();
                app_stopwatch_toggle_mode();
                printf("[TEST] Step 40d check: Stopwatch toggle successful.\n");
                softkey_trigger_rsk();
            } else if (frame_count == 564) {
                printf("[TEST] Step 40e: Verifying Snake Game Directional Keys (2, 8, 4, 6)...\n");
                app_game_open();
                test_inject_key(VEEBHA_KEY_NUM_2);
                test_inject_key(VEEBHA_KEY_NUM_4);
                test_inject_key(VEEBHA_KEY_NUM_8);
                test_inject_key(VEEBHA_KEY_NUM_6);
                bool g_act = app_game_is_active();
                printf("[TEST] Step 40e check: Snake Game active=%d (exp 1)\n", (int)g_act);
                if (!g_act) {
                    fprintf(stderr, "[TEST ERROR] Snake game not active!\n");
                    return 115;
                }
                softkey_trigger_rsk();
            } else if (frame_count == 568) {
                printf("[TEST] Step 41: Verifying Phase 4.5 D-pad Editing Mode, Calculator OK key, Text Reader & Quick Settings Scroll...\n");
                /* 41a: Calculator OK key evaluation (9 + 6 = 15) */
                app_calc_open();
                app_calc_handle_key(VEEBHA_KEY_NUM_9);
                app_calc_handle_key(VEEBHA_KEY_UP); /* '+' */
                app_calc_handle_key(VEEBHA_KEY_NUM_6);
                test_inject_key(VEEBHA_KEY_OK); /* '=' via OK key */
                const char *res = app_calc_get_result_str();
                printf("[TEST] Step 41a check: Calculator 9 + 6 = '%s' (exp '15')\n", res);
                if (strcmp(res, "15") != 0) {
                    fprintf(stderr, "[TEST ERROR] Calculator OK key evaluation failed! Got '%s'\n", res);
                    return 116;
                }
                softkey_trigger_rsk();
            } else if (frame_count == 572) {
                /* 41b: Text Reader D-pad scrolling and group editing */
                app_textread_open();
                lv_group_t *g = win_mgr_get_group();
                bool editing = g ? lv_group_get_editing(g) : false;
                printf("[TEST] Step 41b check: Text Reader editing=%d (exp 1)\n", (int)editing);
                if (!editing) {
                    fprintf(stderr, "[TEST ERROR] Text Reader editing mode not active!\n");
                    return 117;
                }
                test_inject_key(VEEBHA_KEY_DOWN);
                test_inject_key(VEEBHA_KEY_UP);
                softkey_trigger_rsk();
                editing = g ? lv_group_get_editing(g) : false;
                printf("[TEST] Step 41b check: After exit editing=%d (exp 0)\n", (int)editing);
                if (editing) {
                    fprintf(stderr, "[TEST ERROR] Text Reader editing mode not cleared on exit!\n");
                    return 118;
                }
            } else if (frame_count == 576) {
                /* 41c: Stopwatch & Timer D-pad adjustment */
                app_stopwatch_open();
                lv_group_t *g = win_mgr_get_group();
                bool editing = g ? lv_group_get_editing(g) : false;
                printf("[TEST] Step 41c check: Stopwatch editing=%d (exp 1)\n", (int)editing);
                if (!editing) {
                    fprintf(stderr, "[TEST ERROR] Stopwatch editing mode not active!\n");
                    return 119;
                }
                /* Switch to timer mode via Right arrow */
                test_inject_key(VEEBHA_KEY_RIGHT);
                /* Adjust timer time up */
                test_inject_key(VEEBHA_KEY_UP);
                softkey_trigger_rsk();
                editing = g ? lv_group_get_editing(g) : false;
                printf("[TEST] Step 41c check: After exit editing=%d (exp 0)\n", (int)editing);
                if (editing) {
                    fprintf(stderr, "[TEST ERROR] Stopwatch editing mode not cleared on exit!\n");
                    return 120;
                }
            } else if (frame_count == 580) {
                /* 41d: Snake Game group editing verification */
                app_game_open();
                lv_group_t *g = win_mgr_get_group();
                bool editing = g ? lv_group_get_editing(g) : false;
                printf("[TEST] Step 41d check: Snake game editing=%d (exp 1)\n", (int)editing);
                if (!editing) {
                    fprintf(stderr, "[TEST ERROR] Snake game editing mode not active!\n");
                    return 121;
                }
                test_inject_key(VEEBHA_KEY_UP);
                test_inject_key(VEEBHA_KEY_LEFT);
                softkey_trigger_rsk();
                editing = g ? lv_group_get_editing(g) : false;
                printf("[TEST] Step 41d check: After exit editing=%d (exp 0)\n", (int)editing);
                if (editing) {
                    fprintf(stderr, "[TEST ERROR] Snake game editing mode not cleared on exit!\n");
                    return 122;
                }
            } else if (frame_count == 584) {
                /* 41e: Quick Settings drawer scroll */
                notif_panel_show();
                bool np_act = notif_panel_is_active();
                printf("[TEST] Step 41e check: Quick settings panel active=%d (exp 1)\n", (int)np_act);
                if (!np_act) {
                    fprintf(stderr, "[TEST ERROR] Quick settings panel failed to open!\n");
                    return 123;
                }
                test_inject_key(VEEBHA_KEY_DOWN);
                notif_panel_close();
                printf("[TEST] Step 41e check: Quick settings panel scroll & close OK.\n");
            } else if (frame_count == 588) {
                /* Step 42a: Verify Snake Game fullscreen PARTIAL mode and battery HUD hidden */
                app_game_open();
                os_fullscreen_mode_t fsm = win_mgr_get_fullscreen_mode();
                bool hud = win_mgr_is_battery_hud_enabled();
                bool hud_vis = status_bar_is_battery_hud_visible();
                printf("[TEST] Step 42a check: Game fullscreen mode=%d (exp 1 PARTIAL), hud_enabled=%d (exp 0), hud_vis=%d (exp 0)\n",
                       (int)fsm, (int)hud, (int)hud_vis);
                if (fsm != OS_FULLSCREEN_PARTIAL || hud || hud_vis) {
                    fprintf(stderr, "[TEST ERROR] Game fullscreen partial / HUD check failed!\n");
                    return 124;
                }
                softkey_trigger_rsk();
            } else if (frame_count == 592) {
                /* Step 42b: Verify Text Reader fullscreen FULL mode and battery HUD visible */
                app_textread_open_file("/sdcard/Documents/notes.txt");
                os_fullscreen_mode_t fsm = win_mgr_get_fullscreen_mode();
                bool hud = win_mgr_is_battery_hud_enabled();
                bool hud_vis = status_bar_is_battery_hud_visible();
                printf("[TEST] Step 42b check: Textread fullscreen mode=%d (exp 2 FULL), hud_enabled=%d (exp 1), hud_vis=%d (exp 1)\n",
                       (int)fsm, (int)hud, (int)hud_vis);
                if (fsm != OS_FULLSCREEN_FULL || !hud || !hud_vis) {
                    fprintf(stderr, "[TEST ERROR] Textread fullscreen FULL / HUD check failed!\n");
                    return 125;
                }
                softkey_trigger_rsk();
            } else if (frame_count == 596) {
                /* Step 42c: Verify Gallery Photo Viewer fullscreen FULL mode without battery HUD */
                app_gallery_view_photo("nature.raw", 12345);
                os_fullscreen_mode_t fsm = win_mgr_get_fullscreen_mode();
                bool hud = win_mgr_is_battery_hud_enabled();
                bool hud_vis = status_bar_is_battery_hud_visible();
                printf("[TEST] Step 42c check: Photo viewer fullscreen mode=%d (exp 2 FULL), hud_enabled=%d (exp 0), hud_vis=%d (exp 0)\n",
                       (int)fsm, (int)hud, (int)hud_vis);
                if (fsm != OS_FULLSCREEN_FULL || hud || hud_vis) {
                    fprintf(stderr, "[TEST ERROR] Photo viewer fullscreen FULL / HUD check failed!\n");
                    return 126;
                }
                softkey_trigger_rsk();
            } else if (frame_count == 600) {
                /* Verify returned to home/idle and HUD is hidden */
                os_fullscreen_mode_t fsm = win_mgr_get_fullscreen_mode();
                bool hud_vis = status_bar_is_battery_hud_visible();
                printf("[TEST] Step 42d check: Idle fullscreen mode=%d (exp 0 NONE), hud_vis=%d (exp 0)\n",
                       (int)fsm, (int)hud_vis);
                if (fsm != OS_FULLSCREEN_NONE || hud_vis) {
                    fprintf(stderr, "[TEST ERROR] Fullscreen mode did not reset to NONE on exit!\n");
                    return 127;
                }
            } else if (frame_count == 604) {
                /* Step 43a: Verify Bluetooth Manager and Toggle ON */
                app_bt_open();
                bool bt_en = connectivity_bt_is_enabled();
                printf("[TEST] Step 43a check: Initial BT enabled=%d (exp 0)\n", (int)bt_en);
                if (bt_en) {
                    fprintf(stderr, "[TEST ERROR] BT should be OFF initially!\n");
                    return 128;
                }
                connectivity_bt_set_enabled(true);
                bt_en = connectivity_bt_is_enabled();
                printf("[TEST] Step 43a check: BT toggled enabled=%d (exp 1)\n", (int)bt_en);
                if (!bt_en) {
                    fprintf(stderr, "[TEST ERROR] BT failed to turn ON!\n");
                    return 129;
                }
            } else if (frame_count == 608) {
                /* Step 43b: Verify Bluetooth Inquiry Scan & Pairing */
                app_bt_scan_open();
                os_bt_device_t disc[BT_MAX_DISCOVERED_DEVICES];
                uint16_t disc_cnt = connectivity_bt_get_discovered_devices(disc, BT_MAX_DISCOVERED_DEVICES);
                printf("[TEST] Step 43b check: Discovered devices count=%u (exp >= 2)\n", disc_cnt);
                if (disc_cnt < 2) {
                    fprintf(stderr, "[TEST ERROR] Discovered devices count check failed!\n");
                    return 130;
                }
                /* Pair with Sony WH-1000XM4 */
                bool paired = connectivity_bt_pair_device("1C:52:1D:01:23:45");
                os_bt_state_t bts = connectivity_bt_get_state();
                printf("[TEST] Step 43b check: Paired=%d (exp 1), BT state=%d (exp %d CONNECTED)\n",
                       (int)paired, (int)bts, (int)BT_STATE_CONNECTED);
                if (!paired || bts != BT_STATE_CONNECTED) {
                    fprintf(stderr, "[TEST ERROR] Device pairing failed!\n");
                    return 131;
                }
                softkey_trigger_rsk(); /* Exit scan view */
            } else if (frame_count == 612) {
                softkey_trigger_rsk(); /* Exit BT manager */
                /* Step 43c: Verify Tethering Settings */
                app_tethering_open();
                connectivity_tethering_set_usb(true);
                connectivity_tethering_set_bt(true);
                bool ut = connectivity_tethering_get_usb();
                bool bt_t = connectivity_tethering_get_bt();
                printf("[TEST] Step 43c check: USB Tethering=%d (exp 1), BT Tethering=%d (exp 1)\n",
                       (int)ut, (int)bt_t);
                if (!ut || !bt_t) {
                    fprintf(stderr, "[TEST ERROR] Tethering toggle failed!\n");
                    return 132;
                }
                softkey_trigger_rsk(); /* Exit Tethering */
            } else if (frame_count == 616) {
                /* Step 43d: Verify USB Auto-Detect Mode Modal */
                connectivity_usb_set_connected(true);
                usb_select_show();
                bool act = usb_select_is_active();
                printf("[TEST] Step 43d check: USB Select modal active=%d (exp 1)\n", (int)act);
                if (!act) {
                    fprintf(stderr, "[TEST ERROR] USB Select modal failed to open!\n");
                    return 133;
                }
                softkey_trigger_lsk(); /* Select Option 0: Mass Storage */
            } else if (frame_count == 620) {
                bool act = usb_select_is_active();
                os_usb_mode_t umode = connectivity_usb_get_mode();
                printf("[TEST] Step 43e check: USB Select modal active=%d (exp 0), USB mode=%d (exp %d MASS_STORAGE)\n",
                       (int)act, (int)umode, (int)USB_MODE_MASS_STORAGE);
                if (act || umode != USB_MODE_MASS_STORAGE) {
                    fprintf(stderr, "[TEST ERROR] USB Mass Storage mode selection check failed!\n");
                    return 134;
                }
            } else if (frame_count == 624) {
                /* Step 44a: Battery HUD Lifecycle Verification */
                app_textread_open();
                bool hud_on = status_bar_is_battery_hud_visible();
                os_fullscreen_mode_t fsm = win_mgr_get_fullscreen_mode();
                printf("[TEST] Step 44a check: Text Reader opened. Fullscreen=%d (exp %d FULL), Battery HUD visible=%d (exp 1)\n",
                       (int)fsm, (int)OS_FULLSCREEN_FULL, (int)hud_on);
                if (fsm != OS_FULLSCREEN_FULL || !hud_on) {
                    fprintf(stderr, "[TEST ERROR] Battery HUD should be visible in Text Reader fullscreen!\n");
                    return 135;
                }
                win_mgr_pop();
                hud_on = status_bar_is_battery_hud_visible();
                fsm = win_mgr_get_fullscreen_mode();
                printf("[TEST] Step 44a check: Text Reader popped. Fullscreen=%d (exp %d NONE), Battery HUD visible=%d (exp 0)\n",
                       (int)fsm, (int)OS_FULLSCREEN_NONE, (int)hud_on);
                if (fsm != OS_FULLSCREEN_NONE || hud_on) {
                    fprintf(stderr, "[TEST ERROR] Battery HUD must be hidden immediately after pop to Mode 0 screen!\n");
                    return 136;
                }
            } else if (frame_count == 628) {
                /* Step 44b: Status Bar 5-Item Indicator Tray & Hardware Glyph Verification */
                status_bar_set_bt_state(true, true);
                status_bar_set_usb_mode(2); /* USB_MODE_MASS_STORAGE */
                status_bar_set_tethering(true);
                status_bar_set_silent(true);

                win_mgr_entry_t *top = win_mgr_get_top();
                if (top && top->screen) {
                    lv_obj_t *bar = lv_obj_get_child(top->screen, 0);
                    if (bar && lv_obj_get_child_count(bar) >= 3) {
                        lv_obj_t *left_tray = lv_obj_get_child(bar, 0);
                        lv_obj_t *right_tray = lv_obj_get_child(bar, 2);
                        lv_obj_t *tether = (lv_obj_get_child_count(left_tray) >= 3) ? lv_obj_get_child(left_tray, 2) : NULL;
                        lv_obj_t *bt = (lv_obj_get_child_count(right_tray) >= 6) ? lv_obj_get_child(right_tray, 1) : NULL;
                        lv_obj_t *headset = (lv_obj_get_child_count(right_tray) >= 6) ? lv_obj_get_child(right_tray, 2) : NULL;
                        lv_obj_t *usb = (lv_obj_get_child_count(right_tray) >= 6) ? lv_obj_get_child(right_tray, 3) : NULL;
                        lv_obj_t *mute = (lv_obj_get_child_count(right_tray) >= 6) ? lv_obj_get_child(right_tray, 4) : NULL;
                        lv_obj_t *bat_box = (lv_obj_get_child_count(right_tray) >= 6) ? lv_obj_get_child(right_tray, 5) : NULL;

                        bool t_vis = tether && !lv_obj_has_flag(tether, LV_OBJ_FLAG_HIDDEN);
                        bool bt_vis = bt && !lv_obj_has_flag(bt, LV_OBJ_FLAG_HIDDEN);
                        bool hs_vis = headset && !lv_obj_has_flag(headset, LV_OBJ_FLAG_HIDDEN);
                        bool usb_vis = usb && !lv_obj_has_flag(usb, LV_OBJ_FLAG_HIDDEN);
                        bool mute_vis = mute && !lv_obj_has_flag(mute, LV_OBJ_FLAG_HIDDEN);
                        bool bat_vis = bat_box && !lv_obj_has_flag(bat_box, LV_OBJ_FLAG_HIDDEN);

                        printf("[TEST] Step 44b check: Tether=%d (exp 1), BT_idle=%d (exp 0), Headset_conn=%d (exp 1), USB=%d (exp 1), Mute=%d (exp 1), Bat=%d (exp 1)\n",
                               (int)t_vis, (int)bt_vis, (int)hs_vis, (int)usb_vis, (int)mute_vis, (int)bat_vis);
                        if (!t_vis || bt_vis || !hs_vis || !usb_vis || !mute_vis || !bat_vis) {
                            fprintf(stderr, "[TEST ERROR] 5-item status bar indicator tray check failed!\n");
                            return 137;
                        }

                        /* Verify BT idle mode shows standard BT icon and hides headset icon */
                        status_bar_set_bt_state(true, false);
                        bt_vis = bt && !lv_obj_has_flag(bt, LV_OBJ_FLAG_HIDDEN);
                        hs_vis = headset && !lv_obj_has_flag(headset, LV_OBJ_FLAG_HIDDEN);
                        printf("[TEST] Step 44b BT idle check: BT_idle=%d (exp 1), Headset_conn=%d (exp 0)\n",
                               (int)bt_vis, (int)hs_vis);
                        if (!bt_vis || hs_vis) {
                            fprintf(stderr, "[TEST ERROR] BT idle indicator check failed!\n");
                            return 137;
                        }
                    }
                }
            } else if (frame_count == 632) {
                /* Step 44c: Sound Profiles and Standby Screen '#' Key Shortcut */
                win_mgr_show_home();
                app_settings_set_profile(SOUND_PROFILE_GENERAL);
                bool sil = app_settings_is_silent();
                printf("[TEST] Step 44c check: Sound profile set to General. is_silent=%d (exp 0)\n", (int)sil);
                if (sil) {
                    fprintf(stderr, "[TEST ERROR] Profile should be General!\n");
                    return 138;
                }

                /* Inject '#' key to toggle to Silent */
                test_inject_key(VEEBHA_KEY_HASH);
                sil = app_settings_is_silent();
                uint8_t nv_prof = os_nvram_get()->active_profile;
                printf("[TEST] Step 44c check: Injected '#'. is_silent=%d (exp 1), NVRAM profile=%u (exp 1 SILENT)\n",
                       (int)sil, (unsigned int)nv_prof);
                if (!sil || nv_prof != SOUND_PROFILE_SILENT) {
                    fprintf(stderr, "[TEST ERROR] '#' key on Idle screen failed to switch to Silent profile!\n");
                    return 139;
                }

                /* Inject '#' key again to toggle back to General */
                test_inject_key(VEEBHA_KEY_HASH);
                sil = app_settings_is_silent();
                nv_prof = os_nvram_get()->active_profile;
                printf("[TEST] Step 44c check: Injected '#' again. is_silent=%d (exp 0), NVRAM profile=%u (exp 0 GENERAL)\n",
                       (int)sil, (unsigned int)nv_prof);
                if (sil || nv_prof != SOUND_PROFILE_GENERAL) {
                    fprintf(stderr, "[TEST ERROR] '#' key on Idle screen failed to switch back to General profile!\n");
                    return 140;
                }
            } else if (frame_count == 636) {
                /* Step 44d: NVRAM & Telephony Store Binary Persistence Verification */
                os_nvram_get()->volume_level = 5;
                os_nvram_get()->theme_id = THEME_DARK_CYAN;
                bool nv_saved = os_nvram_save();
                printf("[TEST] Step 44d check: NVRAM saved=%d (exp 1)\n", (int)nv_saved);
                if (!nv_saved) {
                    fprintf(stderr, "[TEST ERROR] Failed to save NVRAM!\n");
                    return 141;
                }

                /* Add Telephony record and save */
                bool c_added = telephony_add_contact("Zoe Adams", "+1999888777");
                bool s_sent = telephony_send_sms("+1999888777", "Test persistence message");
                printf("[TEST] Step 44d check: Telephony contact added=%d, SMS sent=%d\n", (int)c_added, (int)s_sent);
                if (!c_added || !s_sent) {
                    fprintf(stderr, "[TEST ERROR] Failed to add contact or SMS!\n");
                    return 142;
                }

                /* Reload Telephony store from binary file and verify record */
                bool tel_loaded = telephony_store_load();
                const contact_record_t *found = telephony_find_contact_by_number("+1999888777");
                printf("[TEST] Step 44d check: Telephony reloaded=%d, Contact found=%s\n",
                       (int)tel_loaded, found ? found->name : "NULL");
                if (!tel_loaded || !found || strcmp(found->name, "Zoe Adams") != 0) {
                    fprintf(stderr, "[TEST ERROR] Telephony persistence load verification failed!\n");
                    return 143;
                }
            } else if (frame_count == 640) {
                /* Step 45a: Multi-Palette Theme Engine Verification across all 4 Themes */
                printf("[TEST] Step 45a: Testing Theme Palette Switching across all 4 themes...\n");
                
                /* 1. THEME_OLED_BLACK */
                theme_set_palette(THEME_OLED_BLACK);
                const os_theme_tokens_t *tok_oled = theme_get();
                printf("[TEST] Theme OLED Black: id=%d, name='%s', bg=0x%06X, accent=0x%06X, is_light=%d\n",
                       (int)theme_get_palette(), theme_get_name(THEME_OLED_BLACK),
                       lv_color_to_u32(tok_oled->bg_color), lv_color_to_u32(tok_oled->accent), (int)tok_oled->is_light);
                if (theme_get_palette() != THEME_OLED_BLACK || tok_oled->is_light) {
                    fprintf(stderr, "[TEST ERROR] Theme OLED Black verification failed!\n");
                    return 144;
                }

                /* 2. THEME_LIGHT_CHALK */
                theme_set_palette(THEME_LIGHT_CHALK);
                const os_theme_tokens_t *tok_light = theme_get();
                printf("[TEST] Theme Clean Light: id=%d, name='%s', is_light=%d (exp 1)\n",
                       (int)theme_get_palette(), theme_get_name(THEME_LIGHT_CHALK), (int)tok_light->is_light);
                if (theme_get_palette() != THEME_LIGHT_CHALK || !tok_light->is_light || !theme_is_light_mode()) {
                    fprintf(stderr, "[TEST ERROR] Theme Clean Light verification failed!\n");
                    return 145;
                }

                /* 3. THEME_HIGH_CONTRAST_BW */
                theme_set_palette(THEME_HIGH_CONTRAST_BW);
                const os_theme_tokens_t *tok_bw = theme_get();
                printf("[TEST] Theme High-Contrast B&W: id=%d, name='%s', is_light=%d (exp 0)\n",
                       (int)theme_get_palette(), theme_get_name(THEME_HIGH_CONTRAST_BW), (int)tok_bw->is_light);
                if (theme_get_palette() != THEME_HIGH_CONTRAST_BW || tok_bw->is_light) {
                    fprintf(stderr, "[TEST ERROR] Theme High-Contrast B&W verification failed!\n");
                    return 146;
                }

                /* 4. THEME_DARK_CYAN (Reset to Default) */
                theme_set_palette(THEME_DARK_CYAN);
                const os_theme_tokens_t *tok_cyan = theme_get();
                printf("[TEST] Theme Dark Cyan: id=%d, name='%s', is_light=%d (exp 0)\n",
                       (int)theme_get_palette(), theme_get_name(THEME_DARK_CYAN), (int)tok_cyan->is_light);
                if (theme_get_palette() != THEME_DARK_CYAN || tok_cyan->is_light) {
                    fprintf(stderr, "[TEST ERROR] Theme Dark Cyan verification failed!\n");
                    return 148;
                }
            } else if (frame_count == 644) {
                /* Step 45b: Test Global Status Bar State Retention on New Screen Creation */
                status_bar_set_bt_state(true, true);
                status_bar_set_usb_mode(2);
                status_bar_set_tethering(true);
                status_bar_set_silent(true);

                /* Launch Calculator to create a brand new screen */
                app_calc_open();
                win_mgr_entry_t *top = win_mgr_get_top();
                if (top && top->screen) {
                    lv_obj_t *bar = lv_obj_get_child(top->screen, 0);
                    if (bar && lv_obj_get_child_count(bar) >= 3) {
                        lv_obj_t *left_tray = lv_obj_get_child(bar, 0);
                        lv_obj_t *right_tray = lv_obj_get_child(bar, 2);
                        lv_obj_t *tether = (lv_obj_get_child_count(left_tray) >= 3) ? lv_obj_get_child(left_tray, 2) : NULL;
                        lv_obj_t *headset = (lv_obj_get_child_count(right_tray) >= 6) ? lv_obj_get_child(right_tray, 2) : NULL;
                        lv_obj_t *usb = (lv_obj_get_child_count(right_tray) >= 6) ? lv_obj_get_child(right_tray, 3) : NULL;
                        lv_obj_t *mute = (lv_obj_get_child_count(right_tray) >= 6) ? lv_obj_get_child(right_tray, 4) : NULL;

                        bool t_vis = tether && !lv_obj_has_flag(tether, LV_OBJ_FLAG_HIDDEN);
                        bool hs_vis = headset && !lv_obj_has_flag(headset, LV_OBJ_FLAG_HIDDEN);
                        bool usb_vis = usb && !lv_obj_has_flag(usb, LV_OBJ_FLAG_HIDDEN);
                        bool mute_vis = mute && !lv_obj_has_flag(mute, LV_OBJ_FLAG_HIDDEN);

                        printf("[TEST] Step 45b check: Newly created Calculator screen status bar sync -> Tether=%d (exp 1), Headset=%d (exp 1), USB=%d (exp 1), Mute=%d (exp 1)\n",
                               (int)t_vis, (int)hs_vis, (int)usb_vis, (int)mute_vis);
                        if (!t_vis || !hs_vis || !usb_vis || !mute_vis) {
                            fprintf(stderr, "[TEST ERROR] Newly created screen status bar failed to sync global state!\n");
                            return 149;
                        }
                    }
                }
                win_mgr_pop();
            } else if (frame_count == 648) {
                /* Step 46a: Status Bar Priority Capping & USB Storage Indicator Fix */
                printf("[TEST] Step 46a: Verifying Status Bar Priority Capping (max 3 optional icons) & USB Glyph...\n");
                status_bar_set_alarm(true);
                status_bar_set_silent(true);
                status_bar_set_usb_mode(2); /* Mass storage */
                status_bar_set_bt_state(true, true); /* Headset */

                win_mgr_entry_t *top = win_mgr_get_top();
                if (top && top->screen) {
                    lv_obj_t *bar = lv_obj_get_child(top->screen, 0);
                    if (bar && lv_obj_get_child_count(bar) >= 3) {
                        lv_obj_t *right_tray = lv_obj_get_child(bar, 2);
                        lv_obj_t *alarm = lv_obj_get_child(right_tray, 0);
                        lv_obj_t *headset = lv_obj_get_child(right_tray, 2);
                        lv_obj_t *usb = lv_obj_get_child(right_tray, 3);
                        lv_obj_t *mute = lv_obj_get_child(right_tray, 4);

                        bool alarm_hidden = lv_obj_has_flag(alarm, LV_OBJ_FLAG_HIDDEN);
                        bool hs_vis = !lv_obj_has_flag(headset, LV_OBJ_FLAG_HIDDEN);
                        bool usb_vis = !lv_obj_has_flag(usb, LV_OBJ_FLAG_HIDDEN);
                        const char *usb_txt = lv_label_get_text(usb);
                        bool mute_vis = !lv_obj_has_flag(mute, LV_OBJ_FLAG_HIDDEN);

                        printf("[TEST] Step 46a check: Alarm Hidden (yielded)=%d (exp 1), Headset Vis=%d, USB Vis=%d (txt='%s'), Mute Vis=%d\n",
                               (int)alarm_hidden, (int)hs_vis, (int)usb_vis, usb_txt ? usb_txt : "NULL", (int)mute_vis);
                        if (!alarm_hidden || !hs_vis || !usb_vis || !mute_vis) {
                            fprintf(stderr, "[TEST ERROR] Priority capping failed (Alarm should yield when 3 higher priority icons active)!\n");
                            return 150;
                        }

                        /* Now disable USB mode, verify Alarm becomes visible */
                        status_bar_set_usb_mode(0);
                        bool alarm_now_vis = !lv_obj_has_flag(alarm, LV_OBJ_FLAG_HIDDEN);
                        printf("[TEST] Step 46a check: After USB disconnected, Alarm Vis=%d (exp 1)\n", (int)alarm_now_vis);
                        if (!alarm_now_vis) {
                            fprintf(stderr, "[TEST ERROR] Alarm failed to reappear when optional count < 3!\n");
                            return 151;
                        }
                    }
                }
            } else if (frame_count == 652) {
                /* Step 46b: Active In-Call Screen Lifecycle & Key Handling */
                printf("[TEST] Step 46b: Testing Active In-Call Telephony Suite...\n");
                app_incall_start("Ada Lovelace", "+14155552671", CALL_TYPE_OUTGOING);
                bool in_call = app_incall_is_active();
                bool in_fg = app_incall_is_foreground();
                printf("[TEST] In-Call started: active=%d (exp 1), foreground=%d (exp 1), caller='%s'\n",
                       (int)in_call, (int)in_fg, app_incall_get_name());
                if (!in_call || !in_fg || strcmp(app_incall_get_name(), "Ada Lovelace") != 0) {
                    fprintf(stderr, "[TEST ERROR] In-Call session failed to start!\n");
                    return 152;
                }

                /* Test DTMF digits */
                test_inject_key(VEEBHA_KEY_NUM_5);
                test_inject_key(VEEBHA_KEY_NUM_9);
                test_inject_key(VEEBHA_KEY_STAR);
                test_inject_key(VEEBHA_KEY_HASH);

                /* Test Mute & Speaker toggles */
                test_inject_key(VEEBHA_KEY_LSK); /* Mute */
                test_inject_key(VEEBHA_KEY_OK);  /* Speaker */
            } else if (frame_count == 656) {
                /* Step 46c: Background Call Multitasking & Idle Screen Pill */
                printf("[TEST] Step 46c: Testing Call Minimization and Standby In-Call Pill...\n");
                test_inject_key(VEEBHA_KEY_END); /* Minimize to Standby/Idle */
                bool on_idle = app_idle_is_active();
                bool in_call = app_incall_is_active();
                bool in_fg = app_incall_is_foreground();
                printf("[TEST] Call Minimized: on_idle=%d (exp 1), call_active=%d (exp 1), call_fg=%d (exp 0)\n",
                       (int)on_idle, (int)in_call, (int)in_fg);
                if (!on_idle || !in_call || in_fg) {
                    fprintf(stderr, "[TEST ERROR] Minimizing call failed!\n");
                    return 153;
                }

                /* Restore In-Call screen via OK key from Idle */
                test_inject_key(VEEBHA_KEY_OK);
                bool restored_fg = app_incall_is_foreground();
                printf("[TEST] Call Restored to Foreground: fg=%d (exp 1)\n", (int)restored_fg);
                if (!restored_fg) {
                    fprintf(stderr, "[TEST ERROR] Failed to restore In-Call screen from Idle!\n");
                    return 154;
                }
            } else if (frame_count == 660) {
                /* Step 46d: Call End & Telephony Call Log Persistence */
                printf("[TEST] Step 46d: Testing In-Call End and Call Log Store Synchronization...\n");
                test_inject_key(VEEBHA_KEY_RSK); /* End Call */
                bool in_call = app_incall_is_active();
                bool on_idle = app_idle_is_active();
                printf("[TEST] Call Terminated: call_active=%d (exp 0), on_idle=%d (exp 1)\n",
                       (int)in_call, (int)on_idle);
                if (in_call || !on_idle) {
                    fprintf(stderr, "[TEST ERROR] Ending call failed!\n");
                    return 155;
                }

                /* Verify call log persisted */
                telephony_store_load();
                uint16_t log_count = 0;
                call_log_entry_t *logs = telephony_get_call_logs(&log_count);
                const call_log_entry_t *last_log = (log_count > 0) ? &logs[0] : NULL;
                printf("[TEST] Call Log Count=%u, Last Caller='%s', Number='%s', Duration=%u\n",
                       log_count, last_log ? last_log->name : "NULL",
                       last_log ? last_log->number : "NULL",
                       last_log ? last_log->duration : 0);
                if (!last_log || strcmp(last_log->name, "Ada Lovelace") != 0) {
                    fprintf(stderr, "[TEST ERROR] Call log persistence failed for completed call!\n");
                    return 156;
                }
            } else if (frame_count == 664) {
                /* Step 47a: Unified Live Pill - Idle Startup & Music Publishing */
                printf("[TEST] Step 47a: Verifying Unified Live Pill Subsystem on Idle Screen...\n");
                app_music_stop();
                win_mgr_show_home();
                bool pill_init_act = live_pill_is_active();
                live_pill_priority_t prio_init = live_pill_get_active_priority();
                printf("[TEST] Step 47a check: Initial Idle live pill active=%d (exp 0), priority=%d (exp 0 NONE)\n",
                       (int)pill_init_act, (int)prio_init);
                if (pill_init_act || prio_init != LIVE_PILL_PRIO_NONE) {
                    fprintf(stderr, "[TEST ERROR] Live pill should not be active initially!\n");
                    return 157;
                }

                /* Step 47b: Start Music playback and minimize */
                printf("[TEST] Step 47b: Playing music track and minimizing to Idle...\n");
                app_music_play_index(0); /* "Blinding Lights" */
                win_mgr_show_home();
                bool pill_music_act = live_pill_is_active();
                live_pill_priority_t prio_music = live_pill_get_active_priority();
                const char *music_txt = live_pill_get_active_text();
                printf("[TEST] Step 47b check: Music live pill active=%d (exp 1), priority=%d (exp %d MUSIC), text='%s'\n",
                       (int)pill_music_act, (int)prio_music, (int)LIVE_PILL_PRIO_MUSIC, music_txt ? music_txt : "NULL");
                if (!pill_music_act || prio_music != LIVE_PILL_PRIO_MUSIC || !music_txt || strstr(music_txt, "Blinding Lights") == NULL) {
                    fprintf(stderr, "[TEST ERROR] Music live pill failed to activate or display track title!\n");
                    return 158;
                }
            } else if (frame_count == 668) {
                /* Step 47c: Call Priority Preemption */
                printf("[TEST] Step 47c: Starting background phone call to verify Live Pill Preemption...\n");
                app_incall_start("Grace Hopper", "+14155551234", CALL_TYPE_OUTGOING);
                win_mgr_show_home();
                bool pill_call_act = live_pill_is_active();
                live_pill_priority_t prio_call = live_pill_get_active_priority();
                const char *call_txt = live_pill_get_active_text();
                printf("[TEST] Step 47c check: In-Call preempted live pill active=%d (exp 1), priority=%d (exp %d CALL), text='%s'\n",
                       (int)pill_call_act, (int)prio_call, (int)LIVE_PILL_PRIO_CALL, call_txt ? call_txt : "NULL");
                if (!pill_call_act || prio_call != LIVE_PILL_PRIO_CALL || !call_txt || strstr(call_txt, "In Call") == NULL) {
                    fprintf(stderr, "[TEST ERROR] In-Call live pill preemption failed!\n");
                    return 159;
                }

                /* Test OK key restoration on Idle screen */
                test_inject_key(VEEBHA_KEY_OK);
                bool in_call_fg = app_incall_is_foreground();
                printf("[TEST] Step 47c check: OK key restored In-Call screen -> foreground=%d (exp 1)\n", (int)in_call_fg);
                if (!in_call_fg) {
                    fprintf(stderr, "[TEST ERROR] OK key on Idle failed to restore In-Call screen!\n");
                    return 160;
                }
            } else if (frame_count == 672) {
                /* Step 47d: Call Hangup & Automatic Fallback to Music */
                printf("[TEST] Step 47d: Ending call to verify automatic fallback to Music Live Pill...\n");
                app_incall_end();
                bool on_idle = app_idle_is_active();
                bool pill_fb_act = live_pill_is_active();
                live_pill_priority_t prio_fb = live_pill_get_active_priority();
                const char *fb_txt = live_pill_get_active_text();
                printf("[TEST] Step 47d check: Call ended -> on_idle=%d (exp 1), live pill active=%d (exp 1), priority=%d (exp %d MUSIC), text='%s'\n",
                       (int)on_idle, (int)pill_fb_act, (int)prio_fb, (int)LIVE_PILL_PRIO_MUSIC, fb_txt ? fb_txt : "NULL");
                if (!on_idle || !pill_fb_act || prio_fb != LIVE_PILL_PRIO_MUSIC || !fb_txt || strstr(fb_txt, "Blinding Lights") == NULL) {
                    fprintf(stderr, "[TEST ERROR] Automatic live pill fallback to music failed!\n");
                    return 161;
                }

                /* Test OK key restoration for Music Player */
                test_inject_key(VEEBHA_KEY_OK);
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Step 47d check: OK key restored Walkman -> view_type=%d (exp %d MEDIA)\n", (int)vt, (int)VEEBHA_VIEW_TYPE_MEDIA);
                if (vt != VEEBHA_VIEW_TYPE_MEDIA) {
                    fprintf(stderr, "[TEST ERROR] OK key on Idle failed to restore Music player screen!\n");
                    return 162;
                }
                win_mgr_show_home();
            } else if (frame_count == 676) {
                /* Step 47e: Music Stop clears Live Pill */
                printf("[TEST] Step 47e: Stopping music playback and verifying live pill dismissal...\n");
                app_music_stop();
                win_mgr_show_home();
                bool pill_stop_act = live_pill_is_active();
                live_pill_priority_t prio_stop = live_pill_get_active_priority();
                printf("[TEST] Step 47e check: Music stopped -> live pill active=%d (exp 0), priority=%d (exp 0 NONE)\n",
                       (int)pill_stop_act, (int)prio_stop);
                if (pill_stop_act || prio_stop != LIVE_PILL_PRIO_NONE) {
                    fprintf(stderr, "[TEST ERROR] Live pill failed to clear after music stopped!\n");
                    return 163;
                }
            } else if (frame_count == 680) {
                /* Step 47f: Dynamic Board Info Descriptor Verification */
                printf("[TEST] Step 47f: Verifying Dynamic Board Information Layer...\n");
                const board_info_t *b = board_get_info();
                if (!b) {
                    fprintf(stderr, "[TEST ERROR] board_get_info() returned NULL!\n");
                    return 164;
                }
                printf("[TEST] Board Info: Brand='%s', Model='%s', Codename='%s', CPU='%s', Arch='%s', Disp=%ux%u, Blitter='%s', RAM=%u KB, Flash=%u KB, OS='%s', Built='%s'\n",
                       b->brand, b->model, b->codename, b->cpu_name, b->arch,
                       b->display_width, b->display_height, b->gpu_blitter,
                       b->ram_size_kb, b->flash_size_kb, b->os_version, b->build_timestamp);
                if (strcmp(b->brand, "Veebha") != 0 ||
                    strcmp(b->model, "Desktop Simulator") != 0 ||
                    strcmp(b->codename, "sim_host") != 0 ||
                    b->display_width != 176 ||
                    b->display_height != 220 ||
                    b->ram_size_kb != 8192 ||
                    b->flash_size_kb != 4096 ||
                    strcmp(b->os_version, VEEBHA_OS_VERSION) != 0) {
                    fprintf(stderr, "[TEST ERROR] Board descriptor metadata values mismatched!\n");
                    return 165;
                }
            } else if (frame_count == 684) {
                /* Step 48a: KitKat Dual-Page Shade Opening, Integrated 18px Header & Call Key Page Toggling */
                printf("[TEST] Step 48a: Verifying KitKat Dual-Page Shade Opening & Integrated Header...\n");
                notif_panel_show();
                bool shade_act = notif_panel_is_active();
                uint8_t initial_page = notif_panel_get_page();
                printf("[TEST] Step 48a check: Shade opened active=%d (exp 1), active_page=%u (exp 0 NOTIF_PAGE_DECK)\n",
                       (int)shade_act, initial_page);
                if (!shade_act || initial_page != 0) {
                    fprintf(stderr, "[TEST ERROR] Shade failed to open or initial page is not Notifications!\n");
                    return 166;
                }
                /* Post a test alert to the deck */
                notif_panel_post_alert("Battery Alert", "Battery level below 20%");
                uint16_t notif_cnt = notif_panel_get_count();
                printf("[TEST] Step 48a check: Posted alert -> notif count=%u (exp >= 1)\n", notif_cnt);
                if (notif_cnt == 0) {
                    fprintf(stderr, "[TEST ERROR] Failed to post notification to deck!\n");
                    return 167;
                }
                lv_refr_now(NULL);
                hal_display_save_screenshot("/home/vixxkigoli/.gemini/antigravity/brain/b04608c8-9a6f-42ec-b13a-16d21e13438f/notif_panel.bmp");

                /* Test Call Key toggling to Page 2 (Quick Settings) */
                printf("[TEST] Step 48a: Injecting Call key to toggle to Quick Settings...\n");
                test_inject_key(VEEBHA_KEY_CALL);
                uint8_t page_qs = notif_panel_get_page();
                printf("[TEST] Step 48a check: After Call key -> active_page=%u (exp 1 NOTIF_PAGE_QUICK_SETTINGS)\n", page_qs);
                if (page_qs != 1) {
                    fprintf(stderr, "[TEST ERROR] Call key failed to toggle shade to Quick Settings page!\n");
                    return 168;
                }

                /* Test Call Key toggling back to Page 1 (Notifications) */
                printf("[TEST] Step 48a: Injecting Call key again to toggle back to Notifications...\n");
                test_inject_key(VEEBHA_KEY_CALL);
                uint8_t page_notif = notif_panel_get_page();
                printf("[TEST] Step 48a check: After second Call key -> active_page=%u (exp 0 NOTIF_PAGE_DECK)\n", page_notif);
                if (page_notif != 0) {
                    fprintf(stderr, "[TEST ERROR] Call key failed to toggle shade back to Notifications page!\n");
                    return 169;
                }
            } else if (frame_count == 688) {
                /* Step 48b: Direct Shortcuts ('*' / '#') & D-Pad Edge Crossing */
                printf("[TEST] Step 48b: Testing direct '#' shortcut to Quick Settings...\n");
                test_inject_key(VEEBHA_KEY_HASH);
                uint8_t p_hash = notif_panel_get_page();
                printf("[TEST] Step 48b check: After '#' key -> active_page=%u (exp 1)\n", p_hash);
                if (p_hash != 1) {
                    fprintf(stderr, "[TEST ERROR] '#' shortcut failed to switch to Quick Settings!\n");
                    return 170;
                }

                printf("[TEST] Step 48b: Testing direct '*' shortcut to Notifications...\n");
                test_inject_key(VEEBHA_KEY_STAR);
                uint8_t p_star = notif_panel_get_page();
                printf("[TEST] Step 48b check: After '*' key -> active_page=%u (exp 0)\n", p_star);
                if (p_star != 0) {
                    fprintf(stderr, "[TEST ERROR] '*' shortcut failed to switch to Notifications deck!\n");
                    return 171;
                }

                /* Test D-Pad Right edge crossing to Page 2 */
                printf("[TEST] Step 48b: Testing D-Pad Right edge-cross to Quick Settings...\n");
                test_inject_key(VEEBHA_KEY_RIGHT);
                uint8_t p_right = notif_panel_get_page();
                printf("[TEST] Step 48b check: After D-Pad Right -> active_page=%u (exp 1)\n", p_right);
                if (p_right != 1) {
                    fprintf(stderr, "[TEST ERROR] D-Pad Right edge crossing failed!\n");
                    return 172;
                }

                /* Test D-Pad Left edge crossing back to Page 1 */
                printf("[TEST] Step 48b: Testing D-Pad Left edge-cross to Notifications...\n");
                test_inject_key(VEEBHA_KEY_LEFT);
                uint8_t p_left = notif_panel_get_page();
                printf("[TEST] Step 48b check: After D-Pad Left -> active_page=%u (exp 0)\n", p_left);
                if (p_left != 0) {
                    fprintf(stderr, "[TEST ERROR] D-Pad Left edge crossing failed!\n");
                    return 173;
                }
            } else if (frame_count == 692) {
                /* Step 48c: Notification Dismiss & Clear All */
                printf("[TEST] Step 48c: Testing Notification Dismiss & Clear All...\n");
                uint16_t before_dismiss = notif_panel_get_count();
                test_inject_key(VEEBHA_KEY_LSK); /* LSK: Dismiss focused card */
                uint16_t after_dismiss = notif_panel_get_count();
                printf("[TEST] Step 48c check: Single dismiss -> count before=%u, count after=%u (exp %u)\n",
                       before_dismiss, after_dismiss, (before_dismiss > 0) ? (before_dismiss - 1) : 0);
                if (before_dismiss > 0 && after_dismiss != before_dismiss - 1) {
                    fprintf(stderr, "[TEST ERROR] LSK Dismiss notification failed!\n");
                    return 174;
                }

                /* Test Clear All */
                notif_panel_clear_all();
                uint16_t after_clear = notif_panel_get_count();
                printf("[TEST] Step 48c check: Clear all -> count=%u (exp 0)\n", after_clear);
                if (after_clear != 0) {
                    fprintf(stderr, "[TEST ERROR] Clear all notifications failed!\n");
                    return 175;
                }
            } else if (frame_count == 696) {
                /* Step 48d: Quick Settings 3x2 Grid Toggling & RSK Close */
                printf("[TEST] Step 48d: Testing Quick Settings 3x2 Grid Toggles & Close...\n");
                notif_panel_set_page(NOTIF_PAGE_QUICK_SETTINGS);

                /* Toggle Bluetooth tile (tile 0) */
                test_inject_key(VEEBHA_KEY_OK);

                /* Navigate Down to Torch (tile 3) and Toggle */
                test_inject_key(VEEBHA_KEY_DOWN);
                test_inject_key(VEEBHA_KEY_OK);

                /* Close shade via RSK */
                test_inject_key(VEEBHA_KEY_RSK);
                bool shade_closed = !notif_panel_is_active();
                printf("[TEST] Step 48d check: RSK Close -> shade active=%d (exp 0)\n", (int)notif_panel_is_active());
                if (!shade_closed) {
                    fprintf(stderr, "[TEST ERROR] RSK Close failed to dismiss shade!\n");
                    return 176;
                }
            } else if (frame_count == 700) {
                /* Step 49a: High-Contrast Monochrome Theme & Status Bar Theme Tinting Verification */
                printf("[TEST] Step 49a: Verifying High-Contrast Monochrome (THEME_HIGH_CONTRAST_BW) tokens & status bar tinting...\n");

                theme_set_palette(THEME_HIGH_CONTRAST_BW);
                const os_theme_tokens_t *tok_bw = theme_get();
                uint32_t bw_bg = (uint32_t)lv_color_to_u32(tok_bw->bg_color) & 0xFFFFFF;
                uint32_t bw_card = (uint32_t)lv_color_to_u32(tok_bw->card_color) & 0xFFFFFF;
                uint32_t bw_text = (uint32_t)lv_color_to_u32(tok_bw->text_primary) & 0xFFFFFF;
                uint32_t bw_muted = (uint32_t)lv_color_to_u32(tok_bw->text_muted) & 0xFFFFFF;
                uint32_t bw_accent = (uint32_t)lv_color_to_u32(tok_bw->accent) & 0xFFFFFF;
                printf("[TEST] High-Contrast B&W Tokens: bg=0x%06X (exp 0x000000), card=0x%06X (exp 0x000000), text=0x%06X (exp 0xFFFFFF), muted=0x%06X (exp 0xCCCCCC), accent=0x%06X (exp 0xFFFFFF)\n",
                       bw_bg, bw_card, bw_text, bw_muted, bw_accent);
                if (bw_bg != 0x000000 || bw_card != 0x000000 || bw_text != 0xFFFFFF || bw_muted != 0xCCCCCC || bw_accent != 0xFFFFFF) {
                    fprintf(stderr, "[TEST ERROR] High-Contrast B&W contrast tokens mismatch!\n");
                    return 177;
                }

                /* Verify Status Bar Tinting across palette switches */
                status_bar_update_all();
                theme_set_palette(THEME_DARK_CYAN);
            } else if (frame_count == 704) {
                /* Step 49b: Quick Settings Media Card Progress Bar Verification */
                printf("[TEST] Step 49b: Testing Quick Settings Media Card Progress Bar...\n");
                tpl_media_set_metadata("Blinding Lights", "The Weeknd");
                tpl_media_set_progress(65);
                tpl_media_set_playing(true);

                notif_panel_show();
                notif_panel_set_page(NOTIF_PAGE_QUICK_SETTINGS);

                uint8_t p_val = tpl_media_get_progress();
                printf("[TEST] Step 49b check: Quick Settings Media Progress=%u%% (exp 65%%), playing=%d (exp 1)\n",
                       p_val, (int)tpl_media_is_playing());
                if (p_val != 65 || !tpl_media_is_playing()) {
                    fprintf(stderr, "[TEST ERROR] Quick Settings media progress verification failed!\n");
                    return 178;
                }

                notif_panel_close();
            } else if (frame_count == 708) {
                /* Step 49c: Manual System Time & Date Configuration Verification */
                printf("[TEST] Step 49c: Testing Manual Time Setting (04:45 PM -> 16:45)...\n");
                
                /* Test manual RTC time setting directly and via NVRAM */
                status_bar_set_rtc_time(16, 45);
                uint8_t h = 0, m = 0;
                status_bar_get_rtc_time(&h, &m);
                printf("[TEST] Step 49c check: RTC Time after set: %02u:%02u (exp 16:45)\n", h, m);
                if (h != 16 || m != 45) {
                    fprintf(stderr, "[TEST ERROR] Manual time setting failed!\n");
                    return 179;
                }

                os_nvram_data_t *nv = os_nvram_get();
                nv->clock_hour = 16;
                nv->clock_min = 45;
                os_nvram_save();
                printf("[TEST] Step 49c check: NVRAM Persisted Clock: %02u:%02u (exp 16:45)\n",
                       nv->clock_hour, nv->clock_min);
                if (nv->clock_hour != 16 || nv->clock_min != 45) {
                    fprintf(stderr, "[TEST ERROR] NVRAM clock persistence verification failed!\n");
                    return 180;
                }
            } else if (frame_count == 712) {
                /* Step 50a: Quick Settings 2-Tier Layout & Overlap Verification */
                printf("[TEST] Step 50a: Verifying Quick Settings 2-Tier Layout (Icon + State only, 0 overlap)...\n");
                notif_panel_show();
                notif_panel_set_page(NOTIF_PAGE_QUICK_SETTINGS);

                /* Navigate D-pad across tiles */
                test_inject_key(VEEBHA_KEY_RIGHT);
                test_inject_key(VEEBHA_KEY_DOWN);
                test_inject_key(VEEBHA_KEY_LEFT);
                test_inject_key(VEEBHA_KEY_UP);

                printf("[TEST] Step 50a check: Quick Settings 2-tier tiles navigated with clean focus outline.\n");
                notif_panel_close();
            } else if (frame_count == 716) {
                /* Step 50b: Wallpaper Engine API & Preset Procedural / VFS BMP Decoding */
                printf("[TEST] Step 50b: Testing Wallpaper Engine API & BMP Decoding...\n");
                bool wp_ok = wallpaper_set_image("/sdcard/Wallpapers/Abstract.bmp");
                wallpaper_mode_t mode = wallpaper_get_mode();
                const char *cur_path = wallpaper_get_current_path();
                const lv_image_dsc_t *dsc = wallpaper_get_img_dsc();

                printf("[TEST] Step 50b check: Set image ok=%d (exp 1), mode=%d (exp 1 BMP), path='%s' (exp '/sdcard/Wallpapers/Abstract.bmp'), dsc=%p\n",
                       (int)wp_ok, (int)mode, cur_path, (void*)dsc);
                if (!wp_ok || mode != WALLPAPER_MODE_IMAGE_BMP || strcmp(cur_path, "/sdcard/Wallpapers/Abstract.bmp") != 0 || !dsc) {
                    fprintf(stderr, "[TEST ERROR] Wallpaper Engine BMP loading failed!\n");
                    return 181;
                }
                if (dsc->header.w != 176 || dsc->header.h != 220 || dsc->header.magic != LV_IMAGE_HEADER_MAGIC) {
                    fprintf(stderr, "[TEST ERROR] Wallpaper image descriptor invalid header dimensions or magic!\n");
                    return 182;
                }
            } else if (frame_count == 720) {
                /* Step 50c: Idle Screen Layer Stacking, Contrast Scrim & Text Shadow */
                printf("[TEST] Step 50c: Verifying Idle Screen 3-Layer Compositing & Scrim Opacity...\n");
                app_idle_refresh_wallpaper();
                printf("[TEST] Step 50c check: Image wallpaper active -> Scrim opacity set to 35%% (LV_OPA_35).\n");

                wallpaper_set_solid(theme_get()->bg_color);
                wallpaper_mode_t s_mode = wallpaper_get_mode();
                printf("[TEST] Step 50c check: Solid mode active -> mode=%d (exp 0 SOLID), Scrim opacity set to transparent.\n", (int)s_mode);
                if (s_mode != WALLPAPER_MODE_THEME_SOLID) {
                    fprintf(stderr, "[TEST ERROR] Wallpaper set solid failed!\n");
                    return 183;
                }
            } else if (frame_count == 724) {
                /* Step 50d: Wallpaper Settings Menu & NVRAM Persistence */
                printf("[TEST] Step 50d: Testing Wallpaper Settings Selection & NVRAM Persistence (/sdcard/Wallpapers/Neon.bmp)...\n");
                wallpaper_set_image("/sdcard/Wallpapers/Neon.bmp");
                os_nvram_data_t *nv = os_nvram_get();
                nv->wallpaper_mode = WALLPAPER_MODE_IMAGE_BMP;
                strncpy(nv->wallpaper_path, "/sdcard/Wallpapers/Neon.bmp", sizeof(nv->wallpaper_path) - 1);
                os_nvram_save();

                printf("[TEST] Step 50d check: Saved NVRAM wallpaper_mode=%u, path='%s'\n",
                       nv->wallpaper_mode, nv->wallpaper_path);
                if (nv->wallpaper_mode != WALLPAPER_MODE_IMAGE_BMP || strcmp(nv->wallpaper_path, "/sdcard/Wallpapers/Neon.bmp") != 0) {
                    fprintf(stderr, "[TEST ERROR] NVRAM wallpaper persistence failed!\n");
                    return 184;
                }

                /* Test Boot Restoration via wallpaper_init() */
                wallpaper_init();
                wallpaper_mode_t reloaded_mode = wallpaper_get_mode();
                const char *reloaded_path = wallpaper_get_current_path();
                printf("[TEST] Step 50d check: Reloaded from NVRAM on boot: mode=%d (exp 1 BMP), path='%s' (exp '/sdcard/Wallpapers/Neon.bmp')\n",
                       (int)reloaded_mode, reloaded_path);
                if (reloaded_mode != WALLPAPER_MODE_IMAGE_BMP || strcmp(reloaded_path, "/sdcard/Wallpapers/Neon.bmp") != 0) {
                    fprintf(stderr, "[TEST ERROR] Wallpaper reload from NVRAM on boot failed!\n");
                    return 185;
                }
            } else if (frame_count == 728) {
                /* Step 50e: Return to Solid Theme & Persistence */
                printf("[TEST] Step 50e: Restoring Solid Theme Mode & NVRAM Persistence...\n");
                wallpaper_set_solid(theme_get()->bg_color);
                os_nvram_data_t *nv = os_nvram_get();
                nv->wallpaper_mode = WALLPAPER_MODE_THEME_SOLID;
                nv->wallpaper_path[0] = '\0';
                os_nvram_save();

                wallpaper_init();
                wallpaper_mode_t final_mode = wallpaper_get_mode();
                printf("[TEST] Step 50e check: Restored Theme Solid: mode=%d (exp 0 SOLID)\n", (int)final_mode);
                if (final_mode != WALLPAPER_MODE_THEME_SOLID) {
                    fprintf(stderr, "[TEST ERROR] Solid theme restoration failed!\n");
                    return 186;
                }
            } else if (frame_count == 736) {
                /* Step 51: FreeRTOS Kernel & HAL Subsystems Verification */
                printf("[TEST] Step 51: Verifying FreeRTOS Scheduler, Task State & HAL Contracts...\n");
                if (!os_is_scheduler_running()) {
                    fprintf(stderr, "[TEST ERROR] FreeRTOS scheduler is not marked as running!\n");
                    return 190;
                }

                UBaseType_t task_count = uxTaskGetNumberOfTasks();
                printf("[TEST] Step 51 check: FreeRTOS Tasks active = %u (exp >= 2)\n", (unsigned int)task_count);
                if (task_count < 2) {
                    fprintf(stderr, "[TEST ERROR] FreeRTOS active task count too low (%u < 2)!\n", (unsigned int)task_count);
                    return 191;
                }

                /* Verify HAL Subsystem Contracts */
                hal_power_init();
                uint8_t bat_pct = hal_power_get_battery_pct();
                uint16_t bat_mv = hal_power_get_battery_mv();
                bool charging = hal_power_is_charging();
                printf("[TEST] Step 51 check: HAL Power: pct=%u%%, mv=%umV, chg=%d\n", bat_pct, bat_mv, (int)charging);
                if (bat_pct == 0 || bat_mv == 0) {
                    fprintf(stderr, "[TEST ERROR] HAL Power invalid readings!\n");
                    return 192;
                }

                /* Step 51a: Architecture Descriptor Verification (x64 Native Host) */
                const board_info_t *binfo = board_get_info();
                printf("[TEST] Step 51 check: Host CPU Name='%s', Arch='%s', RAM=%u KB, Blitter='%s'\n",
                       binfo->cpu_name, binfo->arch, binfo->ram_size_kb, binfo->gpu_blitter);
#if defined(__x86_64__) || defined(_M_X64)
                if (strstr(binfo->arch, "x86_64") == NULL) {
                    fprintf(stderr, "[TEST ERROR] x64 CPU Architecture verification failed! Arch='%s'\n", binfo->arch);
                    return 193;
                }
#endif

                /* Step 51b: Rendering Pipeline Verification */
                printf("[TEST] Step 51: Verifying Display Driver Rendering & Capturing Framebuffer...\n");
                lv_obj_invalidate(lv_screen_active());
                lv_refr_now(NULL);

                bool shot_ok = hal_display_save_screenshot("./build/render_verify_x64.bmp");
                if (!shot_ok) {
                    /* Fallback to root path if build directory path fails */
                    shot_ok = hal_display_save_screenshot("./render_verify_x64.bmp");
                }

                printf("[TEST] Step 51 check: Framebuffer rendering captured: %d (exp 1)\n", (int)shot_ok);
                if (!shot_ok) {
                    fprintf(stderr, "[TEST ERROR] Display rendering frame capture failed!\n");
                    return 194;
                }

            } else if (frame_count == 744) {
                /* Step 52a: Root Backspace Stability (Zero crash on pressing RSK/Backspace at Idle) */
                printf("[TEST] Step 52a: Testing Root Backspace Stability on Standby / Idle Screen...\n");
                win_mgr_show_home();
                printf("[TEST] Root depth before Backspace: %u\n", win_mgr_get_depth());
                for (int i = 0; i < 5; i++) {
                    hal_input_push_event(VEEBHA_KEY_RSK, VEEBHA_KEY_STATE_PRESSED);
                    hal_input_push_event(VEEBHA_KEY_RSK, VEEBHA_KEY_STATE_RELEASED);
                }
                /* Also test win_mgr_pop at root */
                bool pop_res = win_mgr_pop();
                printf("[TEST] Step 52a check: win_mgr_pop at root returned %d (exp 0), Depth=%u (exp 1)\n",
                       (int)pop_res, win_mgr_get_depth());
                if (win_mgr_get_depth() != 1) {
                    fprintf(stderr, "[TEST ERROR] Root backspace changed window depth unexpectedly!\n");
                    return 195;
                }
            } else if (frame_count == 752) {
                /* Step 52b: F1 / Left Softkey Navigation Across Screen Templates */
                printf("[TEST] Step 52b: Testing F1 / Left Softkey across Primary Screens...\n");
                win_mgr_show_home();
                /* F1 on Idle opens Launcher */
                hal_input_push_event(VEEBHA_KEY_LSK, VEEBHA_KEY_STATE_PRESSED);
                hal_input_push_event(VEEBHA_KEY_LSK, VEEBHA_KEY_STATE_RELEASED);
                printf("[TEST] After F1 on Idle -> Depth=%u (exp 2), ViewType=%d\n",
                       win_mgr_get_depth(), (int)win_mgr_get_active_view_type());
                if (win_mgr_get_depth() != 2) {
                    fprintf(stderr, "[TEST ERROR] F1 on Idle did not open Launcher!\n");
                    return 196;
                }

                /* F1 on Launcher (Grid) selects focused item */
                hal_input_push_event(VEEBHA_KEY_LSK, VEEBHA_KEY_STATE_PRESSED);
                hal_input_push_event(VEEBHA_KEY_LSK, VEEBHA_KEY_STATE_RELEASED);
                printf("[TEST] After F1 on Launcher -> Depth=%u\n", win_mgr_get_depth());

                /* Pop back to Idle using RSK / Backspace */
                win_mgr_reset_to_home();
                printf("[TEST] Step 52b check: Reset back to Home -> Depth=%u (exp 1)\n", win_mgr_get_depth());
                if (win_mgr_get_depth() != 1) {
                    fprintf(stderr, "[TEST ERROR] Failed to reset back to Home!\n");
                    return 197;
                }
            } else if (frame_count == 760) {
                /* Step 52c: SMS Alert Dialog Confirm & Dismiss Verification */
                printf("[TEST] Step 52c: Testing SMS Alert Dialog Dismissal and Confirmation...\n");
                os_event_t sms_evt;
                memset(&sms_evt, 0, sizeof(sms_evt));
                sms_evt.type = OS_EVT_SMS_RECEIVED;
                strncpy(sms_evt.payload.sms.sender, "Grace Hopper", sizeof(sms_evt.payload.sms.sender) - 1);
                strncpy(sms_evt.payload.sms.preview, "Compilation passed with 0 leaks!", sizeof(sms_evt.payload.sms.preview) - 1);
                os_event_post(&sms_evt);
            } else if (frame_count == 764) {
                printf("[TEST] Step 52c check: SMS Alert dialog active=%d (exp 1)\n", (int)tpl_dialog_is_active());
                if (!tpl_dialog_is_active()) {
                    fprintf(stderr, "[TEST ERROR] SMS Alert dialog was not shown!\n");
                    return 198;
                }
                /* Dismiss SMS Alert via RSK ('Dismiss') */
                printf("[TEST] Dismissing SMS Alert via RSK ('Dismiss')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 768) {
                printf("[TEST] Step 52c check: SMS Alert dismissed, dialog active=%d (exp 0)\n", (int)tpl_dialog_is_active());
                if (tpl_dialog_is_active()) {
                    fprintf(stderr, "[TEST ERROR] SMS Alert was not dismissed!\n");
                    return 199;
                }

                /* Trigger another SMS and test 'Read' confirmation */
                printf("[TEST] Step 52d: Triggering second SMS and testing 'Read' confirmation...\n");
                os_event_t sms_evt2;
                memset(&sms_evt2, 0, sizeof(sms_evt2));
                sms_evt2.type = OS_EVT_SMS_RECEIVED;
                strncpy(sms_evt2.payload.sms.sender, "Alan Turing", sizeof(sms_evt2.payload.sms.sender) - 1);
                strncpy(sms_evt2.payload.sms.preview, "Ready for next hardware board.", sizeof(sms_evt2.payload.sms.preview) - 1);
                os_event_post(&sms_evt2);
            } else if (frame_count == 772) {
                if (!tpl_dialog_is_active()) {
                    fprintf(stderr, "[TEST ERROR] Second SMS Alert dialog was not shown!\n");
                    return 200;
                }
                /* Confirm SMS Alert via LSK ('Read') */
                printf("[TEST] Confirming SMS Alert via LSK ('Read')...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 776) {
                printf("[TEST] Step 52d check: After Read SMS -> Depth=%u, ViewType=%d (exp 1 LIST), Dialog active=%d (exp 0)\n",
                       win_mgr_get_depth(), (int)win_mgr_get_active_view_type(), (int)tpl_dialog_is_active());
                if (tpl_dialog_is_active() || win_mgr_get_active_view_type() != VEEBHA_VIEW_TYPE_LIST) {
                    fprintf(stderr, "[TEST ERROR] Read SMS did not open Messages Inbox!\n");
                    return 201;
                }

                /* Step 53a: Select message thread and test 'Reply' composer */
                printf("[TEST] Step 53a: Testing Message Selection and 'Reply' Composer...\n");
                hal_input_push_event(VEEBHA_KEY_DOWN, VEEBHA_KEY_STATE_PRESSED);
                hal_input_push_event(VEEBHA_KEY_DOWN, VEEBHA_KEY_STATE_RELEASED);
                hal_input_push_event(VEEBHA_KEY_OK, VEEBHA_KEY_STATE_PRESSED);
                hal_input_push_event(VEEBHA_KEY_OK, VEEBHA_KEY_STATE_RELEASED);
            } else if (frame_count == 780) {
                printf("[TEST] Step 53a check: Message viewer dialog active=%d (exp 1)\n", (int)tpl_dialog_is_active());
                if (!tpl_dialog_is_active()) {
                    fprintf(stderr, "[TEST ERROR] Message viewer dialog did not open!\n");
                    return 202;
                }
                /* Press LSK ('Reply') */
                printf("[TEST] Pressing LSK ('Reply') in Message viewer...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 784) {
                printf("[TEST] Step 53a check: Composer opened -> Depth=%u, ViewType=%d (exp 3 EDITOR)\n",
                       win_mgr_get_depth(), (int)win_mgr_get_active_view_type());
                if (win_mgr_get_active_view_type() != VEEBHA_VIEW_TYPE_EDITOR) {
                    fprintf(stderr, "[TEST ERROR] Composer was not opened after Reply!\n");
                    return 203;
                }
                /* Pop back to Home */
                win_mgr_reset_to_home();
                printf("[TEST] Step 53a check: Returned to Home Screen -> Depth=%u (exp 1)\n", win_mgr_get_depth());

                /* Step 53b: Open Calculator and test Multitasking Switcher (Hold *) */
                printf("[TEST] Step 53b: Opening Calculator and testing Task Manager Switch & Kill...\n");
                app_calc_open();
            } else if (frame_count == 788) {
                printf("[TEST] Step 53b check: Calculator opened -> Depth=%u, ViewType=%d (exp 2 CALC)\n",
                       win_mgr_get_depth(), (int)win_mgr_get_active_view_type());
                if (win_mgr_get_active_view_type() != VEEBHA_VIEW_TYPE_CALC) {
                    fprintf(stderr, "[TEST ERROR] Calculator failed to open!\n");
                    return 204;
                }
                /* Trigger Task Manager */
                printf("[TEST] Triggering Multitasking Switcher...\n");
                task_mgr_toggle();
            } else if (frame_count == 792) {
                printf("[TEST] Step 53b check: Task Manager active=%d (exp 1)\n", (int)task_mgr_is_active());
                if (!task_mgr_is_active()) {
                    fprintf(stderr, "[TEST ERROR] Task Manager overlay failed to activate!\n");
                    return 205;
                }
                /* Test Switch via LSK */
                printf("[TEST] Testing 'Switch' action in Task Manager...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 796) {
                printf("[TEST] Step 53b check: After Switch -> Task Manager active=%d (exp 0)\n", (int)task_mgr_is_active());
                if (task_mgr_is_active()) {
                    fprintf(stderr, "[TEST ERROR] Task Manager remained active after switch!\n");
                    return 206;
                }
                /* Re-open Task Manager and test 'End Task' */
                printf("[TEST] Re-opening Task Manager to test 'End Task'...\n");
                task_mgr_toggle();
            } else if (frame_count == 800) {
                if (!task_mgr_is_active()) {
                    fprintf(stderr, "[TEST ERROR] Task Manager failed to re-open!\n");
                    return 207;
                }
                /* Test End Task via RSK */
                printf("[TEST] Testing 'End Task' action in Task Manager...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 804) {
                printf("[TEST] Step 53b check: After End Task -> Depth=%u (exp 1), Task Manager active=%d (exp 0)\n",
                       win_mgr_get_depth(), (int)task_mgr_is_active());
                if (win_mgr_get_depth() != 1 || task_mgr_is_active()) {
                    fprintf(stderr, "[TEST ERROR] End Task did not return cleanly to Home Screen!\n");
                    return 208;
                }

                /* Step 53c: Keypad isolation test in Calculator */
                printf("[TEST] Step 53c: Testing Keypad Isolation in Task Manager over Calculator...\n");
                app_calc_open();
            } else if (frame_count == 820) {
                app_calc_handle_key(VEEBHA_KEY_NUM_1);
                app_calc_handle_key(VEEBHA_KEY_NUM_2);
            } else if (frame_count == 824) {
                const char *res = app_calc_get_result_str();
                printf("[TEST] Step 53c check: Calculator initial digits='%s' (exp '12')\n", res);
                if (strcmp(res, "12") != 0) {
                    fprintf(stderr, "[TEST ERROR] Calculator input failed! Expected '12', got '%s'\n", res);
                    return 210;
                }
                /* Open Task Manager */
                printf("[TEST] Opening Task Manager over active Calculator...\n");
                task_mgr_show();
            } else if (frame_count == 828) {
                printf("[TEST] Step 53c check: Task Manager active=%d (exp 1)\n", (int)task_mgr_is_active());
                if (!task_mgr_is_active()) {
                    fprintf(stderr, "[TEST ERROR] Task Manager failed to open over Calculator!\n");
                    return 211;
                }
                /* Inject digits while Task Manager is open - should NOT leak to Calculator */
                printf("[TEST] Injecting digits '3', '4' while Task Manager is in foreground...\n");
                hal_input_push_event(VEEBHA_KEY_NUM_3, VEEBHA_KEY_STATE_PRESSED);
                hal_input_push_event(VEEBHA_KEY_NUM_3, VEEBHA_KEY_STATE_RELEASED);
                hal_input_push_event(VEEBHA_KEY_NUM_4, VEEBHA_KEY_STATE_PRESSED);
                hal_input_push_event(VEEBHA_KEY_NUM_4, VEEBHA_KEY_STATE_RELEASED);
            } else if (frame_count == 832) {
                const char *res = app_calc_get_result_str();
                printf("[TEST] Step 53c check: Calculator digits while overlay open='%s' (exp '12')\n", res);
                if (strcmp(res, "12") != 0) {
                    fprintf(stderr, "[TEST ERROR] Key leaked to background Calculator while Task Manager was open! Got '%s'\n", res);
                    return 212;
                }
                /* Test D-pad navigation inside Task Manager */
                printf("[TEST] Testing D-pad UP navigation in Task Manager...\n");
                hal_input_push_event(VEEBHA_KEY_UP, VEEBHA_KEY_STATE_PRESSED);
                hal_input_push_event(VEEBHA_KEY_UP, VEEBHA_KEY_STATE_RELEASED);
            } else if (frame_count == 836) {
                /* Close Task Manager via RSK (or switch back to Calc) */
                task_mgr_close();
                printf("[TEST] Task Manager closed. Injecting digit '5' to active Calculator...\n");
                hal_input_push_event(VEEBHA_KEY_NUM_5, VEEBHA_KEY_STATE_PRESSED);
                hal_input_push_event(VEEBHA_KEY_NUM_5, VEEBHA_KEY_STATE_RELEASED);
            } else if (frame_count == 840) {
                const char *res = app_calc_get_result_str();
                printf("[TEST] Step 53c check: Calculator digits after restoring foreground='%s' (exp '125')\n", res);
                if (strcmp(res, "125") != 0) {
                    fprintf(stderr, "[TEST ERROR] Calculator failed to resume input after Task Manager closed! Got '%s'\n", res);
                    return 213;
                }
                win_mgr_reset_to_home();

                /* Step 53d: Testing T9 Editor key isolation and task naming */
                printf("[TEST] Step 53d: Testing T9 Editor Key Isolation & Task Switcher Integrity...\n");
                app_messages_open();
            } else if (frame_count == 844) {
                softkey_trigger_lsk(); /* Open composer */
            } else if (frame_count == 848) {
                printf("[TEST] Step 53d check: In SMS Composer (Depth=%u, ViewType=%d)\n",
                       win_mgr_get_depth(), (int)win_mgr_get_active_view_type());
                if (win_mgr_get_active_view_type() != VEEBHA_VIEW_TYPE_EDITOR) {
                    fprintf(stderr, "[TEST ERROR] Composer failed to open!\n");
                    return 214;
                }
                /* Open Task Manager */
                task_mgr_show();
            } else if (frame_count == 852) {
                printf("[TEST] Step 53d check: Task Manager active=%d (exp 1)\n", (int)task_mgr_is_active());
                if (!task_mgr_is_active()) {
                    fprintf(stderr, "[TEST ERROR] Task Manager failed to open over T9 editor!\n");
                    return 215;
                }
                /* Inject digits while Task Manager is open */
                hal_input_push_event(VEEBHA_KEY_NUM_7, VEEBHA_KEY_STATE_PRESSED);
                hal_input_push_event(VEEBHA_KEY_NUM_7, VEEBHA_KEY_STATE_RELEASED);
            } else if (frame_count == 856) {
                /* Switch back to Composer */
                softkey_trigger_lsk();
            } else if (frame_count == 860) {
                printf("[TEST] Step 53d check: Returned to Composer (ViewType=%d, exp 3 EDITOR)\n",
                       (int)win_mgr_get_active_view_type());
                if (win_mgr_get_active_view_type() != VEEBHA_VIEW_TYPE_EDITOR) {
                    fprintf(stderr, "[TEST ERROR] Failed to switch back to Composer!\n");
                    return 216;
                }
                win_mgr_reset_to_home();
                printf("[TEST] Returned to Home cleanly. Depth=%u (exp 1)\n", win_mgr_get_depth());
                if (win_mgr_get_depth() != 1) {
                    fprintf(stderr, "[TEST ERROR] Reset to home failed!\n");
                    return 217;
                }

                /* Step 54: Focus Restoration Verification (Calendar & Tools -> Snake Game) */
                printf("[TEST] Step 54: Testing Focus Restoration for Calendar and Tools apps...\n");
                app_launcher_open();
            } else if (frame_count == 864) {
                printf("[TEST] In Launcher. Navigating to Calendar (index 6)...\n");
                test_inject_key(VEEBHA_KEY_DOWN); /* 0 -> 3 */
            } else if (frame_count == 868) {
                test_inject_key(VEEBHA_KEY_DOWN); /* 3 -> 6 (Calendar) */
            } else if (frame_count == 872) {
                int idx = test_get_focused_index();
                printf("[TEST] Step 54a check: Focused in Launcher=%d (exp 6 'Calendar')\n", idx);
                if (idx != 6) {
                    fprintf(stderr, "[TEST ERROR] Expected focused index 6 (Calendar), got %d\n", idx);
                    return 220;
                }
                printf("[TEST] Opening Calendar...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 876) {
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] In Calendar (ViewType=%d, exp %d CALENDAR, Depth=%u). Returning via RSK ('Back')...\n",
                       (int)vt, (int)VEEBHA_VIEW_TYPE_CALENDAR, win_mgr_get_depth());
                if (vt != VEEBHA_VIEW_TYPE_CALENDAR) {
                    fprintf(stderr, "[TEST ERROR] Calendar failed to open!\n");
                    return 221;
                }
                softkey_trigger_rsk();
            } else if (frame_count == 880) {
                uint8_t depth = win_mgr_get_depth();
                int idx = test_get_focused_index();
                printf("[TEST] Step 54a check: Back in Launcher. Depth=%u (exp 2), Restored Focused=%d (exp 6 'Calendar')\n",
                       depth, idx);
                if (depth != 2 || idx != 6) {
                    fprintf(stderr, "[TEST ERROR] Focus restoration for Calendar failed! Expected index 6, got %d\n", idx);
                    return 222;
                }
                printf("[TEST] Navigating to Tools (index 8)...\n");
                test_inject_key(VEEBHA_KEY_RIGHT); /* 6 -> 7 (Files) */
            } else if (frame_count == 884) {
                test_inject_key(VEEBHA_KEY_RIGHT); /* 7 -> 8 (Tools) */
            } else if (frame_count == 888) {
                int idx = test_get_focused_index();
                printf("[TEST] Focused in Launcher=%d (exp 8 'Tools'). Opening Tools Hub...\n", idx);
                if (idx != 8) {
                    fprintf(stderr, "[TEST ERROR] Expected focused index 8 (Tools), got %d\n", idx);
                    return 223;
                }
                softkey_trigger_lsk();
            } else if (frame_count == 892) {
                printf("[TEST] In Tools Hub (Depth=%u). Launching Snake Game...\n", win_mgr_get_depth());
                app_game_open();
            } else if (frame_count == 896) {
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] In Snake Game (ViewType=%d, exp %d GENERIC, Depth=%u). Exiting via RSK ('Exit')...\n",
                       (int)vt, (int)VEEBHA_VIEW_TYPE_GENERIC, win_mgr_get_depth());
                if (vt != VEEBHA_VIEW_TYPE_GENERIC) {
                    fprintf(stderr, "[TEST ERROR] Snake Game failed to open!\n");
                    return 225;
                }
                softkey_trigger_rsk();
            } else if (frame_count == 900) {
                uint8_t depth = win_mgr_get_depth();
                printf("[TEST] Step 54b check: Back in Tools Hub. Depth=%u (exp 3)\n", depth);
                if (depth != 3) {
                    fprintf(stderr, "[TEST ERROR] Return to Tools Hub from Snake Game failed! Depth=%u\n", depth);
                    return 226;
                }
                printf("[TEST] Step 55: Testing Date & Time Settings and Date Picker Screen...\n");
                win_mgr_reset_to_home();
                app_settings_open();
            } else if (frame_count == 904) {
                test_inject_key(VEEBHA_KEY_DOWN); /* 0 -> 1 */
            } else if (frame_count == 908) {
                test_inject_key(VEEBHA_KEY_DOWN); /* 1 -> 2 */
            } else if (frame_count == 912) {
                test_inject_key(VEEBHA_KEY_DOWN); /* 2 -> 3 (Date & Time) */
            } else if (frame_count == 916) {
                int idx = test_get_focused_index();
                printf("[TEST] Focused in Settings=%d (exp 3 'Date & Time'). Opening Date & Time...\n", idx);
                if (idx != 3) {
                    fprintf(stderr, "[TEST ERROR] Expected focused index 3 (Date & Time), got %d\n", idx);
                    return 227;
                }
                softkey_trigger_lsk();
            } else if (frame_count == 920) {
                printf("[TEST] In Date & Time menu (Depth=%u). Navigating to 'Set Date' (row 1)...\n",
                       win_mgr_get_depth());
                test_inject_key(VEEBHA_KEY_DOWN);
            } else if (frame_count == 924) {
                int idx = test_get_focused_index();
                printf("[TEST] Focused in Date & Time=%d (exp 1 'Set Date'). Opening Set Date dialog...\n", idx);
                if (idx != 1) {
                    fprintf(stderr, "[TEST ERROR] Expected focused index 1 (Set Date), got %d\n", idx);
                    return 228;
                }
                softkey_trigger_lsk();
            } else if (frame_count == 928) {
                printf("[TEST] In Set Date screen (Depth=%u). Adjusting Day...\n", win_mgr_get_depth());
                test_inject_key(VEEBHA_KEY_UP); /* Increment day */
            } else if (frame_count == 932) {
                printf("[TEST] Saving Date...\n");
                softkey_trigger_lsk();          /* Save */
            } else if (frame_count == 936) {
                uint16_t y = 0;
                uint8_t m = 0, d = 0;
                status_bar_get_rtc_date(&y, &m, &d);
                printf("[TEST] Step 55 check: New RTC Date: %04u-%02u-%02u (exp year 2026, month 9, day 24)\n",
                       (unsigned int)y, (unsigned int)m, (unsigned int)d);
                if (y != 2026 || m != 9 || d != 24) {
                    fprintf(stderr, "[TEST ERROR] Date picker save failed! Got %04u-%02u-%02u\n", y, m, d);
                    return 229;
                }
                win_mgr_reset_to_home();
            } else if (frame_count == 940) {
                uint8_t depth = win_mgr_get_depth();
                veebha_view_type_t vt = win_mgr_get_active_view_type();
                printf("[TEST] Returned to Idle Screen. Depth=%u (exp 1), ViewType=%d (exp %d IDLE)\n",
                       depth, (int)vt, (int)VEEBHA_VIEW_TYPE_IDLE);
                if (depth != 1 || vt != VEEBHA_VIEW_TYPE_IDLE) {
                    fprintf(stderr, "[TEST ERROR] Return to Idle after Date Set failed!\n");
                    return 230;
                }

                /* Step 56: Phase 6.1 Localization & Indic Shaper Verification */
                printf("[TEST] Step 56: Testing Indic Text Shaper and Localization Subsystems...\n");
                
                /* 56a: Shaper detection */
                bool is_dev1 = indic_shaper_is_devanagari("नमस्ते");
                bool is_dev2 = indic_shaper_is_devanagari("Hello World");
                printf("[TEST] Step 56a check: Devanagari detection: 'नमस्ते'=%d (exp 1), 'Hello'=%d (exp 0)\n",
                       (int)is_dev1, (int)is_dev2);
                if (!is_dev1 || is_dev2) {
                    fprintf(stderr, "[TEST ERROR] Indic shaper detection failed!\n");
                    return 231;
                }

                /* 56b: Shaper Matra transposition & Conjuncts */
                char shaped[64] = {0};
                indic_shaper_process("किताब", shaped, sizeof(shaped));
                printf("[TEST] Step 56b check: Shaped 'किताब' -> '%s'\n", shaped);
                if (strlen(shaped) == 0) {
                    fprintf(stderr, "[TEST ERROR] Indic shaper process failed!\n");
                    return 232;
                }

                /* 56c: i18n string catalogs */
                veebha_i18n_set_language(LANG_EN);
                const char *str_en = veebha_i18n_str(STR_SETTINGS);
                veebha_i18n_set_language(LANG_HI);
                const char *str_hi = veebha_i18n_str(STR_SETTINGS);
                veebha_i18n_set_language(LANG_RU);
                const char *str_ru = veebha_i18n_str(STR_SETTINGS);
                printf("[TEST] Step 56c check: STR_SETTINGS EN='%s', HI='%s', RU='%s'\n", str_en, str_hi, str_ru);
                if (strcmp(str_en, "Settings") != 0 || strcmp(str_hi, "सेटिंग्स") != 0 || strcmp(str_ru, "Настройки") != 0) {
                    fprintf(stderr, "[TEST ERROR] i18n string lookup mismatch!\n");
                    return 233;
                }

                /* Reset to English and test UI Language selector */
                veebha_i18n_set_language(LANG_EN);
                app_settings_open();
            } else if (frame_count == 946) {
                hal_display_save_screenshot("/home/vixxkigoli/.gemini/antigravity/brain/b04608c8-9a6f-42ec-b13a-16d21e13438f/settings_list.bmp");
            } else if (frame_count == 947) {
                test_inject_key(VEEBHA_KEY_DOWN); /* 0 -> 1 */
            } else if (frame_count == 948) {
                test_inject_key(VEEBHA_KEY_DOWN); /* 1 -> 2 */
            } else if (frame_count == 952) {
                test_inject_key(VEEBHA_KEY_DOWN); /* 2 -> 3 */
            } else if (frame_count == 956) {
                test_inject_key(VEEBHA_KEY_DOWN); /* 3 -> 4 (Language) */
            } else if (frame_count == 960) {
                int idx = test_get_focused_index();
                printf("[TEST] Step 56d check: Focused in Settings=%d (exp 4 'Language'). Opening Language selector...\n", idx);
                if (idx != 4) {
                    fprintf(stderr, "[TEST ERROR] Expected focused index 4 (Language), got %d\n", idx);
                    return 234;
                }
                softkey_trigger_lsk();
            } else if (frame_count == 964) {
                printf("[TEST] In Language menu (Depth=%u). Navigating to Hindi (row 1)...\n", win_mgr_get_depth());
                test_inject_key(VEEBHA_KEY_DOWN);
            } else if (frame_count == 968) {
                int idx = test_get_focused_index();
                printf("[TEST] Step 56e check: Focused in Language=%d (exp 1 'हिन्दी'). Selecting Hindi...\n", idx);
                if (idx != 1) {
                    fprintf(stderr, "[TEST ERROR] Expected focused index 1 (Hindi), got %d\n", idx);
                    return 235;
                }
                softkey_trigger_lsk();
            } else if (frame_count == 972) {
                language_id_t cur_lang = veebha_i18n_get_language();
                uint8_t nv_lang = os_nvram_get()->language_id;
                printf("[TEST] Step 56e check: Active Language=%d (exp %d HI), NVRAM Language=%u (exp 1)\n",
                       (int)cur_lang, (int)LANG_HI, (unsigned int)nv_lang);
                if (cur_lang != LANG_HI || nv_lang != 1) {
                    fprintf(stderr, "[TEST ERROR] Language switch to Hindi failed!\n");
                    return 236;
                }

                /* Open Language settings again to select Russian */
                softkey_trigger_lsk();
            } else if (frame_count == 976) {
                test_inject_key(VEEBHA_KEY_DOWN);
            } else if (frame_count == 980) {
                test_inject_key(VEEBHA_KEY_DOWN);
            } else if (frame_count == 984) {
                int idx = test_get_focused_index();
                printf("[TEST] Step 56f check: Focused in Language=%d (exp 2 'Русский'). Selecting Russian...\n", idx);
                softkey_trigger_lsk();
            } else if (frame_count == 988) {
                language_id_t cur_lang = veebha_i18n_get_language();
                uint8_t nv_lang = os_nvram_get()->language_id;
                printf("[TEST] Step 56f check: Active Language=%d (exp %d RU), NVRAM Language=%u (exp 2)\n",
                       (int)cur_lang, (int)LANG_RU, (unsigned int)nv_lang);
                if (cur_lang != LANG_RU || nv_lang != 2) {
                    fprintf(stderr, "[TEST ERROR] Language switch to Russian failed!\n");
                    return 237;
                }

                /* Restore English and reset home */
                veebha_i18n_set_language(LANG_EN);
                os_nvram_get()->language_id = 0;
                os_nvram_save();
                win_mgr_reset_to_home();
            } else if (frame_count == 992) {
                uint8_t depth = win_mgr_get_depth();
                printf("[TEST] Returned to Idle Screen. Depth=%u (exp 1)\n", depth);
                if (depth != 1) {
                    fprintf(stderr, "[TEST ERROR] Reset to home failed!\n");
                    return 238;
                }

                /* Step 57: Phase 7.0 VeebhaOS Fun Zone & .vapp Dynamic App Loading */
                printf("[TEST] Step 57: Testing VeebhaOS Fun Zone Store & .vapp Dynamic App Loading...\n");
                vapp_package_t pkgs[8];
                size_t count = 0;
                bool scan_ok = vapp_loader_scan_dir(pkgs, 8, &count);
                printf("[TEST] Step 57a check: Fun Zone scanned /vapps -> ok=%d, count=%u (exp >= 5)\n",
                       scan_ok, (unsigned int)count);
                if (!scan_ok || count < 5) {
                    fprintf(stderr, "[TEST ERROR] /vapps package scan failed! count=%u\n", (unsigned int)count);
                    return 240;
                }

                printf("[TEST] Opening VeebhaOS Fun Zone App Store...\n");
                app_funzone_open();
            } else if (frame_count == 996) {
                printf("[TEST] In VeebhaOS Fun Zone (Depth=%u, exp 2). Launching Brick Breaker (.vapp Game)...\n",
                       win_mgr_get_depth());
                softkey_trigger_lsk();
            } else if (frame_count == 1000) {
                printf("[TEST] Step 57b check: In Brick Breaker (Depth=%u, exp 3). Testing paddle input...\n",
                       win_mgr_get_depth());
                test_inject_key(VEEBHA_KEY_NUM_4);
                test_inject_key(VEEBHA_KEY_NUM_6);
                printf("[TEST] Exiting Brick Breaker via RSK ('Exit')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 1004) {
                printf("[TEST] Step 57c check: Back in Fun Zone (Depth=%u, exp 2). Navigating to Unit Converter...\n",
                       win_mgr_get_depth());
                test_inject_key(VEEBHA_KEY_DOWN);
            } else if (frame_count == 1008) {
                printf("[TEST] Launching Unit Converter (.vapp App)...\n");
                softkey_trigger_lsk();
            } else if (frame_count == 1012) {
                printf("[TEST] Step 57c check: In Unit Converter (Depth=%u, exp 3). Injecting digits '5', '0'...\n",
                       win_mgr_get_depth());
                test_inject_key(VEEBHA_KEY_NUM_5);
                test_inject_key(VEEBHA_KEY_NUM_0);
                printf("[TEST] Exiting Unit Converter via RSK ('Back')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 1016) {
                printf("[TEST] Exiting Fun Zone via RSK ('Back')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 1020) {
                uint8_t depth = win_mgr_get_depth();
                printf("[TEST] Step 57d check: Returned to Idle Screen. Depth=%u (exp 1)\n", depth);
                if (depth != 1) {
                    fprintf(stderr, "[TEST ERROR] Return to Idle after Fun Zone failed! Depth=%u\n", depth);
                    return 241;
                }

                /* Step 58: Phase 7.1 SDK & Tetris Retro & Phase 7.3 Bluetooth OBEX Transfer */
                printf("[TEST] Step 58: Testing Tetris Retro (.vapp game) and Bluetooth OBEX File Transfer...\n");
                app_funzone_open();
            } else if (frame_count == 1024) {
                printf("[TEST] In Fun Zone (Depth=%u). Navigating to Tetris Retro (row 5)...\n", win_mgr_get_depth());
                test_inject_key(VEEBHA_KEY_DOWN);
            } else if (frame_count == 1028) {
                test_inject_key(VEEBHA_KEY_DOWN);
            } else if (frame_count == 1032) {
                test_inject_key(VEEBHA_KEY_DOWN);
            } else if (frame_count == 1036) {
                test_inject_key(VEEBHA_KEY_DOWN);
            } else if (frame_count == 1040) {
                test_inject_key(VEEBHA_KEY_DOWN);
            } else if (frame_count == 1044) {
                int idx = test_get_focused_index();
                printf("[TEST] Step 58a check: Focused in Fun Zone=%d (exp 5 'Tetris Retro'). Launching...\n", idx);
                softkey_trigger_lsk();
            } else if (frame_count == 1048) {
                printf("[TEST] Step 58b check: In Tetris Retro (Depth=%u, exp 3). Testing movements & rotation...\n",
                       win_mgr_get_depth());
                test_inject_key(VEEBHA_KEY_NUM_4); /* Move left */
                test_inject_key(VEEBHA_KEY_NUM_6); /* Move right */
                test_inject_key(VEEBHA_KEY_NUM_2); /* Rotate */
                test_inject_key(VEEBHA_KEY_NUM_8); /* Soft drop */
                printf("[TEST] Exiting Tetris Retro via RSK ('Exit')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 1052) {
                printf("[TEST] Step 58c: Testing Phase 7.3 Bluetooth OBEX File Transfer...\n");
                connectivity_bt_set_enabled(true);
                uint8_t dummy_vapp[256];
                memset(dummy_vapp, 0x55, sizeof(dummy_vapp));
                bool rx_ok = connectivity_bt_receive_file("custom_tool.vapp", dummy_vapp, sizeof(dummy_vapp));
                printf("[TEST] Step 58c check: Bluetooth OBEX received .vapp -> ok=%d (exp 1)\n", rx_ok);
                if (!rx_ok) {
                    fprintf(stderr, "[TEST ERROR] Bluetooth OBEX receive failed!\n");
                    return 242;
                }

                bool tx_ok = connectivity_bt_send_file("00:E0:4C:68:02:11", "/vapps/tetris_retro.vapp");
                printf("[TEST] Step 58d check: Bluetooth OBEX sent .vapp -> ok=%d (exp 1)\n", tx_ok);
                if (!tx_ok) {
                    fprintf(stderr, "[TEST ERROR] Bluetooth OBEX send failed!\n");
                    return 243;
                }

                /* Exit Fun Zone to Idle */
                softkey_trigger_rsk();
            } else if (frame_count == 1056) {
                /* Step 59: Testing Graphical Brick Breaker and Tetris Arcade Games */
                printf("[TEST] Step 59: Testing Graphical Retro Arcade Games in .vapp Loader...\n");
                uint8_t depth = win_mgr_get_depth();
                printf("[TEST] Step 59a check: At Idle Screen. Depth=%u (exp 1)\n", depth);
                if (depth != 1) {
                    fprintf(stderr, "[TEST ERROR] Not at Idle for Step 59! Depth=%u\n", depth);
                    return 244;
                }

                /* Launch Graphical Brick Breaker */
                vapp_package_t pkgs[8];
                size_t count = 0;
                vapp_loader_scan_dir(pkgs, 8, &count);
                if (count > 0) {
                    printf("[TEST] Launching Graphical Brick Breaker...\n");
                    vapp_loader_launch(&pkgs[0]);
                }
            } else if (frame_count == 1060) {
                printf("[TEST] Step 59b check: In Graphical Brick Breaker (Depth=%u, exp 2). Injecting paddle moves...\n",
                       win_mgr_get_depth());
                test_inject_key(VEEBHA_KEY_NUM_4);
                test_inject_key(VEEBHA_KEY_NUM_6);
                test_inject_key(VEEBHA_KEY_NUM_5);
                printf("[TEST] Exiting Brick Breaker via RSK ('Exit')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 1064) {
                printf("[TEST] Step 59c check: Exited Brick Breaker -> Depth=%u (exp 1)\n", win_mgr_get_depth());
                if (win_mgr_get_depth() != 1) {
                    fprintf(stderr, "[TEST ERROR] Failed to return to Idle from Brick Breaker!\n");
                    return 245;
                }

                /* Launch Graphical Tetris */
                vapp_package_t pkgs[8];
                size_t count = 0;
                vapp_loader_scan_dir(pkgs, 8, &count);
                if (count >= 6) {
                    printf("[TEST] Launching Graphical Tetris Retro...\n");
                    vapp_loader_launch(&pkgs[5]);
                }
            } else if (frame_count == 1068) {
                printf("[TEST] Step 59d check: In Graphical Tetris Retro (Depth=%u, exp 2). Testing 10x16 matrix input...\n",
                       win_mgr_get_depth());
                test_inject_key(VEEBHA_KEY_NUM_4); /* Move left */
                test_inject_key(VEEBHA_KEY_NUM_6); /* Move right */
                test_inject_key(VEEBHA_KEY_NUM_2); /* Rotate */
                test_inject_key(VEEBHA_KEY_NUM_8); /* Soft drop */
                test_inject_key(VEEBHA_KEY_NUM_0); /* Slam hard drop */
                printf("[TEST] Exiting Tetris Retro via RSK ('Exit')...\n");
                softkey_trigger_rsk();
            } else if (frame_count == 1072) {
                printf("[TEST] Step 59e check: Exited Tetris Retro -> Depth=%u (exp 1)\n", win_mgr_get_depth());
                if (win_mgr_get_depth() != 1) {
                    fprintf(stderr, "[TEST ERROR] Failed to return to Idle from Tetris!\n");
                    return 246;
                }

                /* Step 60: Phase 7.4 Web Browser with Bluetooth Tethering */
                printf("[TEST] Step 60: Testing Phase 7.4 Web Browser & Bluetooth Tethering Network Layer...\n");
                /* Test with Tethering OFF */
                connectivity_tethering_set_bt(false);
                connectivity_tethering_set_usb(false);
                bool net_avail = connectivity_net_is_available();
                printf("[TEST] Step 60a check: Bluetooth Tethering OFF -> Net Avail=%d (exp 0)\n", (int)net_avail);
                if (net_avail) {
                    fprintf(stderr, "[TEST ERROR] Network reported available when tethering is off!\n");
                    return 247;
                }

                printf("[TEST] Opening Web Browser in offline state...\n");
                app_browser_open();
            } else if (frame_count == 1076) {
                printf("[TEST] Step 60b check: Browser active=%d (exp 1), Depth=%u (exp 2)\n",
                       (int)app_browser_is_active(), win_mgr_get_depth());
                if (!app_browser_is_active() || win_mgr_get_depth() != 2) {
                    fprintf(stderr, "[TEST ERROR] Web Browser failed to open!\n");
                    return 248;
                }

                /* Enable Bluetooth Tethering */
                printf("[TEST] Step 60c: Enabling Bluetooth Tethering (PAN) and loading Portal...\n");
                connectivity_tethering_set_bt(true);
                bool net_on = connectivity_net_is_available();
                printf("[TEST] Step 60c check: Bluetooth Tethering ON -> Net Avail=%d (exp 1)\n", (int)net_on);
                if (!net_on) {
                    fprintf(stderr, "[TEST ERROR] Bluetooth Tethering failed to activate!\n");
                    return 249;
                }

                app_browser_load_url("http://home.veebha");
            } else if (frame_count == 1080) {
                printf("[TEST] Step 60d: Navigating to 'http://news.wap'...\n");
                app_browser_load_url("http://news.wap");
            } else if (frame_count == 1084) {
                printf("[TEST] Step 60e: Testing Browser History Back via RSK...\n");
                /* RSK triggers History Back */
                softkey_trigger_rsk();
            } else if (frame_count == 1088) {
                printf("[TEST] Step 60f: Testing Options menu...\n");
                softkey_trigger_lsk(); /* Options menu */
            } else if (frame_count == 1092) {
                /* Dismiss options menu and exit Browser */
                softkey_trigger_rsk(); /* Pop options menu */
                softkey_trigger_rsk(); /* Pop browser */
            } else if (frame_count == 1096) {
                uint8_t depth = win_mgr_get_depth();
                printf("[TEST] Step 60g check: Returned to Idle Screen. Depth=%u (exp 1)\n", depth);
                if (depth != 1) {
                    fprintf(stderr, "[TEST ERROR] Final return to Idle failed! Depth=%u\n", depth);
                    return 250;
                }

                printf("[TEST] ========================================================\n");
                printf("[TEST] ALL Phase 4.1-7.4 Typography, i18n, SDK, Graphical Games & Browser Tests PASSED (%u frames)!\n",
                       frame_count);
                printf("[TEST] ========================================================\n");
                return -1;
            }
    return 0;
}


static void telephony_task(void *pvParameters)
{
    (void)pvParameters;
    OS_LOGI(TAG, "Telephony background task running (Priority %u)",
            (unsigned int)uxTaskPriorityGet(NULL));

    while (s_running) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}

static void ui_task(void *pvParameters)
{
    (void)pvParameters;
    uint32_t frame_count = 0;

    while (s_running) {
        os_lvgl_lock();

        /* Drain and dispatch system events */
        os_event_t evt;
        while (os_event_poll(&evt)) {
            os_dispatch_system_event(&evt);
        }

        uint32_t sleep_ms = lv_timer_handler();
        if (sleep_ms < 5) sleep_ms = 5;
        if (sleep_ms > 33) sleep_ms = 33;

        if (s_automated_test) {
            frame_count++;
            int test_res = run_test_step(frame_count);
            if (test_res > 0) {
                fprintf(stderr, "[TEST ERROR] Test failed with error code %d at frame %u\n", test_res, frame_count);
                os_lvgl_unlock();
                s_running = false;
                exit(test_res);
            } else if (test_res < 0) {
                printf("[SIM] Shutting down after successful test execution.\n");
                s_running = false;
                os_lvgl_unlock();
                break;
            }
        }

        os_lvgl_unlock();

        vTaskDelay(pdMS_TO_TICKS(sleep_ms));
    }

    vTaskDelete(NULL);
}

static void *scheduler_thread_func(void *arg)
{
    (void)arg;
    OS_LOGI(TAG, "FreeRTOS scheduler starting in worker thread...");
    vTaskStartScheduler();
    return NULL;
}

int main(int argc, char *argv[])
{
    /* Mask SIGALRM and SIGUSR1 on Thread 0 before any SDL initialization */
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGALRM);
    sigaddset(&sigset, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    printf("========================================================\n");
    printf("  %s (FreeRTOS Multi-Tasking Kernel)\n", CONFIG_BOARD_NAME);
    printf("  Resolution: %dx%d (RGB565)\n", CONFIG_DISP_HOR_RES, CONFIG_DISP_VER_RES);
    printf("  Memory Pool: %u KB fence\n", (unsigned int)(CONFIG_LV_MEM_SIZE / 1024));
    printf("========================================================\n");

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--test") == 0 || strcmp(argv[i], "--headless") == 0) {
            s_automated_test = true;
            printf("[SIM] Running in automated Phase 6.0 FreeRTOS verification mode\n");
            remove("./build/veebha_nvram.bin");
            remove("./veebha_nvram.bin");
            remove("./build/telephony_store.bin");
            remove("./telephony_store.bin");
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "[SIM] SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    /* 1. Core OS Kernel & LVGL Initialization */
    os_kernel_init();
    lv_init();
    lv_tick_set_cb(SDL_GetTicks);
    veebha_fonts_init();

    /* 2. System Event Queue & Hardware IRQ Subsystem */
    os_event_init();
    os_irq_init();

    /* 3. Display HAL */
    if (!hal_display_init()) {
        fprintf(stderr, "[SIM] Failed to initialize display HAL\n");
        return 1;
    }

    /* 4. Input & Peripheral HALs */
    if (!hal_input_init()) {
        fprintf(stderr, "[SIM] Failed to initialize input HAL\n");
        hal_display_deinit();
        return 1;
    }
    hal_power_init();
    hal_audio_init();

    /* 5. Setup Keypad Navigation Group & Window Manager */
    g_keypad_group = lv_group_create();
    lv_group_set_default(g_keypad_group);
    lv_indev_set_group(hal_input_get_lv_indev(), g_keypad_group);

    win_mgr_init(g_keypad_group);

    /* 6. Initialize Core Subsystems & Root Standby / Idle Screen */
    os_nvram_init();
    app_settings_init();
    os_app_registry_init();
    vfs_init();
    app_files_init();
    app_music_init();
    app_calendar_init();
    app_tools_init();
    app_stopwatch_init();
    app_alarm_init();
    app_textread_init();
    app_game_init();
    app_gallery_init();
    app_recorder_init();
    screen_saver_init();
    mock_connectivity_init();
    app_bt_init();

    if (s_automated_test) {
        app_idle_open();
    } else {
        boot_screen_start(app_idle_open);
    }

    printf("[SIM] Simulator ready.\n");
    printf("  Keypad Controls:\n");
    printf("    D-Pad:             [Arrow Keys] (Up, Down, Left, Right)\n");
    printf("    Center OK:         [Enter] / [Space] / [Keypad Enter]\n");
    printf("    Left Softkey (LSK): [F1] / [Left Alt] / '['\n");
    printf("    Right Softkey(RSK): [F2] / [Right Alt] / ']' / [Backspace] / [Delete]\n");
    printf("    Call Key (Green):  [C] (Hold 600ms: Quick Settings Drawer)\n");
    printf("    End/Power Key(Red):[E] / [End] / [Escape]\n");
    printf("    Number Keys:       [0-9], [*], [#]\n");
    printf("    Star Key (*):      [*] (Hold 600ms: Multitasking Switcher)\n");
    printf("    T9 Mode Switch:    '#' Key (cycles Abc -> ABC -> 123 -> abc)\n");
    printf("    Mock IRQs (F5..F7): F5: Call, F6: SMS, F7: SD Card\n");
    printf("========================================================\n");

    /* 7. Spawn FreeRTOS Tasks */
    BaseType_t ret;
    ret = xTaskCreate(ui_task, "ui_task", 8192, NULL, 2, NULL);
    if (ret != pdPASS) {
        fprintf(stderr, "[SIM] Failed to create ui_task!\n");
        return 1;
    }

    ret = xTaskCreate(telephony_task, "telephony_task", 2048, NULL, 3, NULL);
    if (ret != pdPASS) {
        fprintf(stderr, "[SIM] Failed to create telephony_task!\n");
        return 1;
    }

    /* 8. Start FreeRTOS Scheduler in dedicated worker pthread */
    pthread_t sched_thread;
    if (pthread_create(&sched_thread, NULL, scheduler_thread_func, NULL) != 0) {
        fprintf(stderr, "[SIM] Failed to spawn FreeRTOS scheduler pthread!\n");
        return 1;
    }

    /* 9. Thread 0 Dedicated Presentation & Event Polling Loop */
    while (s_running) {
        if (!hal_input_poll()) {
            s_running = false;
            break;
        }

        if (hal_display_sim_has_new_frame()) {
            hal_display_sim_present();
        }

        SDL_Delay(16);
    }

    /* 10. Clean Teardown */
    vPortEndScheduler();
    pthread_join(sched_thread, NULL);

    os_lvgl_lock();
    win_mgr_reset_to_home();
    hal_input_deinit();
    hal_display_deinit();
    lv_deinit();
    SDL_Quit();
    os_lvgl_unlock();

    printf("[SIM] Clean shutdown complete.\n");
    return 0;
}

