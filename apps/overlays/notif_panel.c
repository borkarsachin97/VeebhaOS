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

#include "veebha_overlays.h"
#include "veebha_win_mgr.h"
#include "veebha_softkeys.h"
#include "veebha_templates.h"
#include "veebha_status_bar.h"
#include "veebha_theme.h"
#include "drivers/hal_input.h"
#include "boards/board_config.h"
#include "apps/common/mock_telephony.h"
#include "apps/messages/app_messages.h"
#include "apps/calllogs/app_calllogs.h"
#include "apps/settings/app_settings.h"
#include "sdk/include/veebha_i18n.h"
#include "sdk/text/font_fallback.h"
#include "sdk/include/veebha_hardware.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_NOTIFS 16

typedef struct {
    uint32_t id;
    uint8_t  type; /* 0: Alert, 1: SMS, 2: Missed Call */
    char     icon[8];
    char     source[32];
    char     timestamp[16];
    char     body[160];
} notif_entry_t;

static notif_entry_t s_notifs[MAX_NOTIFS];
static uint16_t      s_notif_count = 0;
static uint32_t      s_next_notif_id = 1;

static char s_alert_title[32] = "";
static char s_alert_msg[64] = "";

typedef struct {
    bool    bluetooth;
    uint8_t brightness_level; /* 0: 25%, 1: 50%, 2: 75%, 3: 100% */
    bool    flashlight;
} quick_settings_state_t;

static quick_settings_state_t s_settings_state = {
    .bluetooth = false,
    .brightness_level = 1,
    .flashlight = false
};

typedef struct {
    lv_obj_t          *backdrop;
    lv_obj_t          *container;
    lv_obj_t          *header;
    lv_obj_t          *clock_lbl;
    lv_obj_t          *dot_left;
    lv_obj_t          *dot_right;
    lv_obj_t          *batt_lbl;

    lv_obj_t          *viewport;
    lv_obj_t          *page_notif;
    lv_obj_t          *page_qs;

    lv_obj_t          *notif_cards[MAX_NOTIFS];
    lv_obj_t          *clear_all_btn;
    lv_obj_t          *empty_lbl;

    lv_obj_t          *qs_grid;
    lv_obj_t          *qs_tiles[6];
    lv_obj_t          *media_card;
    lv_obj_t          *media_icon;
    lv_obj_t          *media_bar;

    lv_group_t        *group;
    lv_obj_t          *saved_focus;
    char               saved_lsk_label[WIN_MGR_LABEL_MAX];
    softkey_callback_t saved_lsk_cb;
    char               saved_rsk_label[WIN_MGR_LABEL_MAX];
    softkey_callback_t saved_rsk_cb;

    notif_page_t       active_page;
    int8_t             focused_qs_idx; /* 0..5 */
    int8_t             focused_notif_idx; /* 0..s_notif_count (or s_notif_count for clear button) */
} notif_panel_ctx_t;

static notif_panel_ctx_t *s_panel = NULL;
static lv_group_t        *s_notif_panel_group = NULL;
static bool               s_panel_visible = false;

static void rebuild_notif_deck(void);
static void update_qs_tile_visuals(uint8_t idx);
static void update_header_indicators(void);
static void update_softkeys_for_focus(void);

void notif_panel_post_alert(const char *title, const char *msg)
{
    if (title) {
        strncpy(s_alert_title, title, sizeof(s_alert_title) - 1);
        s_alert_title[sizeof(s_alert_title) - 1] = '\0';
    } else {
        s_alert_title[0] = '\0';
    }
    if (msg) {
        strncpy(s_alert_msg, msg, sizeof(s_alert_msg) - 1);
        s_alert_msg[sizeof(s_alert_msg) - 1] = '\0';
    } else {
        s_alert_msg[0] = '\0';
    }

    if (title && title[0] != '\0') {
        /* Check if alert already exists, update it */
        bool found = false;
        for (uint16_t i = 0; i < s_notif_count; i++) {
            if (s_notifs[i].type == 0 && strcmp(s_notifs[i].source, title) == 0) {
                if (msg) {
                    strncpy(s_notifs[i].body, msg, sizeof(s_notifs[i].body) - 1);
                }
                found = true;
                break;
            }
        }
        if (!found && s_notif_count < MAX_NOTIFS) {
            notif_entry_t *n = &s_notifs[s_notif_count++];
            n->id = s_next_notif_id++;
            n->type = 0;
            strncpy(n->icon, LV_SYMBOL_BELL, sizeof(n->icon) - 1);
            strncpy(n->source, title, sizeof(n->source) - 1);
            strncpy(n->timestamp, "Just now", sizeof(n->timestamp) - 1);
            strncpy(n->body, msg ? msg : "", sizeof(n->body) - 1);
        }
    }

    if (s_panel && s_panel_visible && s_panel->page_notif && lv_obj_is_valid(s_panel->page_notif)) {
        rebuild_notif_deck();
    }
}

const char * notif_panel_get_last_alert_title(void)
{
    return s_alert_title;
}

const char * notif_panel_get_last_alert_msg(void)
{
    return s_alert_msg;
}

void notif_panel_clear_all(void)
{
    s_notif_count = 0;
    s_alert_title[0] = '\0';
    s_alert_msg[0] = '\0';
    if (s_panel && s_panel_visible && s_panel->page_notif && lv_obj_is_valid(s_panel->page_notif)) {
        rebuild_notif_deck();
    }
}

uint16_t notif_panel_get_count(void)
{
    return s_notif_count;
}

bool notif_panel_is_active(void)
{
    return s_panel != NULL && s_panel_visible;
}

uint8_t notif_panel_get_page(void)
{
    return (s_panel && s_panel_visible) ? (uint8_t)s_panel->active_page : 0;
}

static void on_notif_card_focus_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *body_lbl = (lv_obj_t *)lv_event_get_user_data(e);
    if (!body_lbl || !lv_obj_is_valid(body_lbl)) return;

    if (code == LV_EVENT_FOCUSED) {
        lv_label_set_long_mode(body_lbl, LV_LABEL_LONG_DOT);
        if (s_panel) {
            lv_obj_t *card = lv_event_get_target(e);
            for (uint16_t i = 0; i < s_notif_count; i++) {
                if (s_panel->notif_cards[i] == card) {
                    s_panel->focused_notif_idx = (int8_t)i;
                    break;
                }
            }
            update_softkeys_for_focus();
        }
    } else if (code == LV_EVENT_DEFOCUSED) {
        lv_label_set_long_mode(body_lbl, LV_LABEL_LONG_DOT);
    }
}

