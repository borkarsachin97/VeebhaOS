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

#include "app_idle.h"
#include "app_launcher.h"
#include "apps/contacts/app_contacts.h"
#include "apps/common/mock_telephony.h"
#include "sdk/include/veebha_live_pill.h"
#include "sdk/include/veebha_templates.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_status_bar.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_log.h"
#include "sdk/include/veebha_i18n.h"
#include "sdk/text/font_fallback.h"
#include "sdk/core/wallpaper.h"
#include "boards/board_config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TAG "APP_IDLE"
#define LV_OPA_35 ((lv_opa_t)89)

typedef struct {
    lv_obj_t          *softkey_bar;
    lv_obj_t          *first_item;
    veebha_view_type_t view_type;
    char               title[WIN_MGR_LABEL_MAX];
    os_fullscreen_mode_t fullscreen_mode;
    bool               show_battery_hud;
    lv_obj_t          *wp_img;
    lv_obj_t          *scrim_overlay;
    lv_obj_t          *content_layer;
    lv_obj_t          *status_bar;
    lv_obj_t          *clock_lbl;
    lv_obj_t          *date_lbl;
    lv_obj_t          *notif_row;
    lv_obj_t          *sms_pill;
    lv_obj_t          *sms_lbl;
    lv_obj_t          *call_pill;
    lv_obj_t          *call_lbl;
    lv_obj_t          *unified_pill;
    lv_obj_t          *pill_icon_lbl;
    lv_obj_t          *pill_text_lbl;
} app_idle_data_t;

static app_idle_data_t *s_idle_data = NULL;
static lv_obj_t        *s_idle_screen = NULL;
static idle_wallpaper_t s_current_wallpaper = WALLPAPER_DARK;

