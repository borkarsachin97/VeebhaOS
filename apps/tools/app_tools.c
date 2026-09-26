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

#include "app_tools.h"
#include "app_calc.h"
#include "app_stopwatch.h"
#include "app_alarm.h"
#include "app_textread.h"
#include "apps/game/app_game.h"
#include "apps/gallery/app_gallery.h"
#include "apps/recorder/app_recorder.h"
#include "sdk/include/app_registry.h"
#include "sdk/include/veebha_templates.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TAG "APP_TOOLS"

static void on_torch_off_action(void)
{
    win_mgr_pop();
}

static void on_torch_delete_cb(lv_event_t *e)
{
    lv_obj_t *scr = lv_event_get_target(e);
    win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)lv_obj_get_user_data(scr);
    if (hdr) {
        free(hdr);
        lv_obj_set_user_data(scr, NULL);
    }
}

void app_tools_open_torch(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, 176, 220);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)calloc(1, sizeof(win_mgr_screen_hdr_t));
    if (hdr) {
        hdr->view_type = VEEBHA_VIEW_TYPE_GENERIC;
        strncpy(hdr->title, "Torch", sizeof(hdr->title) - 1);
        lv_obj_set_user_data(screen, hdr);
    }
    lv_obj_add_event_cb(screen, on_torch_delete_cb, LV_EVENT_DELETE, NULL);

    /* Viewport: Pure white canvas filling remaining space */
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_size(content, lv_pct(100), 0);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_color(content, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_radius(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(content);
    lv_label_set_text(lbl, LV_SYMBOL_EYE_OPEN "\n\nTORCH ACTIVE");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);

    /* Bottom 20px Softkey Bar */
    lv_obj_t *sk = softkey_bar_create(screen, "Off", "Back");
    if (hdr) {
        hdr->softkey_bar = sk;
    }

    win_mgr_push(screen, "Off", on_torch_off_action, "Back", on_torch_off_action);
}


#include "app_funzone.h"
#include "apps/browser/app_browser.h"

static void on_tools_item_select(uint16_t index)
{
    OS_LOGI(TAG, "Tools Item %u Selected", index);
    switch (index) {
    case 0:
        app_calc_open();
        break;
    case 1:
        app_stopwatch_open();
        break;
    case 2:
        app_alarm_open();
        break;
    case 3:
        app_textread_open();
        break;
    case 4:
        app_recorder_open();
        break;
    case 5:
        app_tools_open_torch();
        break;
    case 6:
        app_game_open();
        break;
    case 7:
        app_gallery_open();
        break;
    case 8:
        app_funzone_open();
        break;
    case 9:
        app_browser_open();
        break;
    default:
        break;
    }
}

#include "sdk/include/veebha_i18n.h"
#include "apps/i18n/cache.h"

void app_tools_init(void)
{
    OS_LOGI(TAG, "Tools application module initialized");
    app_funzone_init();
    app_browser_init();
}

#include "sdk/include/veebha_theme.h"

static lv_obj_t *s_tools_screen = NULL;
static language_id_t s_tools_lang = (language_id_t)-1;
static os_theme_id_t s_tools_theme = (os_theme_id_t)0xFF;

static void on_tools_screen_deleted(lv_event_t *e)
{
    (void)e;
    s_tools_screen = NULL;
}

void app_tools_invalidate(void)
{
    if (s_tools_screen && lv_obj_is_valid(s_tools_screen)) {
        if (!win_mgr_is_screen_in_stack(s_tools_screen)) {
            lv_obj_delete_async(s_tools_screen);
            s_tools_screen = NULL;
        }
    }
}