static void on_clear_btn_focus_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_FOCUSED && s_panel) {
        s_panel->focused_notif_idx = (int8_t)s_notif_count;
        update_softkeys_for_focus();
    }
}

static void on_qs_tile_focus_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_FOCUSED && s_panel) {
        lv_obj_t *tile = lv_event_get_target(e);
        uint8_t idx = (uint8_t)(uintptr_t)lv_obj_get_user_data(tile);
        s_panel->focused_qs_idx = (int8_t)idx;
        update_softkeys_for_focus();
    }
}

static void dismiss_notification_at_index(uint16_t idx)
{
    if (idx >= s_notif_count) return;

    for (uint16_t i = idx; i < s_notif_count - 1; i++) {
        s_notifs[i] = s_notifs[i + 1];
    }
    s_notif_count--;

    if (s_notif_count == 0) {
        s_alert_title[0] = '\0';
        s_alert_msg[0] = '\0';
    }

    rebuild_notif_deck();
}

static void on_card_click(uint16_t idx)
{
    if (idx >= s_notif_count) return;

    notif_entry_t *n = &s_notifs[idx];
    uint8_t type = n->type;

    notif_panel_close();

    if (type == 1) {
        /* SMS */
        app_messages_open();
    } else if (type == 2) {
        /* Missed Call */
        app_calllogs_open();
    } else {
        /* Generic alert */
    }
}

static void on_tile_toggle(uint8_t idx)
{
    if (idx == 0) {
        /* Bluetooth */
        s_settings_state.bluetooth = !s_settings_state.bluetooth;
        status_bar_set_bt_state(s_settings_state.bluetooth, false);
        printf("[QUICK_SETTINGS] Bluetooth: %s\n", s_settings_state.bluetooth ? "ON" : "OFF");
    } else if (idx == 1) {
        /* Brightness */
        s_settings_state.brightness_level = (s_settings_state.brightness_level + 1) % 4;
        static const uint8_t bl_map[4] = {25, 50, 75, 100};
        veebha_hw_backlight_set(bl_map[s_settings_state.brightness_level]);
        printf("[QUICK_SETTINGS] Brightness: %u (%u%%)\n", s_settings_state.brightness_level, bl_map[s_settings_state.brightness_level]);
    } else if (idx == 2) {
        /* Sound Profile */
        sound_profile_t p = app_settings_get_profile();
        p = (sound_profile_t)((p + 1) % SOUND_PROFILE_COUNT);
        app_settings_set_profile(p);
        printf("[QUICK_SETTINGS] Sound Profile: %d\n", (int)p);
    } else if (idx == 3) {
        /* Torch */
        s_settings_state.flashlight = !s_settings_state.flashlight;
        veebha_hw_torch_set(s_settings_state.flashlight);
        printf("[QUICK_SETTINGS] Flashlight/Torch: %s\n", s_settings_state.flashlight ? "ON" : "OFF");
    } else if (idx == 4) {
        /* Battery - info display only */
        printf("[QUICK_SETTINGS] Battery: 85%%\n");
    } else if (idx == 5) {
        /* Settings */
        notif_panel_close();
        app_settings_open();
        return;
    }

    update_qs_tile_visuals(idx);
}

static void on_panel_lsk(void)
{
    if (!s_panel) return;

    if (s_panel->active_page == NOTIF_PAGE_DECK) {
        if (s_panel->focused_notif_idx >= 0 && s_panel->focused_notif_idx < s_notif_count) {
            dismiss_notification_at_index((uint16_t)s_panel->focused_notif_idx);
        } else if (s_panel->focused_notif_idx == s_notif_count && s_notif_count > 0) {
            notif_panel_clear_all();
        }
    } else {
        if (s_panel->focused_qs_idx >= 0 && s_panel->focused_qs_idx < 6) {
            on_tile_toggle((uint8_t)s_panel->focused_qs_idx);
        }
    }
}

static void on_panel_rsk(void)
{
    notif_panel_close();
}

static void update_softkeys_for_focus(void)
{
    if (!s_panel) return;

    if (s_panel->active_page == NOTIF_PAGE_DECK) {
        if (s_notif_count == 0) {
            softkey_set_actions("", NULL, "Close", on_panel_rsk);
        } else if (s_panel->focused_notif_idx == s_notif_count) {
            softkey_set_actions("Clear All", on_panel_lsk, "Close", on_panel_rsk);
        } else {
            softkey_set_actions("Dismiss", on_panel_lsk, "Close", on_panel_rsk);
        }
    } else {
        if (s_panel->focused_qs_idx == 5) {
            softkey_set_actions("Open", on_panel_lsk, "Close", on_panel_rsk);
        } else {
            softkey_set_actions("Toggle", on_panel_lsk, "Close", on_panel_rsk);
        }
    }
}

static void on_card_clicked_event(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    uint16_t idx = (uint16_t)(uintptr_t)lv_obj_get_user_data(target);
    on_card_click(idx);
}

static void on_clear_all_clicked_event(lv_event_t *e)
{
    (void)e;
    notif_panel_clear_all();
}

static void on_tile_clicked_event(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    uint8_t idx = (uint8_t)(uintptr_t)lv_obj_get_user_data(target);
    on_tile_toggle(idx);
}

