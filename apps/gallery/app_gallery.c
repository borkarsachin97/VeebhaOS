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

#include "app_gallery.h"
#include "sdk/vfs/os_vfs.h"
#include "sdk/include/veebha_templates.h"
#include "sdk/include/veebha_status_bar.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "APP_GALLERY"

#define MAX_GALLERY_PHOTOS 16

typedef struct {
    char     name[64];
    uint32_t size_bytes;
} gallery_photo_item_t;

static gallery_photo_item_t s_photos[MAX_GALLERY_PHOTOS];
static tpl_list_item_t      s_list_items[MAX_GALLERY_PHOTOS];
static uint16_t             s_photo_count = 0;

typedef struct {
    lv_obj_t          *softkey_bar;
    lv_obj_t          *first_item;
    veebha_view_type_t view_type;
    char               title[WIN_MGR_LABEL_MAX];
    os_fullscreen_mode_t fullscreen_mode;
    bool               show_battery_hud;
    bool               keep_alive;

    char               filename[64];
    uint32_t           size_bytes;
} viewer_screen_data_t;

static void on_info_dialog_close(void)
{
    tpl_dialog_close();
}

static void on_viewer_info_action(void)
{
    lv_obj_t *top = lv_scr_act();
    viewer_screen_data_t *data = (viewer_screen_data_t *)lv_obj_get_user_data(top);
    if (!data) return;

    static char msg_buf[128];
    snprintf(msg_buf, sizeof(msg_buf),
             "File: %s\nSize: %u KB\nRes: 176x220\nPath: /sdcard/Photos",
             data->filename, (data->size_bytes + 1023) / 1024);

    static tpl_dialog_desc_t dlg = {
        .title = "Image Details",
        .icon = LV_SYMBOL_IMAGE,
        .message = msg_buf,
        .lsk_label = "OK",
        .rsk_label = "Close",
        .on_confirm = on_info_dialog_close,
        .on_cancel = on_info_dialog_close
    };
    tpl_dialog_show(&dlg);
}

static void on_viewer_back_action(void)
{
    win_mgr_pop();
}

static void on_viewer_key_cb(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    if (key == '0' || key == LV_KEY_ESC || key == LV_KEY_BACKSPACE) {
        on_viewer_back_action();
    }
}

static void on_viewer_delete_cb(lv_event_t *e)
{
    lv_obj_t *scr = lv_event_get_target(e);
    viewer_screen_data_t *data = (viewer_screen_data_t *)lv_obj_get_user_data(scr);
    if (data) {
        free(data);
        lv_obj_set_user_data(scr, NULL);
    }
}