void app_tools_open(void)
{
#if defined(CONFIG_BOARD_SIMULATOR) && CONFIG_BOARD_SIMULATOR
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
#endif

    language_id_t cur_lang = veebha_i18n_get_language();
    os_theme_id_t cur_theme = theme_get_palette();

    if (s_tools_screen && lv_obj_is_valid(s_tools_screen)) {
        if (s_tools_lang != cur_lang || s_tools_theme != cur_theme) {
            lv_obj_delete_async(s_tools_screen);
            s_tools_screen = NULL;
        } else {
            win_mgr_push(s_tools_screen, veebha_i18n_str(STR_SELECT), tpl_list_default_lsk, veebha_i18n_str(STR_BACK), tpl_list_default_rsk);
#if defined(CONFIG_BOARD_SIMULATOR) && CONFIG_BOARD_SIMULATOR
            clock_gettime(CLOCK_MONOTONIC, &t1);
            long us = (t1.tv_sec - t0.tv_sec) * 1000000L + (t1.tv_nsec - t0.tv_nsec) / 1000L;
            printf("[TOOLS] Tools Hub menu opened (cached, warm: %ld us)\n", us);
#else
            printf("[TOOLS] Tools Hub menu opened (cached, warm)\n");
#endif
            return;
        }
    }

    static tpl_list_item_t s_tools_items[10];
    s_tools_items[0] = (tpl_list_item_t){ .icon = NULL, .title = veebha_i18n_str(STR_CALCULATOR),  .subtext = "Basic Math Evaluator" };
    s_tools_items[1] = (tpl_list_item_t){ .icon = NULL, .title = veebha_i18n_str(STR_STOPWATCH),   .subtext = "Lap & Countdown" };
    s_tools_items[2] = (tpl_list_item_t){ .icon = NULL, .title = veebha_i18n_str(STR_ALARM),       .subtext = "Daily Wakeup Alarm" };
    s_tools_items[3] = (tpl_list_item_t){ .icon = NULL, .title = veebha_i18n_str(STR_TEXT_VIEWER), .subtext = "View TXT Documents" };
    s_tools_items[4] = (tpl_list_item_t){ .icon = NULL, .title = "Camera & Recorder",              .subtext = "Photo, Video & Voice" };
    s_tools_items[5] = (tpl_list_item_t){ .icon = NULL, .title = "Torch / Flashlight",             .subtext = "Screen Flashlight" };
    s_tools_items[6] = (tpl_list_item_t){ .icon = NULL, .title = veebha_i18n_str(STR_SNAKE),       .subtext = "Retro Arcade Game" };
    s_tools_items[7] = (tpl_list_item_t){ .icon = NULL, .title = veebha_i18n_str(STR_GALLERY),     .subtext = "Browse SD Photos" };
    s_tools_items[8] = (tpl_list_item_t){ .icon = NULL, .title = "VeebhaOS Fun Zone",              .subtext = "App & Game Store (.vapp)" };
    s_tools_items[9] = (tpl_list_item_t){ .icon = NULL, .title = veebha_i18n_str(STR_BROWSER),     .subtext = "Internet via BT Tethering" };

    tpl_list_view_t desc = {
        .title = veebha_i18n_str(STR_TOOLS),
        .items = s_tools_items,
        .count = sizeof(s_tools_items) / sizeof(s_tools_items[0]),
        .on_select = on_tools_item_select,
        .on_back = NULL, /* Defaults to win_mgr_pop() */
        .lsk_label = veebha_i18n_str(STR_SELECT),
        .rsk_label = veebha_i18n_str(STR_BACK),
        .keep_alive = true,
    };

    s_tools_screen = tpl_list_create(&desc);
    if (s_tools_screen) {
        s_tools_lang = cur_lang;
        s_tools_theme = cur_theme;
        lv_obj_add_event_cb(s_tools_screen, on_tools_screen_deleted, LV_EVENT_DELETE, NULL);
        win_mgr_push(s_tools_screen, veebha_i18n_str(STR_SELECT), tpl_list_default_lsk, veebha_i18n_str(STR_BACK), tpl_list_default_rsk);
#if defined(CONFIG_BOARD_SIMULATOR) && CONFIG_BOARD_SIMULATOR
        clock_gettime(CLOCK_MONOTONIC, &t1);
        long us = (t1.tv_sec - t0.tv_sec) * 1000000L + (t1.tv_nsec - t0.tv_nsec) / 1000L;
        printf("[TOOLS] Tools Hub menu opened (cold: %ld us)\n", us);
#else
        printf("[TOOLS] Tools Hub menu opened (cold)\n");
#endif
    }
}
