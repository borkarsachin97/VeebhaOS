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

#include "app_tethering.h"
#include "veebha_connectivity.h"
#include "veebha_templates.h"
#include "veebha_win_mgr.h"
#include "veebha_log.h"
#include <stdio.h>
#include <string.h>

#define TAG "APP_TETHERING"

static tpl_list_item_t s_tether_items[2];
static char           s_tether_subtexts[2][64];

static void on_tethering_select(uint16_t index)
{
    if (index == 0) {
        /* USB Tethering */
        bool cur = connectivity_tethering_get_usb();
        connectivity_tethering_set_usb(!cur);
        win_mgr_pop();
        app_tethering_open();
    } else if (index == 1) {
        /* Bluetooth Tethering */
        bool cur = connectivity_tethering_get_bt();
        connectivity_tethering_set_bt(!cur);
        win_mgr_pop();
        app_tethering_open();
    }
}

void app_tethering_open(void)
{
    bool usb_tether = connectivity_tethering_get_usb();
    bool bt_tether = connectivity_tethering_get_bt();

    /* Row 0: USB Tethering */
    s_tether_items[0].icon = LV_SYMBOL_USB;
    s_tether_items[0].title = "USB Tethering";
    if (usb_tether) {
        snprintf(s_tether_subtexts[0], sizeof(s_tether_subtexts[0]), "ON · Sharing 4G Modem");
    } else {
        snprintf(s_tether_subtexts[0], sizeof(s_tether_subtexts[0]), "OFF · Not Sharing");
    }
    s_tether_items[0].subtext = s_tether_subtexts[0];

    /* Row 1: Bluetooth Tethering */
    s_tether_items[1].icon = LV_SYMBOL_BLUETOOTH;
    s_tether_items[1].title = "Bluetooth Tethering";
    if (bt_tether) {
        snprintf(s_tether_subtexts[1], sizeof(s_tether_subtexts[1]), "ON · PAN Gateway Active");
    } else {
        snprintf(s_tether_subtexts[1], sizeof(s_tether_subtexts[1]), "OFF · Not Connected");
    }
    s_tether_items[1].subtext = s_tether_subtexts[1];

    tpl_list_view_t view_desc = {
        .title = "Tethering",
        .items = s_tether_items,
        .count = 2,
        .on_select = on_tethering_select,
        .on_back = NULL, /* Default win_mgr_pop */
        .lsk_label = "Toggle",
        .rsk_label = "Back"
    };

    lv_obj_t *screen = tpl_list_create(&view_desc);
    if (screen) {
        win_mgr_push(screen, "Toggle", tpl_list_default_lsk, "Back", tpl_list_default_rsk);
        OS_LOGI(TAG, "Tethering settings menu opened");
    }
}
