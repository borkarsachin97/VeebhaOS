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

#include "app_registry.h"
#include "apps/dialer/app_dialer.h"
#include "apps/messages/app_messages.h"
#include "apps/contacts/app_contacts.h"
#include "apps/calllogs/app_calllogs.h"
#include "apps/music/app_music.h"
#include "apps/files/app_files.h"
#include "apps/settings/app_settings.h"
#include "apps/calendar/app_calendar.h"
#include "apps/tools/app_tools.h"
#include "sdk/include/veebha_log.h"
#include "lvgl.h"
#include <string.h>
#include <stdio.h>

#define TAG "APP_REGISTRY"

static os_app_desc_t s_apps[OS_MAX_REGISTERED_APPS];
static uint16_t      s_app_count = 0;

static os_qs_tile_desc_t s_tiles[OS_MAX_QS_TILES];
static uint8_t           s_tile_count = 0;

static void launch_dialer_default(void)
{
    app_dialer_open(NULL);
}

void os_app_registry_init(void)
{
    s_app_count = 0;
    memset(s_apps, 0, sizeof(s_apps));

    /* Register Core System Applications */
    os_app_desc_t core_apps[] = {
        { .id = "phone",     .name = "Phone",     .icon = LV_SYMBOL_CALL,      .launch_cb = launch_dialer_default, .priority = 10 },
        { .id = "messages",  .name = "Messages",  .icon = LV_SYMBOL_ENVELOPE,  .launch_cb = app_messages_open,     .priority = 20 },
        { .id = "contacts",  .name = "Contacts",  .icon = LV_SYMBOL_LIST,      .launch_cb = app_contacts_open,     .priority = 30 },
        { .id = "calllogs",  .name = "Call Logs",  .icon = LV_SYMBOL_REFRESH,   .launch_cb = app_calllogs_open,     .priority = 40 },
        { .id = "music",     .name = "Music",     .icon = LV_SYMBOL_AUDIO,     .launch_cb = app_music_open,        .priority = 50 },
        { .id = "settings",  .name = "Settings",  .icon = LV_SYMBOL_SETTINGS,  .launch_cb = app_settings_open,     .priority = 60 },
        { .id = "calendar",  .name = "Calendar",  .icon = LV_SYMBOL_BELL,      .launch_cb = app_calendar_open,     .priority = 70 },
        { .id = "files",     .name = "Files",     .icon = LV_SYMBOL_DIRECTORY, .launch_cb = app_files_open,        .priority = 80 },
        { .id = "tools",     .name = "Tools",     .icon = LV_SYMBOL_WIFI,      .launch_cb = app_tools_open,        .priority = 90 },
    };

    size_t count = sizeof(core_apps) / sizeof(core_apps[0]);
    for (size_t i = 0; i < count; i++) {
        os_app_register(&core_apps[i]);
    }

    s_tile_count = 0;
    memset(s_tiles, 0, sizeof(s_tiles));

    /* Register Default Quick Settings Tiles */
    os_qs_tile_desc_t core_tiles[] = {
        { .id = "bt",         .name = "Bluetooth", .icon = LV_SYMBOL_BLUETOOTH,    .priority = 10 },
        { .id = "brightness", .name = "Brightness",.icon = LV_SYMBOL_EYE_OPEN,     .priority = 20 },
        { .id = "profile",    .name = "Profile",   .icon = LV_SYMBOL_BELL,         .priority = 30 },
        { .id = "torch",      .name = "Torch",     .icon = LV_SYMBOL_CHARGE,       .priority = 40 },
        { .id = "battery",    .name = "Battery",   .icon = LV_SYMBOL_BATTERY_FULL, .priority = 50 },
        { .id = "settings",   .name = "Settings",  .icon = LV_SYMBOL_SETTINGS,     .priority = 60 },
    };

    size_t tile_count = sizeof(core_tiles) / sizeof(core_tiles[0]);
    for (size_t i = 0; i < tile_count; i++) {
        os_qs_tile_register(&core_tiles[i]);
    }

    OS_LOGI(TAG, "Application registry initialized with %u apps, %u QS tiles", s_app_count, s_tile_count);
}

bool os_app_register(const os_app_desc_t *desc)
{
    if (!desc || !desc->id || !desc->name) return false;
    if (s_app_count >= OS_MAX_REGISTERED_APPS) {
        OS_LOGW(TAG, "App registry full! Cannot register '%s'", desc->id);
        return false;
    }

    /* Check if already registered */
    for (uint16_t i = 0; i < s_app_count; i++) {
        if (strcmp(s_apps[i].id, desc->id) == 0) {
            s_apps[i] = *desc;
            return true;
        }
    }

    /* Insert ordered by priority */
    uint16_t insert_idx = s_app_count;
    for (uint16_t i = 0; i < s_app_count; i++) {
        if (desc->priority < s_apps[i].priority) {
            insert_idx = i;
            break;
        }
    }

    for (uint16_t i = s_app_count; i > insert_idx; i--) {
        s_apps[i] = s_apps[i - 1];
    }

    s_apps[insert_idx] = *desc;
    s_app_count++;

    OS_LOGD(TAG, "Registered app '%s' ('%s') at index %u", desc->id, desc->name, insert_idx);
    return true;
}

uint16_t os_app_get_count(void)
{
    return s_app_count;
}

const os_app_desc_t * os_app_get_by_index(uint16_t index)
{
    if (index < s_app_count) {
        return &s_apps[index];
    }
    return NULL;
}

const os_app_desc_t * os_app_find_by_id(const char *id)
{
    if (!id) return NULL;
    for (uint16_t i = 0; i < s_app_count; i++) {
        if (strcmp(s_apps[i].id, id) == 0) {
            return &s_apps[i];
        }
    }
    return NULL;
}

/* ============================================================================
 * Quick Settings Tile Registry Implementation
 * ============================================================================ */

bool os_qs_tile_register(const os_qs_tile_desc_t *tile)
{
    if (!tile || !tile->id || !tile->name) return false;
    if (s_tile_count >= OS_MAX_QS_TILES) {
        OS_LOGW(TAG, "QS Tile registry full! Cannot register '%s'", tile->id);
        return false;
    }

    /* Check if already registered */
    for (uint8_t i = 0; i < s_tile_count; i++) {
        if (strcmp(s_tiles[i].id, tile->id) == 0) {
            s_tiles[i] = *tile;
            return true;
        }
    }

    /* Insert ordered by priority */
    uint8_t insert_idx = s_tile_count;
    for (uint8_t i = 0; i < s_tile_count; i++) {
        if (tile->priority < s_tiles[i].priority) {
            insert_idx = i;
            break;
        }
    }

    for (uint8_t i = s_tile_count; i > insert_idx; i--) {
        s_tiles[i] = s_tiles[i - 1];
    }

    s_tiles[insert_idx] = *tile;
    s_tile_count++;

    OS_LOGD(TAG, "Registered QS tile '%s' ('%s') at index %u", tile->id, tile->name, insert_idx);
    return true;
}

uint8_t os_qs_tile_get_count(void)
{
    return s_tile_count;
}

const os_qs_tile_desc_t * os_qs_tile_get_by_index(uint8_t index)
{
    if (index < s_tile_count) {
        return &s_tiles[index];
    }
    return NULL;
}

const os_qs_tile_desc_t * os_qs_tile_find_by_id(const char *id)
{
    if (!id) return NULL;
    for (uint8_t i = 0; i < s_tile_count; i++) {
        if (strcmp(s_tiles[i].id, id) == 0) {
            return &s_tiles[i];
        }
    }
    return NULL;
}
