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

#include "app_launcher.h"
#include "app_registry.h"
#include "apps/music/app_music.h"
#include "sdk/include/veebha_templates.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_log.h"
#include "sdk/include/veebha_i18n.h"
#include <stdio.h>
#include <string.h>

#define TAG "APP_LAUNCHER"

static tpl_grid_item_t s_grid_items[OS_MAX_REGISTERED_APPS];

static const char *get_localized_app_title(const os_app_desc_t *app)
{
    if (!app || !app->id) return "";
    if (strcmp(app->id, "phone") == 0) return veebha_i18n_str(STR_PHONE);
    if (strcmp(app->id, "messages") == 0) return veebha_i18n_str(STR_MESSAGES);
    if (strcmp(app->id, "contacts") == 0) return veebha_i18n_str(STR_CONTACTS);
    if (strcmp(app->id, "calllogs") == 0) return veebha_i18n_str(STR_CALL_LOGS);
    if (strcmp(app->id, "music") == 0) return veebha_i18n_str(STR_MUSIC);
    if (strcmp(app->id, "settings") == 0) return veebha_i18n_str(STR_SETTINGS);
    if (strcmp(app->id, "calendar") == 0) return veebha_i18n_str(STR_CALENDAR);
    if (strcmp(app->id, "files") == 0) return veebha_i18n_str(STR_STORAGE);
    if (strcmp(app->id, "tools") == 0) return veebha_i18n_str(STR_TOOLS);
    return app->name ? app->name : "";
}

static void on_launcher_select(uint16_t index)
{
    const os_app_desc_t *app = os_app_get_by_index(index);
    if (!app) return;

    OS_LOGI(TAG, "Launcher Item %u (%s) Selected", index, app->name);

    /* 1. Check if a backgrounded task already exists for this app */
    const char *loc_title = get_localized_app_title(app);
    os_task_entry_t *existing = win_mgr_find_task(app->name);
    if (!existing && app->id) existing = win_mgr_find_task(app->id);
    if (!existing && loc_title && loc_title[0]) existing = win_mgr_find_task(loc_title);

    if (existing) {
        os_task_entry_t *cur_task = win_mgr_get_active_task();
        if (cur_task && cur_task != existing) {
            uint8_t count = win_mgr_get_task_count();
            for (uint8_t i = 0; i < count; i++) {
                if (win_mgr_get_task(i) == cur_task) {
                    if (win_mgr_get_depth() == 2) {
                        win_mgr_task_kill(i);
                    }
                    break;
                }
            }
        }
        win_mgr_task_resume(app->name);
        OS_LOGI(TAG, "Resumed existing task '%s'", app->name);
        return;
    }

    /* 2. Launch fresh instance of application in current stack */
    if (app->launch_cb) {
        os_task_entry_t *cur = win_mgr_get_active_task();
        if (cur) {
            strncpy(cur->name, app->name, sizeof(cur->name) - 1);
            cur->name[sizeof(cur->name) - 1] = '\0';
        }
        app->launch_cb();
    } else {
        OS_LOGI(TAG, "App '%s' has no launch callback (stub)", app->name);
    }
}

static lv_obj_t *s_launcher_screen = NULL;

void app_launcher_invalidate(void)
{
    if (s_launcher_screen && lv_obj_is_valid(s_launcher_screen)) {
        if (!win_mgr_is_screen_in_stack(s_launcher_screen)) {
            win_mgr_set_launcher_screen(NULL);
            lv_obj_delete_async(s_launcher_screen);
            s_launcher_screen = NULL;
        }
    }
}

lv_obj_t * app_launcher_create(void)
{
    uint16_t count = os_app_get_count();
    if (count > OS_MAX_REGISTERED_APPS) count = OS_MAX_REGISTERED_APPS;

    for (uint16_t i = 0; i < count; i++) {
        const os_app_desc_t *app = os_app_get_by_index(i);
        if (app) {
            s_grid_items[i].icon = app->icon;
            s_grid_items[i].title = get_localized_app_title(app);
        }
    }

    tpl_grid_view_t desc = {
        .title = veebha_i18n_str(STR_APP_NAME),
        .items = s_grid_items,
        .count = count,
        .columns = 3,
        .on_select = on_launcher_select,
        .on_back = NULL, /* Defaults to win_mgr_pop() -> returns to Idle */
        .lsk_label = veebha_i18n_str(STR_SELECT),
        .rsk_label = veebha_i18n_str(STR_BACK)
    };

    return tpl_grid_create(&desc);
}

void app_launcher_open(void)
{
    if (!s_launcher_screen || !lv_obj_is_valid(s_launcher_screen)) {
        s_launcher_screen = app_launcher_create();
        win_mgr_set_launcher_screen(s_launcher_screen);
    }
    if (s_launcher_screen) {
        win_mgr_push(s_launcher_screen, veebha_i18n_str(STR_SELECT), tpl_grid_default_lsk, veebha_i18n_str(STR_BACK), tpl_grid_default_rsk);
    }
}
