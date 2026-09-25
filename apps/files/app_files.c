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

#include "app_files.h"
#include "sdk/vfs/os_vfs.h"
#include "sdk/include/app_registry.h"
#include "sdk/include/veebha_templates.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_log.h"
#include "sdk/include/veebha_hardware.h"
#include "sdk/core/vapp_loader.h"
#include "sdk/include/veebha_i18n.h"
#include "apps/music/app_music.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "APP_FILES"

#define MAX_DIR_ENTRIES 16

typedef struct {
    char         path[64];
    bool         is_top_category;
    uint16_t     entry_count;
    vfs_dirent_t entries[MAX_DIR_ENTRIES];
} files_dir_ctx_t;

static files_dir_ctx_t s_dir_contexts[WIN_MGR_MAX_DEPTH];

static void on_dialog_dismiss(void)
{
    tpl_dialog_close();
}

static void on_root_select(uint16_t index)
{
    if (index == 0) {
        app_files_open_path("/internal");
    } else if (index == 1) {
        app_files_open_path("/sdcard");
    } else if (index == 2) {
        app_files_open_path("/extsd");
    } else if (index == 3) {
        app_files_open_path("/other");
    }
}

static void on_files_select(uint16_t index)
{
    uint8_t depth = win_mgr_get_depth();
    if (depth == 0) return;
    uint8_t ctx_idx = depth - 1;
    if (ctx_idx >= WIN_MGR_MAX_DEPTH) return;

    files_dir_ctx_t *ctx = &s_dir_contexts[ctx_idx];

    /* Row 0 is [.. Parent Folder] / [.. Storage Roots] */
    if (index == 0) {
        OS_LOGI(TAG, "Navigating up from '%s'", ctx->path);
        win_mgr_pop();
        return;
    }

    uint16_t entry_idx = index - 1;
    if (entry_idx >= ctx->entry_count) return;

    const vfs_dirent_t *entry = &ctx->entries[entry_idx];
    char target_path[128];
    snprintf(target_path, sizeof(target_path), "%s/%s", ctx->path, entry->name);

    if (entry->type == VFS_NODE_DIR) {
        OS_LOGI(TAG, "Opening directory: %s", target_path);
        app_files_open_path(target_path);
    } else {
        if (entry->mime == VFS_MIME_AUDIO) {
            OS_LOGI(TAG, "Audio file selected: %s -> launching Walkman Player", target_path);
            app_music_play_file(target_path);
        } else if (entry->mime == VFS_MIME_TEXT) {
            char size_str[16];
            vfs_format_size(entry->size_bytes, size_str, sizeof(size_str));
            static char msg_buf[128];
            snprintf(msg_buf, sizeof(msg_buf), "File: %s\nSize: %s\nType: Text Document", entry->name, size_str);

            static tpl_dialog_desc_t text_dlg = {
                .title = "Text File",
                .icon = LV_SYMBOL_FILE,
                .on_confirm = on_dialog_dismiss,
                .on_cancel = on_dialog_dismiss,
                .lsk_label = "OK",
                .rsk_label = "Back"
            };
            text_dlg.message = msg_buf;
            tpl_dialog_show(&text_dlg);
        } else if (entry->mime == VFS_MIME_IMAGE) {
            char size_str[16];
            vfs_format_size(entry->size_bytes, size_str, sizeof(size_str));
            static char img_msg[128];
            snprintf(img_msg, sizeof(img_msg), "File: %s\nSize: %s\nType: Image Asset", entry->name, size_str);

            static tpl_dialog_desc_t img_dlg = {
                .title = "Image File",
                .icon = LV_SYMBOL_IMAGE,
                .on_confirm = on_dialog_dismiss,
                .on_cancel = on_dialog_dismiss,
                .lsk_label = "OK",
                .rsk_label = "Back"
            };
            img_dlg.message = img_msg;
            tpl_dialog_show(&img_dlg);
        } else if (entry->mime == VFS_MIME_VAPP) {
            vapp_package_t pkg;
            if (vapp_loader_read_header(target_path, &pkg)) {
                vapp_loader_launch(&pkg);
            }
        } else {
            char size_str[16];
            vfs_format_size(entry->size_bytes, size_str, sizeof(size_str));
            static char gen_msg[128];
            snprintf(gen_msg, sizeof(gen_msg), "File: %s\nSize: %s\nType: Binary File", entry->name, size_str);

            static tpl_dialog_desc_t gen_dlg = {
                .title = "File Info",
                .icon = LV_SYMBOL_FILE,
                .on_confirm = on_dialog_dismiss,
                .on_cancel = on_dialog_dismiss,
                .lsk_label = "OK",
                .rsk_label = "Back"
            };
            gen_dlg.message = gen_msg;
            tpl_dialog_show(&gen_dlg);
        }
    }
}