void app_gallery_view_photo(const char *filename, uint32_t size_bytes)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, 176, 220);
    lv_obj_set_style_bg_color(screen, theme_get()->bg_color, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    viewer_screen_data_t *data = (viewer_screen_data_t *)calloc(1, sizeof(viewer_screen_data_t));
    if (!data) {
        lv_obj_del(screen);
        return;
    }

    data->view_type = VEEBHA_VIEW_TYPE_GENERIC;
    strncpy(data->title, filename ? filename : "Photo", sizeof(data->title) - 1);
    data->fullscreen_mode = OS_FULLSCREEN_FULL;
    data->show_battery_hud = false;
    if (filename) {
        strncpy(data->filename, filename, sizeof(data->filename) - 1);
    }
    data->size_bytes = size_bytes;

    lv_obj_set_user_data(screen, data);
    lv_obj_add_event_cb(screen, on_viewer_delete_cb, LV_EVENT_DELETE, NULL);

    /* 1. Zone A: Fixed 18px Status Bar (hidden in full mode) */
    status_bar_create(screen, NULL);

    /* 2. Header Strip (18px) - hidden in full screen mode */
    lv_obj_t *hdr = lv_obj_create(screen);
    lv_obj_set_size(hdr, lv_pct(100), 18);
    lv_obj_set_style_bg_color(hdr, theme_get()->card_color, 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(hdr, theme_is_light_mode() ? lv_color_hex(0xE2E8F0) : lv_color_hex(0x282C35), 0);
    lv_obj_set_style_border_width(hdr, 1, 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(hdr, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *hlbl = lv_label_create(hdr);
    lv_label_set_text(hlbl, filename ? filename : "PHOTO");
    lv_obj_set_style_text_color(hlbl, theme_get()->accent, 0);
    lv_obj_set_style_text_font(hlbl, &lv_font_montserrat_10, 0);
    lv_label_set_long_mode(hlbl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(hlbl, 160);
    lv_obj_set_style_text_align(hlbl, LV_TEXT_ALIGN_CENTER, 0);

    /* 3. Viewport (Photo Preview Frame) */
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_size(content, lv_pct(100), 0);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 4, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(content, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(content, on_viewer_key_cb, LV_EVENT_KEY, NULL);

    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, on_viewer_key_cb, LV_EVENT_KEY, NULL);

    /* Picture frame box */
    lv_obj_t *frame = lv_obj_create(content);
    lv_obj_set_size(frame, 168, 200);
    lv_obj_set_style_bg_color(frame, lv_color_hex(0x161B22), 0);
    lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(frame, theme_get()->accent, 0);
    lv_obj_set_style_border_width(frame, 1, 0);
    lv_obj_set_style_radius(frame, 4, 0);
    lv_obj_set_style_pad_all(frame, 0, 0);
    lv_obj_set_flex_flow(frame, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(frame, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(frame, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *icon_lbl = lv_label_create(frame);
    lv_label_set_text(icon_lbl, LV_SYMBOL_IMAGE);
    lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(icon_lbl, theme_get()->accent, 0);

    lv_obj_t *caption_lbl = lv_label_create(frame);
    lv_label_set_text(caption_lbl, "176 x 220 RAW\nFull Color");
    lv_obj_set_style_text_font(caption_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(caption_lbl, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_align(caption_lbl, LV_TEXT_ALIGN_CENTER, 0);

    lv_group_t *grp = win_mgr_get_group();
    if (grp) {
        lv_group_add_obj(grp, content);
    }
    data->first_item = content;

    /* 4. Bottom Softkey Bar */
    data->softkey_bar = softkey_bar_create(screen, "Info", "Back");
    win_mgr_push(screen, "Info", on_viewer_info_action, "Back", on_viewer_back_action);
    win_mgr_set_fullscreen_mode(screen, OS_FULLSCREEN_FULL, false);

    if (grp) {
        lv_group_set_editing(grp, true);
    }
}

static void on_gallery_item_select(uint16_t index)
{
    if (index >= s_photo_count) return;
    OS_LOGI(TAG, "Selected photo: %s (%u bytes)", s_photos[index].name, s_photos[index].size_bytes);
    app_gallery_view_photo(s_photos[index].name, s_photos[index].size_bytes);
}

static void load_gallery_photos(void)
{
    s_photo_count = 0;
    void *dir = NULL;
    if (vfs_opendir("/sdcard/Photos", &dir) && dir != NULL) {
        vfs_dirent_t ent;
        while (vfs_readdir(dir, &ent) && s_photo_count < MAX_GALLERY_PHOTOS) {
            if (ent.type == VFS_NODE_FILE) {
                strncpy(s_photos[s_photo_count].name, ent.name, sizeof(s_photos[s_photo_count].name) - 1);
                s_photos[s_photo_count].size_bytes = ent.size_bytes;

                s_list_items[s_photo_count].icon = LV_SYMBOL_IMAGE;
                s_list_items[s_photo_count].title = s_photos[s_photo_count].name;

                static char s_subtexts[MAX_GALLERY_PHOTOS][32];
                snprintf(s_subtexts[s_photo_count], sizeof(s_subtexts[0]),
                         "%u KB | Photo", (ent.size_bytes + 1023) / 1024);
                s_list_items[s_photo_count].subtext = s_subtexts[s_photo_count];

                s_photo_count++;
            }
        }
        vfs_closedir(dir);
    }

    /* Fallback default if directory empty */
    if (s_photo_count == 0) {
        strncpy(s_photos[0].name, "Wallpaper_Cyber.raw", sizeof(s_photos[0].name) - 1);
        s_photos[0].size_bytes = 78848;
        s_list_items[0].icon = LV_SYMBOL_IMAGE;
        s_list_items[0].title = s_photos[0].name;
        s_list_items[0].subtext = "78 KB | Photo";
        s_photo_count = 1;
    }
}

void app_gallery_init(void)
{
    OS_LOGI(TAG, "Photo Gallery module initialized");
}

void app_gallery_open(void)
{
    load_gallery_photos();

    tpl_list_view_t desc = {
        .title = "PHOTOS",
        .items = s_list_items,
        .count = s_photo_count,
        .on_select = on_gallery_item_select,
        .on_back = NULL,
        .lsk_label = "View",
        .rsk_label = "Back"
    };

    lv_obj_t *scr = tpl_list_create(&desc);
    if (scr) {
        win_mgr_push(scr, "View", tpl_list_default_lsk, "Back", tpl_list_default_rsk);
    }
}
