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

#include "app_bt.h"
#include "app_bt_scan.h"
#include "veebha_connectivity.h"
#include "veebha_templates.h"
#include "veebha_win_mgr.h"
#include "veebha_status_bar.h"
#include "veebha_log.h"
#include <stdio.h>
#include <string.h>

#define TAG "APP_BT"

static os_bt_device_t s_paired_devices[BT_MAX_PAIRED_DEVICES];
static uint16_t       s_paired_count = 0;

static tpl_list_item_t s_bt_items[4 + BT_MAX_PAIRED_DEVICES];
static char           s_bt_subtexts[4 + BT_MAX_PAIRED_DEVICES][64];
static char           s_rename_buf[32] = {0};

static void update_status_bar_indicator(void)
{
    win_mgr_entry_t *top = win_mgr_get_top();
    if (top && top->screen && lv_obj_is_valid(top->screen)) {
        if (lv_obj_get_child_count(top->screen) > 0) {
            lv_obj_t *bar = lv_obj_get_child(top->screen, 0);
            status_bar_set_indicators(bar, connectivity_bt_is_enabled(), false);
        }
    }
}

static void on_rename_saved(const char *text)
{
    if (text && text[0] != '\0') {
        connectivity_bt_set_device_name(text);
    }
    win_mgr_pop(); /* Pop editor */
    win_mgr_pop(); /* Pop old BT menu */
    app_bt_open(); /* Reload BT menu */
}

static void open_device_rename(void)
{
    strncpy(s_rename_buf, connectivity_bt_get_device_name(), sizeof(s_rename_buf) - 1);
    s_rename_buf[sizeof(s_rename_buf) - 1] = '\0';

    tpl_editor_desc_t desc = {
        .title = "Device Name",
        .buffer = s_rename_buf,
        .max_len = sizeof(s_rename_buf) - 1,
        .on_save = on_rename_saved,
        .on_cancel = NULL,
        .lsk_label = "Save",
        .rsk_label = "Cancel"
    };

    lv_obj_t *scr = tpl_editor_create(&desc);
    if (scr) {
        win_mgr_push(scr, "Save", tpl_editor_default_lsk, "Cancel", tpl_editor_default_rsk);
    }
}

static void on_bt_menu_select(uint16_t index)
{
    bool enabled = connectivity_bt_is_enabled();

    if (index == 0) {
        /* Toggle Bluetooth ON / OFF */
        connectivity_bt_set_enabled(!enabled);
        update_status_bar_indicator();
        win_mgr_pop();
        app_bt_open();
        return;
    }

    if (!enabled) return;

    if (index == 1) {
        /* Toggle Visibility */
        connectivity_bt_set_visible(!connectivity_bt_is_visible());
        win_mgr_pop();
        app_bt_open();
    } else if (index == 2) {
        /* Rename Device */
        open_device_rename();
    } else if (index == 3) {
        /* Search for Devices */
        app_bt_scan_open();
    } else {
        /* Paired Device Item */
        uint16_t dev_idx = index - 4;
        if (dev_idx < s_paired_count) {
            os_bt_device_t *dev = &s_paired_devices[dev_idx];
            if (dev->is_connected) {
                connectivity_bt_disconnect_device(dev->mac);
            } else {
                connectivity_bt_connect_device(dev->mac);
            }
            win_mgr_pop();
            app_bt_open();
        }
    }
}

void app_bt_init(void)
{
    OS_LOGI(TAG, "Bluetooth Manager application initialized");
}

void app_bt_open(void)
{
    bool enabled = connectivity_bt_is_enabled();
    uint16_t row = 0;

    /* Row 0: Bluetooth Radio State */
    s_bt_items[row].icon = LV_SYMBOL_BLUETOOTH;
    s_bt_items[row].title = "Bluetooth";
    snprintf(s_bt_subtexts[row], sizeof(s_bt_subtexts[0]), "%s", enabled ? "ON" : "OFF");
    s_bt_items[row].subtext = s_bt_subtexts[row];
    row++;

    if (enabled) {
        /* Row 1: Visibility */
        bool visible = connectivity_bt_is_visible();
        s_bt_items[row].icon = LV_SYMBOL_EYE_OPEN;
        s_bt_items[row].title = "Visibility";
        snprintf(s_bt_subtexts[row], sizeof(s_bt_subtexts[0]), "%s", visible ? "Shown" : "Hidden");
        s_bt_items[row].subtext = s_bt_subtexts[row];
        row++;

        /* Row 2: Device Name */
        s_bt_items[row].icon = LV_SYMBOL_EDIT;
        s_bt_items[row].title = "Device Name";
        snprintf(s_bt_subtexts[row], sizeof(s_bt_subtexts[0]), "%s", connectivity_bt_get_device_name());
        s_bt_items[row].subtext = s_bt_subtexts[row];
        row++;

        /* Row 3: Search for Devices */
        s_bt_items[row].icon = LV_SYMBOL_PLUS;
        s_bt_items[row].title = "+ Search for Devices";
        s_bt_items[row].subtext = "Discover nearby accessories";
        row++;

        /* Rows 4+: Paired Devices */
        s_paired_count = connectivity_bt_get_paired_devices(s_paired_devices, BT_MAX_PAIRED_DEVICES);
        for (uint16_t i = 0; i < s_paired_count; i++) {
            os_bt_device_t *d = &s_paired_devices[i];
            const char *icon = LV_SYMBOL_SETTINGS;

            if (d->type == BT_DEV_AUDIO) {
                icon = LV_SYMBOL_AUDIO;
            } else if (d->type == BT_DEV_COMPUTER) {
                icon = LV_SYMBOL_SETTINGS;
            } else if (d->type == BT_DEV_PHONE) {
                icon = LV_SYMBOL_CALL;
            }

            s_bt_items[row].icon = icon;
            s_bt_items[row].title = d->name;

            snprintf(s_bt_subtexts[row], sizeof(s_bt_subtexts[0]),
                     "%s · %s", d->is_connected ? "Connected" : "Paired", d->mac);
            s_bt_items[row].subtext = s_bt_subtexts[row];
            row++;
        }
    }

    tpl_list_view_t view_desc = {
        .title = "Bluetooth",
        .items = s_bt_items,
        .count = row,
        .on_select = on_bt_menu_select,
        .on_back = NULL, /* Default win_mgr_pop */
        .lsk_label = "Select",
        .rsk_label = "Back"
    };

    lv_obj_t *screen = tpl_list_create(&view_desc);
    if (screen) {
        win_mgr_push(screen, "Select", tpl_list_default_lsk, "Back", tpl_list_default_rsk);
        update_status_bar_indicator();
        OS_LOGI(TAG, "Bluetooth Manager opened (Enabled: %d, Rows: %u)", (int)enabled, row);
    }
}