static void on_files_back(void)
{
    win_mgr_pop();
}

void app_files_open_path(const char *path)
{
    const char *norm_path = (path && path[0]) ? path : "/internal";
    uint8_t target_idx = win_mgr_get_depth();
    if (target_idx >= WIN_MGR_MAX_DEPTH) {
        OS_LOGW(TAG, "Navigation stack depth exceeded");
        return;
    }

    files_dir_ctx_t *ctx = &s_dir_contexts[target_idx];
    memset(ctx, 0, sizeof(files_dir_ctx_t));
    strncpy(ctx->path, norm_path, sizeof(ctx->path) - 1);

    ctx->is_top_category = (strcasecmp(norm_path, "/internal") == 0 ||
                            strcasecmp(norm_path, "/sdcard") == 0 ||
                            strcasecmp(norm_path, "/extsd") == 0 ||
                            strcasecmp(norm_path, "/other") == 0);

    /* Open and read directory entries */
    void *dir = NULL;
    if (vfs_opendir(norm_path, &dir)) {
        while (ctx->entry_count < MAX_DIR_ENTRIES && vfs_readdir(dir, &ctx->entries[ctx->entry_count])) {
            ctx->entry_count++;
        }
        vfs_closedir(dir);
    } else {
        OS_LOGW(TAG, "Failed to open directory: %s", norm_path);
        return;
    }

    /* Build tpl_list declarative items */
    static tpl_list_item_t list_items[MAX_DIR_ENTRIES + 1];
    static char item_subtexts[MAX_DIR_ENTRIES + 1][16];
    uint16_t total_items = 0;

    /* Row 0: Navigation up */
    list_items[0].icon = LV_SYMBOL_DIRECTORY;
    list_items[0].title = ctx->is_top_category ? ".. Storage Roots" : ".. Parent Folder";
    list_items[0].subtext = "Folder";
    total_items++;

    for (uint16_t i = 0; i < ctx->entry_count; i++) {
        const vfs_dirent_t *ent = &ctx->entries[i];
        uint16_t curr_pos = total_items;

        if (ent->type == VFS_NODE_DIR) {
            list_items[curr_pos].icon = LV_SYMBOL_DIRECTORY;
            list_items[curr_pos].title = ent->name;
            strncpy(item_subtexts[curr_pos], "Folder", sizeof(item_subtexts[curr_pos]) - 1);
            list_items[curr_pos].subtext = item_subtexts[curr_pos];
        } else {
            switch (ent->mime) {
            case VFS_MIME_AUDIO:
                list_items[curr_pos].icon = LV_SYMBOL_AUDIO;
                break;
            case VFS_MIME_IMAGE:
                list_items[curr_pos].icon = LV_SYMBOL_IMAGE;
                break;
            case VFS_MIME_VAPP:
                list_items[curr_pos].icon = LV_SYMBOL_PLUS;
                break;
            case VFS_MIME_TEXT:
            default:
                list_items[curr_pos].icon = LV_SYMBOL_FILE;
                break;
            }
            list_items[curr_pos].title = ent->name;
            vfs_format_size(ent->size_bytes, item_subtexts[curr_pos], sizeof(item_subtexts[curr_pos]));
            list_items[curr_pos].subtext = item_subtexts[curr_pos];
        }
        total_items++;
    }

    /* Format Header Strip title (e.g. INTERNAL: /, SD CARD: /Music/, EXT SD: /) */
    static char header_title[WIN_MGR_LABEL_MAX];
    if (strncasecmp(norm_path, "/internal", 9) == 0) {
        const char *sub = norm_path + 9;
        snprintf(header_title, sizeof(header_title), "INTERNAL: %s", (sub[0] != '\0') ? sub : "/");
    } else if (strncasecmp(norm_path, "/sdcard", 7) == 0) {
        const char *sub = norm_path + 7;
        snprintf(header_title, sizeof(header_title), "SD CARD: %s", (sub[0] != '\0') ? sub : "/");
    } else if (strncasecmp(norm_path, "/extsd", 6) == 0) {
        const char *sub = norm_path + 6;
        snprintf(header_title, sizeof(header_title), "EXT SD: %s", (sub[0] != '\0') ? sub : "/");
    } else if (strncasecmp(norm_path, "/other", 6) == 0) {
        const char *sub = norm_path + 6;
        snprintf(header_title, sizeof(header_title), "OTHER: %s", (sub[0] != '\0') ? sub : "/");
    } else {
        snprintf(header_title, sizeof(header_title), "FILES: %s", norm_path);
    }

    tpl_list_view_t view_desc = {
        .title = header_title,
        .items = list_items,
        .count = total_items,
        .on_select = on_files_select,
        .on_back = on_files_back,
        .lsk_label = "Select",
        .rsk_label = "Back"
    };

    lv_obj_t *scr = tpl_list_create(&view_desc);
    if (scr) {
        win_mgr_push(scr, "Select", tpl_list_default_lsk, "Back", on_files_back);
        OS_LOGI(TAG, "File Manager opened '%s' with %u items", header_title, total_items);
    }
}