static void rebuild_notif_deck(void)
{
    if (!s_panel || !s_panel->page_notif) return;

    lv_obj_clean(s_panel->page_notif);
    memset(s_panel->notif_cards, 0, sizeof(s_panel->notif_cards));
    s_panel->clear_all_btn = NULL;
    s_panel->empty_lbl = NULL;

    if (s_panel->group && s_panel->active_page == NOTIF_PAGE_DECK) {
        /* Clear group objects */
        lv_group_remove_all_objs(s_panel->group);
    }

    if (s_notif_count == 0) {
        s_panel->empty_lbl = lv_label_create(s_panel->page_notif);
        lv_label_set_text(s_panel->empty_lbl, "No Notifications");
        lv_obj_set_style_text_color(s_panel->empty_lbl, lv_color_hex(0x6E7480), 0);
        lv_obj_set_style_text_font(s_panel->empty_lbl, veebha_font_get_default(), 0);
        lv_obj_set_style_pad_top(s_panel->empty_lbl, 50, 0);
        s_panel->focused_notif_idx = -1;
    } else {
        for (uint16_t i = 0; i < s_notif_count; i++) {
            notif_entry_t *n = &s_notifs[i];

            lv_obj_t *card = lv_button_create(s_panel->page_notif);
            s_panel->notif_cards[i] = card;
            lv_obj_set_size(card, 166, 38);
            lv_obj_set_style_bg_color(card, theme_get()->card_color, 0);
            lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(card, theme_is_light_mode() ? lv_color_hex(0xCCCCCC) : lv_color_hex(0x2D3340), 0);
            lv_obj_set_style_border_width(card, 1, 0);
            lv_obj_set_style_radius(card, 0, 0);
            lv_obj_set_style_outline_width(card, 0, 0);
            lv_obj_set_style_outline_pad(card, 0, 0);
            lv_obj_set_style_pad_hor(card, 6, 0);
            lv_obj_set_style_pad_ver(card, 3, 0);
            lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_set_style_border_color(card, theme_get()->accent, LV_STATE_FOCUSED);
            lv_obj_set_style_border_width(card, 2, LV_STATE_FOCUSED);
            lv_obj_set_style_radius(card, 0, LV_STATE_FOCUSED);
            lv_obj_set_style_outline_width(card, 0, LV_STATE_FOCUSED);
            lv_obj_set_style_outline_pad(card, 0, LV_STATE_FOCUSED);
            lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

            /* Row 1: Icon + Source + Timestamp */
            lv_obj_t *meta_row = lv_obj_create(card);
            lv_obj_set_size(meta_row, lv_pct(100), 14);
            lv_obj_set_style_bg_opa(meta_row, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(meta_row, 0, 0);
            lv_obj_set_style_pad_all(meta_row, 0, 0);
            lv_obj_remove_flag(meta_row, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_flex_flow(meta_row, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(meta_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

            lv_obj_t *icon_lbl = lv_label_create(meta_row);
            lv_label_set_text(icon_lbl, n->icon);
            lv_obj_set_style_text_color(icon_lbl, theme_get()->accent, 0);
            lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_10, 0);

            lv_obj_t *src_lbl = lv_label_create(meta_row);
            lv_label_set_text(src_lbl, n->source);
            lv_obj_set_style_text_color(src_lbl, theme_get()->accent, 0);
            lv_obj_set_style_text_font(src_lbl, veebha_font_get_default(), 0);
            lv_obj_set_style_pad_left(src_lbl, 4, 0);
            lv_obj_set_flex_grow(src_lbl, 1);

            lv_obj_t *ts_lbl = lv_label_create(meta_row);
            lv_label_set_text(ts_lbl, n->timestamp);
            lv_obj_set_style_text_color(ts_lbl, theme_get()->text_muted, 0);
            lv_obj_set_style_text_font(ts_lbl, veebha_font_get_default(), 0);

            /* Row 2: Body text */
            lv_obj_t *body_lbl = lv_label_create(card);
            lv_label_set_text(body_lbl, n->body);
            lv_obj_set_style_text_color(body_lbl, theme_get()->text_primary, 0);
            lv_obj_set_style_text_font(body_lbl, veebha_font_get_default(), 0);
            lv_obj_set_width(body_lbl, 154);
            lv_label_set_long_mode(body_lbl, LV_LABEL_LONG_DOT);

            lv_obj_set_user_data(card, (void*)(uintptr_t)i);
            lv_obj_add_event_cb(card, on_card_clicked_event, LV_EVENT_CLICKED, NULL);
            lv_obj_add_event_cb(card, on_notif_card_focus_cb, LV_EVENT_FOCUSED, body_lbl);
            lv_obj_add_event_cb(card, on_notif_card_focus_cb, LV_EVENT_DEFOCUSED, body_lbl);

            if (s_panel->group && s_panel->active_page == NOTIF_PAGE_DECK) {
                lv_group_add_obj(s_panel->group, card);
            }
        }

        /* Clear All Button */
        lv_obj_t *clr_btn = lv_button_create(s_panel->page_notif);
        s_panel->clear_all_btn = clr_btn;
        lv_obj_set_size(clr_btn, 110, 22);
        lv_obj_set_style_bg_color(clr_btn, lv_color_hex(0x222630), 0);
        lv_obj_set_style_bg_opa(clr_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(clr_btn, lv_color_hex(0x3B4252), 0);
        lv_obj_set_style_border_width(clr_btn, 1, 0);
        lv_obj_set_style_radius(clr_btn, 0, 0);
        lv_obj_set_style_outline_width(clr_btn, 0, 0);
        lv_obj_set_style_outline_pad(clr_btn, 0, 0);
        lv_obj_set_style_pad_all(clr_btn, 0, 0);
        lv_obj_remove_flag(clr_btn, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_set_style_border_color(clr_btn, theme_get()->accent, LV_STATE_FOCUSED);
        lv_obj_set_style_border_width(clr_btn, 2, LV_STATE_FOCUSED);
        lv_obj_set_style_radius(clr_btn, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(clr_btn, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_pad(clr_btn, 0, LV_STATE_FOCUSED);

        lv_obj_t *clr_lbl = lv_label_create(clr_btn);
        lv_label_set_text(clr_lbl, LV_SYMBOL_CLOSE " Clear All");
        lv_obj_set_style_text_color(clr_lbl, theme_get()->text_primary, 0);
        lv_obj_set_style_text_font(clr_lbl, veebha_font_get_default(), 0);
        lv_obj_center(clr_lbl);

        lv_obj_add_event_cb(clr_btn, on_clear_all_clicked_event, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(clr_btn, on_clear_btn_focus_cb, LV_EVENT_FOCUSED, NULL);

        if (s_panel->group && s_panel->active_page == NOTIF_PAGE_DECK) {
            lv_group_add_obj(s_panel->group, clr_btn);
        }

        s_panel->focused_notif_idx = 0;
    }

    if (s_panel->active_page == NOTIF_PAGE_DECK && s_panel->group) {
        if (s_notif_count > 0 && s_panel->notif_cards[0]) {
            lv_group_focus_obj(s_panel->notif_cards[0]);
        }
    }

    update_softkeys_for_focus();
}

static void update_qs_tile_visuals(uint8_t idx)
{
    if (!s_panel || idx >= 6) return;
    lv_obj_t *tile = s_panel->qs_tiles[idx];
    if (!tile || !lv_obj_is_valid(tile)) return;

    bool is_on = false;
    char status_buf[16] = "";

    switch (idx) {
    case 0: /* Bluetooth */
        is_on = s_settings_state.bluetooth;
        snprintf(status_buf, sizeof(status_buf), "%s", is_on ? "On" : "Off");
        break;
    case 1: /* Brightness */
        is_on = true;
        snprintf(status_buf, sizeof(status_buf), "%u%%", (s_settings_state.brightness_level + 1) * 25);
        break;
    case 2: /* Profile */
        {
            sound_profile_t p = app_settings_get_profile();
            is_on = (p != SOUND_PROFILE_SILENT);
            snprintf(status_buf, sizeof(status_buf), "%s", (p == SOUND_PROFILE_SILENT) ? "Silent" : (p == SOUND_PROFILE_OUTDOOR) ? "Outdoor" : "Normal");
        }
        break;
    case 3: /* Torch */
        is_on = s_settings_state.flashlight;
        snprintf(status_buf, sizeof(status_buf), "%s", is_on ? "On" : "Off");
        break;
    case 4: /* Battery */
        {
            const status_bar_global_state_t *gs = status_bar_get_global_state();
            uint8_t bat_pct = gs ? gs->battery_level : veebha_hw_battery_get_percent();
            bool is_chg = gs ? gs->is_charging : veebha_hw_battery_is_charging();
            is_on = true;
            snprintf(status_buf, sizeof(status_buf), "%u%%%s", (unsigned int)bat_pct, is_chg ? " +" : "");
        }
        break;
    case 5: /* Settings */
        is_on = false;
        snprintf(status_buf, sizeof(status_buf), "Open");
        break;
    default: break;
    }

    if (is_on) {
        lv_obj_set_style_bg_color(tile, lv_color_hex(0x182230), 0);
        lv_obj_set_style_border_color(tile, lv_color_hex(0x28384A), 0);
    } else {
        lv_obj_set_style_bg_color(tile, lv_color_hex(0x121418), 0);
        lv_obj_set_style_border_color(tile, lv_color_hex(0x1F242C), 0);
    }

    if (lv_obj_get_child_count(tile) >= 2) {
        lv_obj_t *icon_lbl = lv_obj_get_child(tile, 0);
        lv_obj_t *status_lbl = lv_obj_get_child(tile, 1);
        if (icon_lbl && lv_obj_is_valid(icon_lbl)) {
            if (idx == 2) {
                sound_profile_t p = app_settings_get_profile();
                lv_label_set_text(icon_lbl, (p == SOUND_PROFILE_SILENT) ? LV_SYMBOL_MUTE : LV_SYMBOL_BELL);
            }
            lv_obj_set_style_text_color(icon_lbl, is_on ? theme_get()->accent : theme_get()->text_muted, 0);
        }
        if (status_lbl && lv_obj_is_valid(status_lbl)) {
            lv_label_set_text(status_lbl, status_buf);
            lv_obj_set_style_text_color(status_lbl, is_on ? theme_get()->text_primary : theme_get()->text_muted, 0);
        }
    }
}

static void update_header_indicators(void)
{
    if (!s_panel || !s_panel->dot_left || !s_panel->dot_right) return;

    if (s_panel->active_page == NOTIF_PAGE_DECK) {
        /* Left dot active/accent, right dot muted */
        lv_obj_set_style_bg_color(s_panel->dot_left, theme_get()->accent, 0);
        lv_obj_set_style_bg_opa(s_panel->dot_left, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(s_panel->dot_left, 0, 0);

        lv_obj_set_style_bg_color(s_panel->dot_right, lv_color_hex(0x444444), 0);
        lv_obj_set_style_bg_opa(s_panel->dot_right, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(s_panel->dot_right, lv_color_hex(0x444444), 0);
        lv_obj_set_style_border_width(s_panel->dot_right, 1, 0);
    } else {
        /* Left dot muted, right dot active/accent */
        lv_obj_set_style_bg_color(s_panel->dot_left, lv_color_hex(0x444444), 0);
        lv_obj_set_style_bg_opa(s_panel->dot_left, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(s_panel->dot_left, lv_color_hex(0x444444), 0);
        lv_obj_set_style_border_width(s_panel->dot_left, 1, 0);

        lv_obj_set_style_bg_color(s_panel->dot_right, theme_get()->accent, 0);
        lv_obj_set_style_bg_opa(s_panel->dot_right, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(s_panel->dot_right, 0, 0);
    }
}

void notif_panel_set_page(uint8_t page)
{
    if (!s_panel) return;
    if (page >= NOTIF_PAGE_COUNT) page = 0;

    s_panel->active_page = (notif_page_t)page;

    if (s_panel->active_page == NOTIF_PAGE_DECK) {
        lv_obj_clear_flag(s_panel->page_notif, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_panel->page_qs, LV_OBJ_FLAG_HIDDEN);

        if (s_panel->group) {
            lv_group_remove_all_objs(s_panel->group);
            for (uint16_t i = 0; i < s_notif_count; i++) {
                if (s_panel->notif_cards[i]) {
                    lv_group_add_obj(s_panel->group, s_panel->notif_cards[i]);
                }
            }
            if (s_panel->clear_all_btn) {
                lv_group_add_obj(s_panel->group, s_panel->clear_all_btn);
            }
            if (s_notif_count > 0 && s_panel->notif_cards[0]) {
                lv_group_focus_obj(s_panel->notif_cards[0]);
                s_panel->focused_notif_idx = 0;
            } else {
                s_panel->focused_notif_idx = -1;
            }
        }
    } else {
        lv_obj_add_flag(s_panel->page_notif, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_panel->page_qs, LV_OBJ_FLAG_HIDDEN);

        if (s_panel->group) {
            lv_group_remove_all_objs(s_panel->group);
            for (uint8_t i = 0; i < 6; i++) {
                if (s_panel->qs_tiles[i]) {
                    lv_group_add_obj(s_panel->group, s_panel->qs_tiles[i]);
                }
            }
            if (s_panel->media_card) {
                lv_group_add_obj(s_panel->group, s_panel->media_card);
            }
            if (s_panel->qs_tiles[0]) {
                lv_group_focus_obj(s_panel->qs_tiles[0]);
                s_panel->focused_qs_idx = 0;
            }
        }
    }

    update_header_indicators();
    update_softkeys_for_focus();
}

void notif_panel_handle_key(veebha_key_t key)
{
    if (!s_panel) return;

    if (key == VEEBHA_KEY_CALL) {
        /* Call key toggles page */
        notif_panel_set_page(s_panel->active_page == NOTIF_PAGE_DECK ? NOTIF_PAGE_QUICK_SETTINGS : NOTIF_PAGE_DECK);
        return;
    }

    if (key == VEEBHA_KEY_STAR) {
        notif_panel_set_page(NOTIF_PAGE_DECK);
        return;
    }

    if (key == VEEBHA_KEY_HASH) {
        notif_panel_set_page(NOTIF_PAGE_QUICK_SETTINGS);
        return;
    }

    if (key == VEEBHA_KEY_RSK) {
        notif_panel_close();
        return;
    }

    if (key == VEEBHA_KEY_LSK) {
        on_panel_lsk();
        return;
    }

    if (key == VEEBHA_KEY_OK) {
        if (s_panel->active_page == NOTIF_PAGE_DECK) {
            if (s_panel->focused_notif_idx >= 0 && s_panel->focused_notif_idx < s_notif_count) {
                on_card_click((uint16_t)s_panel->focused_notif_idx);
            } else if (s_panel->focused_notif_idx == s_notif_count && s_notif_count > 0) {
                notif_panel_clear_all();
            }
        } else {
            if (s_panel->focused_qs_idx >= 0 && s_panel->focused_qs_idx < 6) {
                on_tile_toggle((uint8_t)s_panel->focused_qs_idx);
            } else if (s_panel->focused_qs_idx == 6 && tpl_media_has_active_session()) {
                tpl_media_handle_toggle();
                if (s_panel->media_icon && lv_obj_is_valid(s_panel->media_icon)) {
                    lv_label_set_text(s_panel->media_icon, tpl_media_is_playing() ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
                }
                if (s_panel->media_bar && lv_obj_is_valid(s_panel->media_bar)) {
                    lv_bar_set_value(s_panel->media_bar, (int32_t)tpl_media_get_progress(), LV_ANIM_OFF);
                }
            }
        }
        return;
    }

    if (key == VEEBHA_KEY_RIGHT) {
        if (s_panel->active_page == NOTIF_PAGE_DECK) {
            /* D-pad Right transitions to Quick Settings */
            notif_panel_set_page(NOTIF_PAGE_QUICK_SETTINGS);
            return;
        } else {
            /* Move right inside 3x2 grid */
            int8_t cur = s_panel->focused_qs_idx;
            if (cur >= 0 && cur < 5) {
                s_panel->focused_qs_idx = cur + 1;
                if (s_panel->qs_tiles[s_panel->focused_qs_idx]) {
                    lv_group_focus_obj(s_panel->qs_tiles[s_panel->focused_qs_idx]);
                }
            }
            return;
        }
    }

    if (key == VEEBHA_KEY_LEFT) {
        if (s_panel->active_page == NOTIF_PAGE_QUICK_SETTINGS) {
            int8_t cur = s_panel->focused_qs_idx;
            /* If on Column 0 (tile 0 or tile 3), edge-cross back to Page 1 */
            if (cur == 0 || cur == 3) {
                notif_panel_set_page(NOTIF_PAGE_DECK);
                return;
            } else if (cur > 0 && cur <= 5) {
                s_panel->focused_qs_idx = cur - 1;
                if (s_panel->qs_tiles[s_panel->focused_qs_idx]) {
                    lv_group_focus_obj(s_panel->qs_tiles[s_panel->focused_qs_idx]);
                }
                return;
            }
        }
    }

    if (key == VEEBHA_KEY_DOWN) {
        if (s_panel->active_page == NOTIF_PAGE_DECK) {
            if (s_panel->group) {
                lv_group_focus_next(s_panel->group);
            }
        } else {
            int8_t cur = s_panel->focused_qs_idx;
            if (cur >= 0 && cur <= 2) {
                s_panel->focused_qs_idx = cur + 3;
                if (s_panel->qs_tiles[s_panel->focused_qs_idx]) {
                    lv_group_focus_obj(s_panel->qs_tiles[s_panel->focused_qs_idx]);
                }
            } else if (cur >= 3 && cur <= 5 && s_panel->media_card) {
                s_panel->focused_qs_idx = 6;
                lv_group_focus_obj(s_panel->media_card);
            }
        }
        return;
    }

    if (key == VEEBHA_KEY_UP) {
        if (s_panel->active_page == NOTIF_PAGE_DECK) {
            if (s_panel->group) {
                lv_group_focus_prev(s_panel->group);
            }
        } else {
            int8_t cur = s_panel->focused_qs_idx;
            if (cur >= 3 && cur <= 5) {
                s_panel->focused_qs_idx = cur - 3;
                if (s_panel->qs_tiles[s_panel->focused_qs_idx]) {
                    lv_group_focus_obj(s_panel->qs_tiles[s_panel->focused_qs_idx]);
                }
            } else if (cur == 6) {
                s_panel->focused_qs_idx = 4;
                if (s_panel->qs_tiles[4]) {
                    lv_group_focus_obj(s_panel->qs_tiles[4]);
                }
            }
        }
        return;
    }

    /* Keypad 'C' or 0 for dismissing active notif */
    if (key == VEEBHA_KEY_NUM_0) {
        if (s_panel->active_page == NOTIF_PAGE_DECK && s_panel->focused_notif_idx >= 0 && s_panel->focused_notif_idx < s_notif_count) {
            dismiss_notification_at_index((uint16_t)s_panel->focused_notif_idx);
        }
    }
}

static void on_backdrop_clicked(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *current_target = lv_event_get_current_target(e);
    if (target == current_target) {
        notif_panel_close();
    }
}

void notif_panel_show(void)
{
    if (s_panel && s_panel_visible) return;

    if (s_panel) {
        s_panel_visible = true;

        /* Save current softkey state */
        win_mgr_entry_t *top = win_mgr_get_top();
        if (top) {
            strncpy(s_panel->saved_lsk_label, top->lsk_label, sizeof(s_panel->saved_lsk_label) - 1);
            s_panel->saved_lsk_cb = top->lsk_cb;
            strncpy(s_panel->saved_rsk_label, top->rsk_label, sizeof(s_panel->saved_rsk_label) - 1);
            s_panel->saved_rsk_cb = top->rsk_cb;
        }

        /* Save global group focus */
        lv_group_t *group = win_mgr_get_group();
        if (group) {
            s_panel->saved_focus = lv_group_get_focused(group);
        }

        /* Unhide backdrop */
        if (s_panel->backdrop && lv_obj_is_valid(s_panel->backdrop)) {
            lv_obj_clear_flag(s_panel->backdrop, LV_OBJ_FLAG_HIDDEN);
        }

        /* Update clock & battery */
        uint8_t rtc_h = 12, rtc_m = 0;
        status_bar_get_rtc_time(&rtc_h, &rtc_m);
        char time_buf[16];
        snprintf(time_buf, sizeof(time_buf), "%02u:%02u", (unsigned int)rtc_h, (unsigned int)rtc_m);
        if (s_panel->clock_lbl) lv_label_set_text(s_panel->clock_lbl, time_buf);

        const status_bar_global_state_t *gs = status_bar_get_global_state();
        char bbuf[32];
        snprintf(bbuf, sizeof(bbuf), "%u%% %s", gs ? gs->battery_level : 85, (gs && gs->is_charging) ? LV_SYMBOL_CHARGE : "");
        if (s_panel->batt_lbl) lv_label_set_text(s_panel->batt_lbl, bbuf);

        /* Rebuild deck and tiles */
        rebuild_notif_deck();
        for (uint8_t i = 0; i < 6; i++) {
            update_qs_tile_visuals(i);
        }
        notif_panel_set_page(s_panel->active_page);
        update_header_indicators();

        if (s_panel->group) {
            lv_indev_set_group(hal_input_get_lv_indev(), s_panel->group);
        }
        printf("[QUICK_SETTINGS] Shade restored instantly\n");
        return;
    }

    s_panel = (notif_panel_ctx_t *)calloc(1, sizeof(notif_panel_ctx_t));
    if (!s_panel) return;

    s_panel->active_page = NOTIF_PAGE_DECK;
    s_panel->focused_qs_idx = 0;
    s_panel->focused_notif_idx = 0;

    /* Sync initial notifications from mock telephony if none present */
    if (s_notif_count == 0) {
        uint16_t msg_count = 0;
        sms_message_t *msgs = telephony_get_messages(&msg_count);
        if (msgs && msg_count > 0) {
            sms_message_t *m = &msgs[msg_count - 1];
            notif_entry_t *n = &s_notifs[s_notif_count++];
            n->id = s_next_notif_id++;
            n->type = 1;
            strncpy(n->icon, LV_SYMBOL_ENVELOPE, sizeof(n->icon) - 1);
            snprintf(n->source, sizeof(n->source), "SMS (%.20s)", m->sender);
            strncpy(n->timestamp, "10:45", sizeof(n->timestamp) - 1);
            strncpy(n->body, m->body, sizeof(n->body) - 1);
        }
        if (s_alert_title[0] != '\0' && s_notif_count < MAX_NOTIFS) {
            notif_entry_t *n = &s_notifs[s_notif_count++];
            n->id = s_next_notif_id++;
            n->type = 0;
            strncpy(n->icon, LV_SYMBOL_BELL, sizeof(n->icon) - 1);
            strncpy(n->source, s_alert_title, sizeof(n->source) - 1);
            strncpy(n->timestamp, "Just now", sizeof(n->timestamp) - 1);
            strncpy(n->body, s_alert_msg, sizeof(n->body) - 1);
        }
    }

    /* Save current softkey state */
    win_mgr_entry_t *top = win_mgr_get_top();
    if (top) {
        strncpy(s_panel->saved_lsk_label, top->lsk_label, sizeof(s_panel->saved_lsk_label) - 1);
        s_panel->saved_lsk_cb = top->lsk_cb;
        strncpy(s_panel->saved_rsk_label, top->rsk_label, sizeof(s_panel->saved_rsk_label) - 1);
        s_panel->saved_rsk_cb = top->rsk_cb;
    }

    /* Save global group focus */
    lv_group_t *group = win_mgr_get_group();
    if (group) {
        s_panel->saved_focus = lv_group_get_focused(group);
    }

    if (!s_notif_panel_group) {
        s_notif_panel_group = lv_group_create();
    } else {
        lv_group_remove_all_objs(s_notif_panel_group);
    }
    s_panel->group = s_notif_panel_group;

    /* 1. Modal Backdrop on lv_layer_top (176x220) */
    lv_obj_t *top_layer = lv_layer_top();
    lv_obj_t *backdrop = lv_obj_create(top_layer);
    s_panel->backdrop = backdrop;
    lv_obj_set_size(backdrop, 176, 220);
    lv_obj_align(backdrop, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(backdrop, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(backdrop, LV_OPA_80, 0);
    lv_obj_set_style_border_width(backdrop, 0, 0);
    lv_obj_set_style_radius(backdrop, 0, 0);
    lv_obj_set_style_pad_all(backdrop, 0, 0);
    lv_obj_remove_flag(backdrop, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(backdrop, on_backdrop_clicked, LV_EVENT_CLICKED, NULL);

    /* 2. Main Shade Container */
    lv_obj_t *container = lv_obj_create(backdrop);
    s_panel->container = container;
    lv_obj_set_size(container, 176, 202);
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(container, lv_color_hex(0x101216), 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(container, theme_get()->accent, 0);
    lv_obj_set_style_border_width(container, 1, 0);
    lv_obj_set_style_border_side(container, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_radius(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 3. Consolidated 18px Integrated Header */
    lv_obj_t *header = lv_obj_create(container);
    s_panel->header = header;
    lv_obj_set_size(header, 176, 18);
    os_theme_id_t tid = theme_get_palette();
    bool is_mono = (tid == THEME_HIGH_CONTRAST_BW);
    lv_color_t hdr_bg = (is_mono || tid == THEME_OLED_BLACK) ? lv_color_hex(0x000000) : (theme_is_light_mode() ? lv_color_hex(0xEEEEEE) : lv_color_hex(0x15181F));
    lv_obj_set_style_bg_color(header, hdr_bg, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_color_t hdr_border = is_mono ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x232730);
    lv_obj_set_style_border_color(header, hdr_border, 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_pad_hor(header, 6, 0);
    lv_obj_set_style_pad_ver(header, 0, 0);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Left: Digital Clock */
    s_panel->clock_lbl = lv_label_create(header);
    uint8_t rtc_h = 12, rtc_m = 0;
    status_bar_get_rtc_time(&rtc_h, &rtc_m);
    char time_buf[16];
    snprintf(time_buf, sizeof(time_buf), "%02u:%02u", (unsigned int)rtc_h, (unsigned int)rtc_m);
    lv_label_set_text(s_panel->clock_lbl, time_buf);
    lv_obj_set_style_text_color(s_panel->clock_lbl, theme_get()->text_primary, 0);
    lv_obj_set_style_text_font(s_panel->clock_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_width(s_panel->clock_lbl, 40);

    /* Center: Bubble Page Indicator Dots */
    lv_obj_t *dots_cnt = lv_obj_create(header);
    lv_obj_set_size(dots_cnt, 60, 16);
    lv_obj_set_style_bg_opa(dots_cnt, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dots_cnt, 0, 0);
    lv_obj_set_style_pad_all(dots_cnt, 0, 0);
    lv_obj_set_style_pad_column(dots_cnt, 8, 0);
    lv_obj_remove_flag(dots_cnt, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(dots_cnt, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dots_cnt, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    s_panel->dot_left = lv_obj_create(dots_cnt);
    lv_obj_set_size(s_panel->dot_left, 6, 6);
    lv_obj_set_style_radius(s_panel->dot_left, 0, 0);
    lv_obj_set_style_pad_all(s_panel->dot_left, 0, 0);
    lv_obj_remove_flag(s_panel->dot_left, LV_OBJ_FLAG_SCROLLABLE);

    s_panel->dot_right = lv_obj_create(dots_cnt);
    lv_obj_set_size(s_panel->dot_right, 6, 6);
    lv_obj_set_style_radius(s_panel->dot_right, 0, 0);
    lv_obj_set_style_pad_all(s_panel->dot_right, 0, 0);
    lv_obj_remove_flag(s_panel->dot_right, LV_OBJ_FLAG_SCROLLABLE);

    /* Right: Battery % */
    s_panel->batt_lbl = lv_label_create(header);
    const status_bar_global_state_t *gs = status_bar_get_global_state();
    char bbuf[32];
    snprintf(bbuf, sizeof(bbuf), "%u%% %s", gs ? gs->battery_level : 85, (gs && gs->is_charging) ? LV_SYMBOL_CHARGE : "");
    lv_label_set_text(s_panel->batt_lbl, bbuf);
    lv_obj_set_style_text_color(s_panel->batt_lbl, is_mono ? lv_color_hex(0xFFFFFF) : theme_get()->text_muted, 0);
    lv_obj_set_style_text_font(s_panel->batt_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_align(s_panel->batt_lbl, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_width(s_panel->batt_lbl, 40);

    /* 4. Elastic Viewport (176x182) */
    lv_obj_t *viewport = lv_obj_create(container);
    s_panel->viewport = viewport;
    lv_obj_set_size(viewport, 176, 184);
    lv_obj_set_style_bg_opa(viewport, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(viewport, 0, 0);
    lv_obj_set_style_pad_all(viewport, 0, 0);
    lv_obj_remove_flag(viewport, LV_OBJ_FLAG_SCROLLABLE);

    /* Page 1: Notifications Deck */
    lv_obj_t *p_notif = lv_obj_create(viewport);
    s_panel->page_notif = p_notif;
    lv_obj_set_size(p_notif, 176, 184);
    lv_obj_set_style_bg_opa(p_notif, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(p_notif, 0, 0);
    lv_obj_set_style_pad_hor(p_notif, 4, 0);
    lv_obj_set_style_pad_ver(p_notif, 4, 0);
    lv_obj_set_style_pad_row(p_notif, 4, 0);
    lv_obj_set_flex_flow(p_notif, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(p_notif, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(p_notif, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(p_notif, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_set_scrollbar_mode(p_notif, LV_SCROLLBAR_MODE_OFF);

    /* Page 2: Quick Settings (3x2 Grid) */
    lv_obj_t *p_qs = lv_obj_create(viewport);
    s_panel->page_qs = p_qs;
    lv_obj_set_size(p_qs, 176, 184);
    lv_obj_set_style_bg_opa(p_qs, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(p_qs, 0, 0);
    lv_obj_set_style_pad_hor(p_qs, 5, 0);
    lv_obj_set_style_pad_ver(p_qs, 4, 0);
    lv_obj_set_flex_flow(p_qs, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(p_qs, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(p_qs, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(p_qs, LV_OBJ_FLAG_HIDDEN);

    /* 3x2 Grid Container */
    static const char *qs_icons[6] = { LV_SYMBOL_BLUETOOTH, LV_SYMBOL_EYE_OPEN, LV_SYMBOL_BELL, LV_SYMBOL_CHARGE, LV_SYMBOL_BATTERY_FULL, LV_SYMBOL_SETTINGS };

    lv_obj_t *grid = lv_obj_create(p_qs);
    s_panel->qs_grid = grid;
    lv_obj_set_size(grid, 166, 88);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_style_pad_row(grid, 4, 0);
    lv_obj_set_style_pad_column(grid, 4, 0);
    lv_obj_remove_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    for (uint8_t i = 0; i < 6; i++) {
        lv_obj_t *tile = lv_button_create(grid);
        s_panel->qs_tiles[i] = tile;
        lv_obj_set_size(tile, 50, 40);
        lv_obj_set_style_radius(tile, 0, 0);
        lv_obj_set_style_outline_width(tile, 0, 0);
        lv_obj_set_style_outline_pad(tile, 0, 0);
        lv_obj_set_style_pad_all(tile, 3, 0);
        lv_obj_set_style_pad_row(tile, 2, 0);
        lv_obj_set_style_border_width(tile, 1, 0);
        lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_set_style_border_color(tile, theme_get()->accent, LV_STATE_FOCUSED);
        lv_obj_set_style_border_width(tile, 2, LV_STATE_FOCUSED);
        lv_obj_set_style_radius(tile, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(tile, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_pad(tile, 0, LV_STATE_FOCUSED);
        lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        /* Tier 1: Centered Glyph / Symbol */
        lv_obj_t *t_icon = lv_label_create(tile);
        lv_label_set_text(t_icon, qs_icons[i]);
        lv_obj_set_style_text_font(t_icon, &lv_font_montserrat_12, 0);

        /* Tier 2: State / Value String */
        lv_obj_t *t_stat = lv_label_create(tile);
        lv_label_set_text(t_stat, "");
        lv_obj_set_style_text_color(t_stat, theme_get()->text_muted, 0);
        lv_obj_set_style_text_font(t_stat, &lv_font_montserrat_10, 0);

        lv_obj_set_user_data(tile, (void*)(uintptr_t)i);
        lv_obj_add_event_cb(tile, on_tile_clicked_event, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(tile, on_qs_tile_focus_cb, LV_EVENT_FOCUSED, NULL);

        update_qs_tile_visuals(i);
    }

    /* Mini Media Card below 3x2 Grid */
    if (tpl_media_has_active_session()) {
        lv_obj_t *media_card = lv_button_create(p_qs);
        s_panel->media_card = media_card;
        lv_obj_set_size(media_card, 166, 38);
        lv_obj_set_style_bg_color(media_card, theme_get()->card_color, 0);
        lv_obj_set_style_bg_opa(media_card, LV_OPA_COVER, 0);
        lv_color_t mc_border = is_mono ? lv_color_hex(0xFFFFFF) : (theme_is_light_mode() ? lv_color_hex(0xCCCCCC) : lv_color_hex(0x2D3340));
        lv_obj_set_style_border_color(media_card, mc_border, 0);
        lv_obj_set_style_border_width(media_card, 1, 0);
        lv_obj_set_style_radius(media_card, 0, 0);
        lv_obj_set_style_outline_width(media_card, 0, 0);
        lv_obj_set_style_outline_pad(media_card, 0, 0);
        lv_obj_set_style_pad_hor(media_card, 6, 0);
        lv_obj_set_style_pad_ver(media_card, 2, 0);
        lv_obj_set_style_margin_top(media_card, 4, 0);
        lv_obj_remove_flag(media_card, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_set_style_border_color(media_card, theme_get()->accent, LV_STATE_FOCUSED);
        lv_obj_set_style_border_width(media_card, 2, LV_STATE_FOCUSED);
        lv_obj_set_style_radius(media_card, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(media_card, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_pad(media_card, 0, LV_STATE_FOCUSED);
        lv_obj_set_flex_flow(media_card, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(media_card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t *m_icon = lv_label_create(media_card);
        s_panel->media_icon = m_icon;
        lv_label_set_text(m_icon, tpl_media_is_playing() ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
        lv_obj_set_style_text_color(m_icon, theme_get()->accent, 0);
        lv_obj_set_style_text_font(m_icon, &lv_font_montserrat_12, 0);

        /* Column for title and progress bar */
        lv_obj_t *m_col = lv_obj_create(media_card);
        lv_obj_set_size(m_col, 134, 30);
        lv_obj_set_style_bg_opa(m_col, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(m_col, 0, 0);
        lv_obj_set_style_pad_all(m_col, 0, 0);
        lv_obj_set_style_pad_left(m_col, 4, 0);
        lv_obj_remove_flag(m_col, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(m_col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(m_col, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

        lv_obj_t *m_title = lv_label_create(m_col);
        const char *head = tpl_media_get_headline();
        lv_label_set_text(m_title, (head && head[0]) ? head : "Music Track");
        lv_obj_set_style_text_color(m_title, theme_get()->text_primary, 0);
        lv_obj_set_style_text_font(m_title, veebha_font_get_default(), 0);
        lv_obj_set_width(m_title, 130);
        lv_label_set_long_mode(m_title, LV_LABEL_LONG_DOT);

        /* Slim 2-3px progress bar */
        lv_obj_t *m_bar = lv_bar_create(m_col);
        s_panel->media_bar = m_bar;
        lv_obj_set_size(m_bar, 130, 3);
        lv_bar_set_range(m_bar, 0, 100);
        lv_bar_set_value(m_bar, (int32_t)tpl_media_get_progress(), LV_ANIM_OFF);
        lv_color_t track_bg = is_mono ? lv_color_hex(0x444444) : (theme_is_light_mode() ? lv_color_hex(0xCCCCCC) : lv_color_hex(0x333A48));
        lv_obj_set_style_bg_color(m_bar, track_bg, 0);
        lv_obj_set_style_bg_opa(m_bar, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(m_bar, theme_get()->accent, LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(m_bar, LV_OPA_COVER, LV_PART_INDICATOR);
        lv_obj_set_style_radius(m_bar, 0, 0);
        lv_obj_set_style_radius(m_bar, 0, LV_PART_INDICATOR);
        lv_obj_remove_flag(m_bar, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_set_user_data(media_card, (void*)(uintptr_t)6);
        lv_obj_add_event_cb(media_card, on_tile_clicked_event, LV_EVENT_CLICKED, NULL);
    }

    /* Build Deck Content */
    rebuild_notif_deck();
    update_header_indicators();

    /* Redirect keypad indev to isolated group */
    if (s_panel->group) {
        lv_indev_set_group(hal_input_get_lv_indev(), s_panel->group);
    }

    s_panel_visible = true;
    printf("[QUICK_SETTINGS] KitKat Dual-Page Shade opened on lv_layer_top()\n");
}

void notif_panel_close(void)
{
    if (!s_panel || !s_panel_visible) return;

    s_panel_visible = false;

    /* Restore softkeys */
    softkey_set_actions(s_panel->saved_lsk_label, s_panel->saved_lsk_cb,
                        s_panel->saved_rsk_label, s_panel->saved_rsk_cb);

    /* Restore indev to window manager group */
    lv_indev_set_group(hal_input_get_lv_indev(), win_mgr_get_group());

    /* Restore focus */
    lv_group_t *group = win_mgr_get_group();
    if (group && s_panel->saved_focus && lv_obj_is_valid(s_panel->saved_focus)) {
        lv_group_focus_obj(s_panel->saved_focus);
    }

    /* Hide backdrop instead of deleting and recreating */
    if (s_panel->backdrop && lv_obj_is_valid(s_panel->backdrop)) {
        lv_obj_add_flag(s_panel->backdrop, LV_OBJ_FLAG_HIDDEN);
    }

    printf("[QUICK_SETTINGS] Shade hidden\n");
}

void notif_panel_destroy(void)
{
    if (!s_panel) return;
    if (s_panel_visible) {
        notif_panel_close();
    }
    if (s_panel->backdrop && lv_obj_is_valid(s_panel->backdrop)) {
        lv_obj_delete(s_panel->backdrop);
        s_panel->backdrop = NULL;
    }
    if (s_notif_panel_group) {
        lv_group_remove_all_objs(s_notif_panel_group);
    }
    s_panel->group = NULL;
    free(s_panel);
    s_panel = NULL;
    s_panel_visible = false;
}

void notif_panel_toggle(void)
{
    if (notif_panel_is_active()) {
        notif_panel_close();
    } else {
        notif_panel_show();
    }
}
