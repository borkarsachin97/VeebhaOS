/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "app_funzone.h"
#include "sdk/include/veebha_vapp.h"
#include "sdk/core/vapp_loader.h"
#include "sdk/include/veebha_templates.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_log.h"
#include "sdk/include/veebha_i18n.h"
#include "sdk/vfs/os_vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "APP_FUNZONE"

#define MAX_STORE_PACKAGES 64

static vapp_package_t s_packages[MAX_STORE_PACKAGES];
static size_t         s_package_count = 0;
static tpl_list_item_t s_list_items[MAX_STORE_PACKAGES];
static char           s_subtexts[MAX_STORE_PACKAGES][64];

static void on_funzone_item_select(uint16_t index)
{
    if (index >= s_package_count) return;

    const vapp_package_t *pkg = &s_packages[index];
    OS_LOGI(TAG, "Selected package %u: '%s' (%s)", index, pkg->header.name,
            vapp_type_to_string((vapp_type_t)pkg->header.app_type));

    /* Launch application directly into Window Manager */
    vapp_loader_launch(pkg);
}

void app_funzone_init(void)
{
    OS_LOGI(TAG, "VeebhaOS Fun Zone store module initialized");
    vapp_loader_init();
}

void app_funzone_open(void)
{
    app_funzone_init();

    /* Scan /vapps directory */
    vapp_loader_scan_dir(s_packages, MAX_STORE_PACKAGES, &s_package_count);

    if (s_package_count == 0) {
        static tpl_dialog_desc_t empty_dlg = {
            .title = "Fun Zone",
            .message = "No .vapp packages found in /vapps",
            .icon = LV_SYMBOL_WARNING,
            .on_confirm = NULL,
            .on_cancel = NULL,
            .lsk_label = "OK",
            .rsk_label = "Back"
        };
        tpl_dialog_show(&empty_dlg);
        return;
    }

    for (size_t i = 0; i < s_package_count; i++) {
        const vapp_package_t *pkg = &s_packages[i];
        s_list_items[i].icon = pkg->header.icon_symbol[0] ? pkg->header.icon_symbol : LV_SYMBOL_PLAY;
        s_list_items[i].title = pkg->header.name;

        char size_str[16];
        vfs_format_size(pkg->file_size ? pkg->file_size : pkg->header.req_heap_bytes, size_str, sizeof(size_str));

        snprintf(s_subtexts[i], sizeof(s_subtexts[i]), "[%s] v%s • %s",
                 vapp_type_to_string((vapp_type_t)pkg->header.app_type),
                 pkg->header.version,
                 size_str);
        s_list_items[i].subtext = s_subtexts[i];
    }

    tpl_list_view_t desc = {
        .title = "Fun Zone",
        .items = s_list_items,
        .count = (uint16_t)s_package_count,
        .on_select = on_funzone_item_select,
        .on_back = NULL, /* default win_mgr_pop() */
        .lsk_label = "Open",
        .rsk_label = "Back"
    };

    lv_obj_t *scr = tpl_list_create(&desc);
    if (scr) {
        win_mgr_push(scr, "Open", tpl_list_default_lsk, "Back", tpl_list_default_rsk);
        OS_LOGI(TAG, "VeebhaOS Fun Zone store screen opened (%u apps)", (unsigned int)s_package_count);
    }
}
