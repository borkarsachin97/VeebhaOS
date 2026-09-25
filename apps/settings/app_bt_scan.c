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

#include "app_bt_scan.h"
#include "veebha_connectivity.h"
#include "veebha_templates.h"
#include "veebha_win_mgr.h"
#include "veebha_log.h"
#include <stdio.h>
#include <string.h>

#define TAG "APP_BT_SCAN"

static os_bt_device_t s_discovered[BT_MAX_DISCOVERED_DEVICES];
static uint16_t       s_discovered_count = 0;
static tpl_list_item_t s_scan_items[BT_MAX_DISCOVERED_DEVICES + 1];
static char           s_scan_subtexts[BT_MAX_DISCOVERED_DEVICES][64];

static char           s_pairing_mac[18] = {0};
static char           s_pairing_name[32] = {0};
static char           s_dlg_msg[128] = {0};
static lv_timer_t    *s_scan_timer = NULL;

static void scan_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    uint16_t prev_count = s_discovered_count;
    s_discovered_count = connectivity_bt_get_discovered_devices(s_discovered, BT_MAX_DISCOVERED_DEVICES);

    if (s_discovered_count != prev_count) {
        OS_LOGI(TAG, "Bluetooth Inquiry discovered updated devices (%u -> %u) - refreshing view",
                prev_count, s_discovered_count);
        if (s_scan_timer) {
            lv_timer_del(s_scan_timer);
            s_scan_timer = NULL;
        }
        win_mgr_pop();
        app_bt_scan_open();
    }
}

static void on_pair_confirm(void)
{
    if (s_scan_timer) {
        lv_timer_del(s_scan_timer);
        s_scan_timer = NULL;
    }
    OS_LOGI(TAG, "Confirming pair with '%s' (%s)", s_pairing_name, s_pairing_mac);
    connectivity_bt_pair_device(s_pairing_mac);
    tpl_dialog_close();
    win_mgr_pop(); /* Return to Bluetooth Main Menu */
}

static void on_pair_cancel(void)
{
    OS_LOGI(TAG, "Pairing cancelled");
    tpl_dialog_close();
}

static void on_scan_item_select(uint16_t index)
{
    if (index >= s_discovered_count) return;

    os_bt_device_t *dev = &s_discovered[index];
    strncpy(s_pairing_mac, dev->mac, sizeof(s_pairing_mac) - 1);
    s_pairing_mac[sizeof(s_pairing_mac) - 1] = '\0';
    strncpy(s_pairing_name, dev->name, sizeof(s_pairing_name) - 1);
    s_pairing_name[sizeof(s_pairing_name) - 1] = '\0';

    snprintf(s_dlg_msg, sizeof(s_dlg_msg),
             "Pair with %s?\nPasskey: 582910", dev->name);

    static tpl_dialog_desc_t pair_dlg = {
        .title = "Pairing Request",
        .icon = LV_SYMBOL_BLUETOOTH,
        .message = s_dlg_msg,
        .lsk_label = "Pair",
        .rsk_label = "Cancel",
        .on_confirm = on_pair_confirm,
        .on_cancel = on_pair_cancel
    };

    tpl_dialog_show(&pair_dlg);
}

static void on_scan_back(void)
{
    if (s_scan_timer) {
        lv_timer_del(s_scan_timer);
        s_scan_timer = NULL;
    }
    connectivity_bt_stop_scan();
    win_mgr_pop();
}

void app_bt_scan_open(void)
{
    connectivity_bt_start_scan();

    s_discovered_count = connectivity_bt_get_discovered_devices(s_discovered, BT_MAX_DISCOVERED_DEVICES);

    if (s_discovered_count == 0) {
        s_scan_items[0].icon = LV_SYMBOL_REFRESH;
        s_scan_items[0].title = "No Devices Found";
        s_scan_items[0].subtext = "Searching for nearby devices...";
    } else {
        for (uint16_t i = 0; i < s_discovered_count; i++) {
            os_bt_device_t *d = &s_discovered[i];
            const char *icon = LV_SYMBOL_SETTINGS;
            const char *type_name = "Device";

            if (d->type == BT_DEV_AUDIO) {
                icon = LV_SYMBOL_AUDIO;
                type_name = "Audio Headset";
            } else if (d->type == BT_DEV_COMPUTER) {
                icon = LV_SYMBOL_SETTINGS;
                type_name = "Computer / Laptop";
            } else if (d->type == BT_DEV_PHONE) {
                icon = LV_SYMBOL_CALL;
                type_name = "Phone";
            }

            s_scan_items[i].icon = icon;
            s_scan_items[i].title = d->name;

            snprintf(s_scan_subtexts[i], sizeof(s_scan_subtexts[0]),
                     "%s · %d dBm", type_name, d->rssi);
            s_scan_items[i].subtext = s_scan_subtexts[i];
        }
    }

    tpl_list_view_t view_desc = {
        .title = "Search Devices",
        .items = s_scan_items,
        .count = (s_discovered_count == 0) ? 1 : s_discovered_count,
        .on_select = (s_discovered_count == 0) ? NULL : on_scan_item_select,
        .on_back = on_scan_back,
        .lsk_label = (s_discovered_count == 0) ? "" : "Pair",
        .rsk_label = "Back"
    };

    lv_obj_t *screen = tpl_list_create(&view_desc);
    if (screen) {
        win_mgr_push(screen, view_desc.lsk_label, tpl_list_default_lsk, "Back", on_scan_back);
        if (!s_scan_timer) {
            s_scan_timer = lv_timer_create(scan_timer_cb, 500, NULL);
        }
        OS_LOGI(TAG, "Bluetooth Inquiry Scan View opened with %u devices", s_discovered_count);
    }
}