void app_idle_refresh_wallpaper(void)
{
    if (!s_idle_data || !s_idle_screen) return;

    wallpaper_mode_t mode = wallpaper_get_mode();
    if (mode == WALLPAPER_MODE_IMAGE_BMP) {
        if (s_idle_data->wp_img) {
            lv_image_set_src(s_idle_data->wp_img, wallpaper_get_img_dsc());
            lv_obj_clear_flag(s_idle_data->wp_img, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_idle_data->scrim_overlay) {
            lv_obj_set_style_bg_opa(s_idle_data->scrim_overlay, LV_OPA_35, 0);
        }
        lv_obj_set_style_bg_color(s_idle_screen, lv_color_black(), 0);
    } else {
        if (s_idle_data->wp_img) {
            lv_obj_add_flag(s_idle_data->wp_img, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_idle_data->scrim_overlay) {
            lv_obj_set_style_bg_opa(s_idle_data->scrim_overlay, LV_OPA_TRANSP, 0);
        }
        lv_obj_set_style_bg_color(s_idle_screen, wallpaper_get_solid_color(), 0);
        lv_obj_set_style_bg_grad_dir(s_idle_screen, LV_GRAD_DIR_NONE, 0);
    }
}

void app_idle_set_wallpaper(idle_wallpaper_t wp)
{
    if (wp >= WALLPAPER_COUNT) wp = WALLPAPER_DARK;
    s_current_wallpaper = wp;
    if (wp == WALLPAPER_DARK) {
        wallpaper_set_solid(theme_get()->bg_color);
    } else if (wp == WALLPAPER_CYBER) {
        wallpaper_set_image("/sdcard/Wallpapers/default.bmp");
    } else if (wp == WALLPAPER_SUNSET) {
        wallpaper_set_image("/sdcard/Wallpapers/Abstract.bmp");
    } else if (wp == WALLPAPER_EMERALD) {
        wallpaper_set_image("/sdcard/Wallpapers/Nature.bmp");
    }
}

idle_wallpaper_t app_idle_get_wallpaper(void)
{
    return s_current_wallpaper;
}

static void on_idle_screen_delete_cb(lv_event_t *e)
{
    live_pill_unbind_ui();
    lv_obj_t *scr = lv_event_get_target(e);
    app_idle_data_t *data = (app_idle_data_t *)lv_obj_get_user_data(scr);
    if (data) {
        if (data == s_idle_data) {
            s_idle_data = NULL;
        }
        free(data);
        lv_obj_set_user_data(scr, NULL);
    }
    if (scr == s_idle_screen) {
        s_idle_screen = NULL;
    }
}

static void on_idle_lsk(void)
{
    OS_LOGI(TAG, "LSK 'Menu' triggered -> opening Launcher");
    app_launcher_open();
}

static void on_idle_rsk(void)
{
    OS_LOGI(TAG, "RSK 'Contacts' triggered -> opening Contacts");
    app_contacts_open();
}

static void on_unified_pill_clicked(lv_event_t *e)
{
    (void)e;
    live_pill_trigger_click();
}

lv_obj_t * app_idle_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    if (!scr) return NULL;

    app_idle_data_t *data = (app_idle_data_t *)calloc(1, sizeof(app_idle_data_t));
    if (!data) {
        lv_obj_delete(scr);
        return NULL;
    }

    s_idle_screen = scr;
    s_idle_data = data;

    data->view_type = VEEBHA_VIEW_TYPE_IDLE;
    strncpy(data->title, "Idle", sizeof(data->title) - 1);

    lv_obj_set_size(scr, CONFIG_DISP_HOR_RES, CONFIG_DISP_VER_RES);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* Child 0: Base Wallpaper Image Layer (176x220) */
    data->wp_img = lv_image_create(scr);
    lv_obj_set_size(data->wp_img, CONFIG_DISP_HOR_RES, CONFIG_DISP_VER_RES);
    lv_obj_align(data->wp_img, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_remove_flag(data->wp_img, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_image_set_src(data->wp_img, wallpaper_get_img_dsc());

    /* Child 1: Scrim Layer (Full screen overlay with 35% opacity in image mode) */
    data->scrim_overlay = lv_obj_create(scr);
    lv_obj_set_size(data->scrim_overlay, lv_pct(100), lv_pct(100));
    lv_obj_align(data->scrim_overlay, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(data->scrim_overlay, lv_color_black(), 0);
    lv_obj_set_style_border_width(data->scrim_overlay, 0, 0);
    lv_obj_set_style_radius(data->scrim_overlay, 0, 0);
    lv_obj_set_style_pad_all(data->scrim_overlay, 0, 0);
    lv_obj_remove_flag(data->scrim_overlay, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    /* Child 2: Content Layer (Flex Column: Status Bar, Viewport, Softkeys) */
    lv_obj_t *content = lv_obj_create(scr);
    data->content_layer = content;
    lv_obj_set_size(content, CONFIG_DISP_HOR_RES, CONFIG_DISP_VER_RES);
    lv_obj_align(content, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 1. System Status Bar (18px) */
    data->status_bar = status_bar_create(content, "");

    /* 2. Middle Viewport (Elastic ~182px) */
    lv_obj_t *viewport = lv_obj_create(content);
    lv_obj_set_size(viewport, CONFIG_DISP_HOR_RES, 0);
    lv_obj_set_flex_grow(viewport, 1);
    lv_obj_set_style_bg_opa(viewport, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(viewport, 0, 0);
    lv_obj_set_style_pad_hor(viewport, 6, 0);
    lv_obj_set_style_pad_top(viewport, 6, 0);
    lv_obj_set_style_pad_bottom(viewport, 4, 0);
    lv_obj_set_flex_flow(viewport, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(viewport, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(viewport, LV_OBJ_FLAG_SCROLLABLE);

    /* 2a. Carrier Label */
    lv_obj_t *carrier_lbl = lv_label_create(viewport);
    lv_label_set_text(carrier_lbl, "Veebha Air 4G");
    lv_obj_set_height(carrier_lbl, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(carrier_lbl, lv_color_hex(0xE2E8F0), 0);
    lv_obj_set_style_text_font(carrier_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_pad_bottom(carrier_lbl, 2, 0);

    /* 2b. Frosted Clock & Date Card */
    lv_obj_t *clock_card = lv_obj_create(viewport);
    lv_obj_set_size(clock_card, 150, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(clock_card, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_opa(clock_card, LV_OPA_70, 0);
    lv_obj_set_style_border_color(clock_card, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(clock_card, 1, 0);
    lv_obj_set_style_radius(clock_card, 0, 0);
    lv_obj_set_style_outline_width(clock_card, 0, 0);
    lv_obj_set_style_pad_ver(clock_card, 6, 0);
    lv_obj_set_style_pad_hor(clock_card, 8, 0);
    lv_obj_set_flex_flow(clock_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(clock_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(clock_card, LV_OBJ_FLAG_SCROLLABLE);

#define LV_OPA_35 ((lv_opa_t)89)

    /* Large Digital Clock */
    data->clock_lbl = lv_label_create(clock_card);
    lv_label_set_text(data->clock_lbl, "12:00");
    lv_obj_set_style_text_color(data->clock_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(data->clock_lbl, &lv_font_montserrat_24, 0);

    /* Date Label */
    data->date_lbl = lv_label_create(clock_card);
    lv_label_set_text(data->date_lbl, "Tue, 22 Sep");
    lv_obj_set_style_text_color(data->date_lbl, theme_get()->accent, 0);
    lv_obj_set_style_text_font(data->date_lbl, &lv_font_montserrat_12, 0);

    /* 2d. Notification Pills Container */
    lv_obj_t *pills_cnt = lv_obj_create(viewport);
    lv_obj_set_size(pills_cnt, 160, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(pills_cnt, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(pills_cnt, 0, 0);
    lv_obj_set_style_pad_all(pills_cnt, 0, 0);
    lv_obj_set_style_pad_row(pills_cnt, 3, 0);
    lv_obj_set_flex_flow(pills_cnt, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(pills_cnt, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(pills_cnt, LV_OBJ_FLAG_SCROLLABLE);

    /* Horizontal flex row container for SMS and Missed Calls */
    data->notif_row = lv_obj_create(pills_cnt);
    lv_obj_set_size(data->notif_row, 160, 24);
    lv_obj_set_style_bg_opa(data->notif_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(data->notif_row, 0, 0);
    lv_obj_set_style_pad_all(data->notif_row, 0, 0);
    lv_obj_set_flex_flow(data->notif_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(data->notif_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(data->notif_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(data->notif_row, LV_OBJ_FLAG_HIDDEN);

    /* Left badge: [ ✉ 1 SMS ] */
    data->sms_pill = lv_obj_create(data->notif_row);
    lv_obj_set_size(data->sms_pill, 76, 22);
    lv_obj_set_style_bg_color(data->sms_pill, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_opa(data->sms_pill, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(data->sms_pill, lv_color_hex(0x3B82F6), 0);
    lv_obj_set_style_border_width(data->sms_pill, 1, 0);
    lv_obj_set_style_radius(data->sms_pill, 0, 0);
    lv_obj_set_style_outline_width(data->sms_pill, 0, 0);
    lv_obj_set_style_pad_hor(data->sms_pill, 4, 0);
    lv_obj_set_style_pad_ver(data->sms_pill, 1, 0);
    lv_obj_remove_flag(data->sms_pill, LV_OBJ_FLAG_SCROLLABLE);

    data->sms_lbl = lv_label_create(data->sms_pill);
    lv_label_set_text(data->sms_lbl, LV_SYMBOL_ENVELOPE " 0 SMS");
    lv_obj_center(data->sms_lbl);
    lv_obj_set_style_text_color(data->sms_lbl, lv_color_hex(0x60A5FA), 0);
    lv_obj_set_style_text_font(data->sms_lbl, veebha_font_get_default(), 0);

    /* Right badge: [ 📞 2 Missed ] */
    data->call_pill = lv_obj_create(data->notif_row);
    lv_obj_set_size(data->call_pill, 76, 22);
    lv_obj_set_style_bg_color(data->call_pill, lv_color_hex(0x2D1515), 0);
    lv_obj_set_style_bg_opa(data->call_pill, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(data->call_pill, lv_color_hex(0xEF4444), 0);
    lv_obj_set_style_border_width(data->call_pill, 1, 0);
    lv_obj_set_style_radius(data->call_pill, 0, 0);
    lv_obj_set_style_outline_width(data->call_pill, 0, 0);
    lv_obj_set_style_pad_hor(data->call_pill, 4, 0);
    lv_obj_set_style_pad_ver(data->call_pill, 1, 0);
    lv_obj_remove_flag(data->call_pill, LV_OBJ_FLAG_SCROLLABLE);

    data->call_lbl = lv_label_create(data->call_pill);
    lv_label_set_text(data->call_lbl, LV_SYMBOL_CALL " 0 Missed");
    lv_obj_center(data->call_lbl);
    lv_obj_set_style_text_color(data->call_lbl, lv_color_hex(0xF87171), 0);
    lv_obj_set_style_text_font(data->call_lbl, veebha_font_get_default(), 0);

    /* Unified Live Pill Capsule (160x24) */
    data->unified_pill = lv_obj_create(pills_cnt);
    lv_obj_set_size(data->unified_pill, 160, 24);
    lv_obj_set_style_bg_color(data->unified_pill, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_opa(data->unified_pill, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(data->unified_pill, theme_get()->accent, 0);
    lv_obj_set_style_border_width(data->unified_pill, 1, 0);
    lv_obj_set_style_radius(data->unified_pill, 0, 0);
    lv_obj_set_style_outline_width(data->unified_pill, 0, 0);
    lv_obj_set_style_pad_hor(data->unified_pill, 6, 0);
    lv_obj_set_style_pad_ver(data->unified_pill, 2, 0);
    lv_obj_set_style_pad_column(data->unified_pill, 4, 0);
    lv_obj_remove_flag(data->unified_pill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(data->unified_pill, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(data->unified_pill, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(data->unified_pill, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(data->unified_pill, on_unified_pill_clicked, LV_EVENT_CLICKED, NULL);

    /* Child 1: Icon label */
    data->pill_icon_lbl = lv_label_create(data->unified_pill);
    lv_label_set_text(data->pill_icon_lbl, "");
    lv_obj_set_style_text_color(data->pill_icon_lbl, theme_get()->accent, 0);
    lv_obj_set_style_text_font(data->pill_icon_lbl, veebha_font_get_default(), 0);

    /* Child 2: Text label (width: 124px, centered text, circular scroll) */
    data->pill_text_lbl = lv_label_create(data->unified_pill);
    lv_label_set_text(data->pill_text_lbl, "");
    lv_obj_set_width(data->pill_text_lbl, 124);
    lv_obj_set_style_text_align(data->pill_text_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(data->pill_text_lbl, theme_get()->accent, 0);
    lv_obj_set_style_text_font(data->pill_text_lbl, veebha_font_get_default(), 0);
    lv_label_set_long_mode(data->pill_text_lbl, LV_LABEL_LONG_DOT);

    lv_obj_add_flag(data->unified_pill, LV_OBJ_FLAG_HIDDEN);

    live_pill_bind_ui(data->unified_pill, data->pill_icon_lbl, data->pill_text_lbl);

    /* Dummy focusable object to anchor keypad group cleanly */
    lv_obj_t *focus_anchor = lv_obj_create(viewport);
    lv_obj_set_size(focus_anchor, 1, 1);
    lv_obj_set_style_bg_opa(focus_anchor, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(focus_anchor, 0, 0);
    lv_obj_add_flag(focus_anchor, LV_OBJ_FLAG_CLICKABLE);
    data->first_item = focus_anchor;

    /* 3. Softkey Bar (20px) */
    data->softkey_bar = softkey_bar_create(content, veebha_i18n_str(STR_MENU), veebha_i18n_str(STR_CONTACTS));
    softkey_bar_set_active_widget(data->softkey_bar);
    softkey_set_actions(veebha_i18n_str(STR_MENU), on_idle_lsk, veebha_i18n_str(STR_CONTACTS), on_idle_rsk);

    lv_obj_set_user_data(scr, data);
    lv_obj_add_event_cb(scr, on_idle_screen_delete_cb, LV_EVENT_DELETE, NULL);

    app_idle_refresh_wallpaper();
    app_idle_update();

    OS_LOGI(TAG, "Standby / Idle Screen created (%p)", (void*)scr);
    return scr;
}

void app_idle_update(void)
{
    if (!s_idle_data) return;

    /* Refresh Clock and Date from RTC */
    if (s_idle_data->clock_lbl && lv_obj_is_valid(s_idle_data->clock_lbl)) {
        uint8_t hours = 12, mins = 0;
        status_bar_get_rtc_time(&hours, &mins);
        char clock_buf[16];
        snprintf(clock_buf, sizeof(clock_buf), "%02u:%02u", (unsigned int)hours, (unsigned int)mins);
        const char *curr_text = lv_label_get_text(s_idle_data->clock_lbl);
        if (!curr_text || strcmp(curr_text, clock_buf) != 0) {
            lv_label_set_text(s_idle_data->clock_lbl, clock_buf);
        }
    }

    if (s_idle_data->date_lbl && lv_obj_is_valid(s_idle_data->date_lbl)) {
        uint16_t year = 2026;
        uint8_t month = 9, day = 23;
        status_bar_get_rtc_date(&year, &month, &day);

        static const char *dow_names[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
        static const char *mon_names[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
        static const int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };

        int calc_y = (int)year;
        if (month < 3) calc_y -= 1;
        int dow = (calc_y + calc_y / 4 - calc_y / 100 + calc_y / 400 + t[(month >= 1 && month <= 12) ? (month - 1) : 0] + day) % 7;
        if (dow < 0 || dow > 6) dow = 0;

        const char *mstr = (month >= 1 && month <= 12) ? mon_names[month - 1] : "Sep";
        char date_buf[32];
        snprintf(date_buf, sizeof(date_buf), "%s, %02u %s", dow_names[dow], (unsigned int)day, mstr);
        const char *curr_date = lv_label_get_text(s_idle_data->date_lbl);
        if (!curr_date || strcmp(curr_date, date_buf) != 0) {
            lv_label_set_text(s_idle_data->date_lbl, date_buf);
        }
    }

    /* Refresh Unified Live Pill */
    live_pill_refresh();

    /* Check unread SMS */
    uint16_t unread_sms = telephony_get_unread_sms_count();
    if (s_idle_data->sms_pill && s_idle_data->sms_lbl) {
        if (unread_sms > 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), LV_SYMBOL_ENVELOPE " %u SMS", unread_sms);
            lv_label_set_text(s_idle_data->sms_lbl, buf);
            lv_obj_clear_flag(s_idle_data->sms_pill, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_idle_data->sms_pill, LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* Check missed calls */
    uint16_t missed_calls = telephony_get_missed_call_count();
    if (s_idle_data->call_pill && s_idle_data->call_lbl) {
        if (missed_calls > 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), LV_SYMBOL_CALL " %u Missed", missed_calls);
            lv_label_set_text(s_idle_data->call_lbl, buf);
            lv_obj_clear_flag(s_idle_data->call_pill, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_idle_data->call_pill, LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* Update notification row visibility and alignment */
    if (s_idle_data->notif_row) {
        if (unread_sms > 0 || missed_calls > 0) {
            lv_obj_clear_flag(s_idle_data->notif_row, LV_OBJ_FLAG_HIDDEN);
            if (unread_sms > 0 && missed_calls == 0) {
                lv_obj_set_flex_align(s_idle_data->notif_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            } else if (missed_calls > 0 && unread_sms == 0) {
                lv_obj_set_flex_align(s_idle_data->notif_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            } else {
                lv_obj_set_flex_align(s_idle_data->notif_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            }
        } else {
            lv_obj_add_flag(s_idle_data->notif_row, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void app_idle_open(void)
{
    if (s_idle_screen) {
        win_mgr_show_home();
    } else {
        lv_obj_t *scr = app_idle_create();
        if (scr) {
            win_mgr_push(scr, veebha_i18n_str(STR_MENU), on_idle_lsk, veebha_i18n_str(STR_CONTACTS), on_idle_rsk);
        }
    }
}

bool app_idle_is_active(void)
{
    return (s_idle_screen != NULL && win_mgr_get_active_view_type() == VEEBHA_VIEW_TYPE_IDLE);
}