void app_files_open(void)
{
    static tpl_list_item_t root_items[4];
    static char extsd_subtext[32];

    /* 1. Internal Storage */
    root_items[0].icon = LV_SYMBOL_HOME;
    root_items[0].title = "Internal Storage";
    root_items[0].subtext = "Flash Memory · System & Apps";

    /* 2. SD Card (Built-in) */
    root_items[1].icon = LV_SYMBOL_DIRECTORY;
    root_items[1].title = "SD Card";
    root_items[1].subtext = "Phone Storage · Media";

    /* 3. External SD Card (SDMMC Hardware Card) */
    root_items[2].icon = LV_SYMBOL_SD_CARD;
    root_items[2].title = "External SD Card";
    if (veebha_hw_sdcard_present()) {
        uint32_t cap_mb = veebha_hw_sdcard_get_capacity_mb();
        if (cap_mb >= 1024) {
            snprintf(extsd_subtext, sizeof(extsd_subtext), "Ready · %u GB", (unsigned int)(cap_mb / 1024));
        } else if (cap_mb > 0) {
            snprintf(extsd_subtext, sizeof(extsd_subtext), "Ready · %u MB", (unsigned int)cap_mb);
        } else {
            snprintf(extsd_subtext, sizeof(extsd_subtext), "Ready · SDMMC Card");
        }
    } else {
        snprintf(extsd_subtext, sizeof(extsd_subtext), "No Card Inserted");
    }
    root_items[2].subtext = extsd_subtext;

    /* 4. Other Storage */
    root_items[3].icon = LV_SYMBOL_SETTINGS;
    root_items[3].title = "Other Storage";
    root_items[3].subtext = "NVRAM · Partitions · OTG";

    tpl_list_view_t root_view = {
        .title = "File Manager",
        .items = root_items,
        .count = 4,
        .on_select = on_root_select,
        .on_back = on_files_back,
        .lsk_label = "Select",
        .rsk_label = "Back"
    };

    lv_obj_t *scr = tpl_list_create(&root_view);
    if (scr) {
        win_mgr_push(scr, "Select", tpl_list_default_lsk, "Back", on_files_back);
        OS_LOGI(TAG, "File Manager Root Categories opened (External SD: %s)", extsd_subtext);
    }
}

void app_files_init(void)
{
    os_app_desc_t files_app = {
        .id = "files",
        .name = "Files",
        .icon = LV_SYMBOL_DIRECTORY,
        .launch_cb = app_files_open,
        .priority = 80
    };
    os_app_register(&files_app);
    OS_LOGI(TAG, "File Manager application initialized");
}
