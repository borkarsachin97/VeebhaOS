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

#include "veebha_status_bar.h"
#include "veebha_theme.h"
#include "veebha_win_mgr.h"
#include "apps/home/app_idle.h"
#include "boards/board_config.h"
#include <stdio.h>

static status_bar_global_state_t s_global_state = {
    .hours = 12,
    .minutes = 0,
    .year = 2026,
    .month = 9,
    .day = 23,
    .bt_enabled = false,
    .bt_connected = false,
    .tethering_active = false,
    .usb_mode = 0,
    .silent_mode = false,
    .alarm_enabled = false,
    .battery_level = 85,
    .is_charging = false
};

static lv_obj_t *s_battery_hud = NULL;
static lv_obj_t *s_battery_hud_lbl = NULL;

static void status_bar_refresh_obj(lv_obj_t *bar)
{
    if (!bar || !lv_obj_is_valid(bar)) return;
    if (lv_obj_get_child_count(bar) < 3) return;

    const os_theme_tokens_t *tokens = theme_get();
    os_theme_id_t tid = theme_get_palette();
    bool is_mono = (tid == THEME_HIGH_CONTRAST_BW);

    lv_color_t sb_bg = (is_mono || tid == THEME_OLED_BLACK)
                        ? lv_color_hex(0x000000)
                        : (tokens->is_light ? lv_color_hex(0xDCDCDC) : lv_color_hex(0x181A20));
    lv_obj_set_style_bg_color(bar, sb_bg, 0);

    lv_color_t sig_color = is_mono ? lv_color_hex(0xFFFFFF) : tokens->accent;
    lv_color_t net_color = is_mono ? lv_color_hex(0xFFFFFF) : tokens->text_muted;
    lv_color_t tether_color = is_mono ? lv_color_hex(0xFFFFFF) : tokens->accent;
    lv_color_t bat_color = is_mono ? lv_color_hex(0xFFFFFF) : (tokens->is_light ? tokens->accent : lv_color_hex(0x00E676));
    lv_color_t alarm_color = is_mono ? lv_color_hex(0xFFFFFF) : lv_color_hex(0xF5A623);
    lv_color_t bt_color = is_mono ? lv_color_hex(0xFFFFFF) : tokens->accent;
    lv_color_t headset_color = is_mono ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x00E5FF);
    lv_color_t usb_color = is_mono ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x00E676);
    lv_color_t mute_color = is_mono ? lv_color_hex(0xFFFFFF) : lv_color_hex(0xFF9800);

    /* Zone 1: Left Tray (Signal, 4G, Tethering) */
    lv_obj_t *left_tray = lv_obj_get_child(bar, 0);
    if (left_tray && lv_obj_is_valid(left_tray) && lv_obj_get_child_count(left_tray) >= 3) {
        lv_obj_t *signal_box = lv_obj_get_child(left_tray, 0);
        if (signal_box && lv_obj_is_valid(signal_box)) {
            uint32_t bar_cnt = lv_obj_get_child_count(signal_box);
            for (uint32_t b = 0; b < bar_cnt; b++) {
                lv_obj_t *bar_elem = lv_obj_get_child(signal_box, b);
                if (bar_elem && lv_obj_is_valid(bar_elem)) {
                    lv_obj_set_style_bg_color(bar_elem, sig_color, 0);
                }
            }
        }

        lv_obj_t *net_lbl = lv_obj_get_child(left_tray, 1);
        if (net_lbl && lv_obj_is_valid(net_lbl)) {
            lv_obj_set_style_text_color(net_lbl, net_color, 0);
        }

        lv_obj_t *tether = lv_obj_get_child(left_tray, 2);
        if (tether && lv_obj_is_valid(tether)) {
            lv_obj_set_style_text_color(tether, tether_color, 0);
            if (s_global_state.tethering_active) {
                lv_obj_clear_flag(tether, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(tether, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }

    /* Zone 2: Center Clock */
    lv_obj_t *clock_lbl = lv_obj_get_child(bar, 1);
    if (clock_lbl && lv_obj_is_valid(clock_lbl)) {
        char init_time[16];
        snprintf(init_time, sizeof(init_time), "%02d:%02d", (int)s_global_state.hours, (int)s_global_state.minutes);
        lv_label_set_text(clock_lbl, init_time);
        lv_obj_set_style_text_color(clock_lbl, tokens->text_primary, 0);
    }

    /* Zone 3: Right Tray (Alarm, Bluetooth, Headset, USB, Mute, Battery) */
    lv_obj_t *right_tray = lv_obj_get_child(bar, 2);
    if (right_tray && lv_obj_is_valid(right_tray) && lv_obj_get_child_count(right_tray) >= 6) {
        lv_obj_t *alarm_icon = lv_obj_get_child(right_tray, 0);
        lv_obj_t *bt = lv_obj_get_child(right_tray, 1);
        lv_obj_t *headset = lv_obj_get_child(right_tray, 2);
        lv_obj_t *usb = lv_obj_get_child(right_tray, 3);
        lv_obj_t *mute = lv_obj_get_child(right_tray, 4);
        lv_obj_t *bat_box = lv_obj_get_child(right_tray, 5);

        uint8_t active_count = 0;

        /* Priority 1: Mute / Silent Mode */
        bool show_mute = false;
        if (s_global_state.silent_mode && active_count < 3) {
            show_mute = true;
            active_count++;
        }

        /* Priority 2: USB Mass Storage / Cable */
        bool show_usb = false;
        if ((s_global_state.usb_mode == 2 || s_global_state.usb_mode == 3) && active_count < 3) {
            show_usb = true;
            active_count++;
        }

        /* Priority 3: Audio Headset / Bluetooth */
        bool show_headset = false;
        bool show_bt = false;
        if (s_global_state.bt_enabled && s_global_state.bt_connected && active_count < 3) {
            show_headset = true;
            active_count++;
        } else if (s_global_state.bt_enabled && !s_global_state.bt_connected && active_count < 3) {
            show_bt = true;
            active_count++;
        }

        /* Priority 4: Alarm Clock Armed */
        bool show_alarm = false;
        if (s_global_state.alarm_enabled && active_count < 3) {
            show_alarm = true;
            active_count++;
        }

        /* Apply visibility and colors */
        if (alarm_icon && lv_obj_is_valid(alarm_icon)) {
            lv_obj_set_style_text_color(alarm_icon, alarm_color, 0);
            if (show_alarm) lv_obj_clear_flag(alarm_icon, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(alarm_icon, LV_OBJ_FLAG_HIDDEN);
        }

        if (bt && lv_obj_is_valid(bt)) {
            lv_obj_set_style_text_color(bt, bt_color, 0);
            if (show_bt) lv_obj_clear_flag(bt, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(bt, LV_OBJ_FLAG_HIDDEN);
        }

        if (headset && lv_obj_is_valid(headset)) {
            lv_obj_set_style_text_color(headset, headset_color, 0);
            if (show_headset) lv_obj_clear_flag(headset, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(headset, LV_OBJ_FLAG_HIDDEN);
        }

        if (usb && lv_obj_is_valid(usb)) {
            lv_obj_set_style_text_color(usb, usb_color, 0);
            if (show_usb) lv_obj_clear_flag(usb, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(usb, LV_OBJ_FLAG_HIDDEN);
        }

        if (mute && lv_obj_is_valid(mute)) {
            lv_obj_set_style_text_color(mute, mute_color, 0);
            if (show_mute) lv_obj_clear_flag(mute, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(mute, LV_OBJ_FLAG_HIDDEN);
        }

        /* Battery Box & Level Fill (Always on Far Right) */
        if (bat_box && lv_obj_is_valid(bat_box) && lv_obj_get_child_count(bat_box) >= 2) {
            lv_obj_t *bat_fill = lv_obj_get_child(bat_box, 0);
            lv_obj_t *charge_icon = lv_obj_get_child(bat_box, 1);

            lv_obj_set_style_border_color(bat_box, bat_color, 0);

            if (bat_fill && lv_obj_is_valid(bat_fill)) {
                int32_t fill_w = (int32_t)((s_global_state.battery_level * 12) / 100);
                if (fill_w < 1) fill_w = 1;
                if (fill_w > 12) fill_w = 12;
                lv_obj_set_width(bat_fill, fill_w);
                lv_obj_set_style_bg_color(bat_fill, bat_color, 0);
            }

            if (charge_icon && lv_obj_is_valid(charge_icon)) {
                lv_obj_set_style_text_color(charge_icon, bat_color, 0);
                if (s_global_state.is_charging) {
                    lv_obj_clear_flag(charge_icon, LV_OBJ_FLAG_HIDDEN);
                } else {
                    lv_obj_add_flag(charge_icon, LV_OBJ_FLAG_HIDDEN);
                }
            }
        }
    }
}

static void status_bar_find_and_refresh(lv_obj_t *obj)
{
    if (!obj || !lv_obj_is_valid(obj)) return;

    /* A status bar container has 3 children (left tray, clock, right tray) and height CONFIG_STATUS_BAR_HEIGHT */
    if (lv_obj_get_child_count(obj) == 3 && lv_obj_get_height(obj) == CONFIG_STATUS_BAR_HEIGHT) {
        status_bar_refresh_obj(obj);
        return;
    }

    uint32_t cnt = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < cnt; i++) {
        status_bar_find_and_refresh(lv_obj_get_child(obj, i));
    }
}

static void status_bar_screen_iter_cb(lv_obj_t *screen, void *user_data)
{
    (void)user_data;
    if (!screen || !lv_obj_is_valid(screen)) return;
    status_bar_find_and_refresh(screen);
}

void status_bar_update_all(void)
{
    win_mgr_for_each_screen(status_bar_screen_iter_cb, NULL);
    app_idle_update();
}

const status_bar_global_state_t * status_bar_get_global_state(void)
{
    return &s_global_state;
}

lv_obj_t * status_bar_create(lv_obj_t *parent, const char *title)
{
    LV_UNUSED(title);
    if (!parent) return NULL;

    /* Zone A: Fixed 18px Top Status Bar */
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, lv_pct(100), CONFIG_STATUS_BAR_HEIGHT);
    lv_obj_set_style_bg_color(bar, theme_is_light_mode() ? lv_color_hex(0xDCDCDC) : lv_color_hex(0x181A20), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    /* 1. Zone 1: Left Container (Signal & Network): Stepped 4-bar cellular meter + "4G" label + Tether icon */
    lv_obj_t *left_tray = lv_obj_create(bar);
    lv_obj_set_size(left_tray, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(left_tray, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left_tray, 0, 0);
    lv_obj_set_style_pad_all(left_tray, 0, 0);
    lv_obj_set_style_pad_column(left_tray, 3, 0);
    lv_obj_remove_flag(left_tray, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(left_tray, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(left_tray, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(left_tray, LV_ALIGN_LEFT_MID, 4, 0);

    /* Signal Bars Container (bottom aligned) */
    lv_obj_t *signal_box = lv_obj_create(left_tray);
    lv_obj_set_size(signal_box, 14, 10);
    lv_obj_set_style_bg_opa(signal_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(signal_box, 0, 0);
    lv_obj_set_style_pad_all(signal_box, 0, 0);
    lv_obj_set_style_pad_column(signal_box, 1, 0);
    lv_obj_remove_flag(signal_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(signal_box, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(signal_box, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);

    static const uint8_t bar_heights[4] = { 3, 5, 7, 9 };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *bar_elem = lv_obj_create(signal_box);
        lv_obj_set_size(bar_elem, 2, bar_heights[i]);
        lv_obj_set_style_bg_color(bar_elem, lv_color_hex(0x00E676), 0);
        lv_obj_set_style_bg_opa(bar_elem, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(bar_elem, 0, 0);
        lv_obj_set_style_radius(bar_elem, 0, 0);
        lv_obj_remove_flag(bar_elem, LV_OBJ_FLAG_SCROLLABLE);
    }

    /* "4G" Network Type Label */
    lv_obj_t *net_lbl = lv_label_create(left_tray);
    lv_label_set_text(net_lbl, "4G");
    lv_obj_set_style_text_color(net_lbl, theme_get()->text_muted, 0);
    lv_obj_set_style_text_font(net_lbl, &lv_font_montserrat_10, 0);

    /* Tethering Hotspot Icon */
    lv_obj_t *tether_icon = lv_label_create(left_tray);
    lv_label_set_text(tether_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(tether_icon, theme_get()->accent, 0);
    lv_obj_set_style_text_font(tether_icon, &lv_font_montserrat_10, 0);
    lv_obj_add_flag(tether_icon, LV_OBJ_FLAG_HIDDEN);

    /* 2. Zone 2: Center Clock Label (Pinned strictly to dead center) */
    lv_obj_t *clock_lbl = lv_label_create(bar);
    char init_time[16];
    snprintf(init_time, sizeof(init_time), "%02d:%02d", (int)s_global_state.hours, (int)s_global_state.minutes);
    lv_label_set_text(clock_lbl, init_time);
    lv_obj_set_width(clock_lbl, LV_SIZE_CONTENT);
    lv_obj_set_height(clock_lbl, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(clock_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(clock_lbl, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(clock_lbl, theme_get()->text_primary, 0);
    lv_obj_set_style_text_font(clock_lbl, &lv_font_montserrat_12, 0);
    lv_obj_align(clock_lbl, LV_ALIGN_CENTER, 0, 0);

    /* 3. Zone 3: Right Container (Hardware Indicators: Alarm, Bluetooth, Headset, USB, Mute, Battery) */
    lv_obj_t *right_tray = lv_obj_create(bar);
    lv_obj_set_size(right_tray, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(right_tray, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right_tray, 0, 0);
    lv_obj_set_style_pad_all(right_tray, 0, 0);
    lv_obj_set_style_pad_column(right_tray, 3, 0);
    lv_obj_remove_flag(right_tray, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(right_tray, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(right_tray, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(right_tray, LV_ALIGN_RIGHT_MID, -4, 0);

    /* Child 0: Alarm Armed Indicator */
    lv_obj_t *alarm_icon = lv_label_create(right_tray);
    lv_label_set_text(alarm_icon, LV_SYMBOL_BELL);
    lv_obj_set_style_text_color(alarm_icon, lv_color_hex(0xF5A623), 0); /* Amber/Gold */
    lv_obj_set_style_text_font(alarm_icon, &lv_font_montserrat_12, 0);
    lv_obj_add_flag(alarm_icon, LV_OBJ_FLAG_HIDDEN);

    /* Child 1: Bluetooth Indicator (Idle) */
    lv_obj_t *bt_icon = lv_label_create(right_tray);
    lv_label_set_text(bt_icon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(bt_icon, theme_get()->accent, 0);
    lv_obj_set_style_text_font(bt_icon, &lv_font_montserrat_12, 0);
    lv_obj_add_flag(bt_icon, LV_OBJ_FLAG_HIDDEN);

    /* Child 2: Audio Headset Indicator (Connected) */
    lv_obj_t *headset_icon = lv_label_create(right_tray);
    lv_label_set_text(headset_icon, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_color(headset_icon, lv_color_hex(0x00E5FF), 0); /* Cyan */
    lv_obj_set_style_text_font(headset_icon, &lv_font_montserrat_12, 0);
    lv_obj_add_flag(headset_icon, LV_OBJ_FLAG_HIDDEN);

    /* Child 3: USB Mass Storage / Cable Indicator */
    lv_obj_t *usb_icon = lv_label_create(right_tray);
    lv_label_set_text(usb_icon, LV_SYMBOL_USB);
    lv_obj_set_style_text_color(usb_icon, lv_color_hex(0x00E676), 0); /* Green */
    lv_obj_set_style_text_font(usb_icon, &lv_font_montserrat_12, 0);
    lv_obj_add_flag(usb_icon, LV_OBJ_FLAG_HIDDEN);

    /* Child 4: Silent / Mute Indicator */
    lv_obj_t *mute_icon = lv_label_create(right_tray);
    lv_label_set_text(mute_icon, LV_SYMBOL_MUTE);
    lv_obj_set_style_text_color(mute_icon, lv_color_hex(0xFF9800), 0); /* Orange */
    lv_obj_set_style_text_font(mute_icon, &lv_font_montserrat_12, 0);
    lv_obj_add_flag(mute_icon, LV_OBJ_FLAG_HIDDEN);

    /* Child 5: Battery Box: 18x10px container with green fill + charging bolt */
    lv_obj_t *bat_box = lv_obj_create(right_tray);
    lv_obj_set_size(bat_box, 18, 10);
    lv_obj_set_style_bg_opa(bat_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(bat_box, lv_color_hex(0x00E676), 0);
    lv_obj_set_style_border_width(bat_box, 1, 0);
    lv_obj_set_style_radius(bat_box, 2, 0);
    lv_obj_set_style_pad_all(bat_box, 1, 0);
    lv_obj_remove_flag(bat_box, LV_OBJ_FLAG_SCROLLABLE);

    /* Inner green fill representing charge level */
    lv_obj_t *bat_fill = lv_obj_create(bat_box);
    lv_obj_set_size(bat_fill, 10, 6);
    lv_obj_align(bat_fill, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(bat_fill, lv_color_hex(0x00E676), 0);
    lv_obj_set_style_bg_opa(bat_fill, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bat_fill, 0, 0);
    lv_obj_set_style_radius(bat_fill, 1, 0);
    lv_obj_remove_flag(bat_fill, LV_OBJ_FLAG_SCROLLABLE);

    /* Charging lightning bolt indicator */
    lv_obj_t *bat_charge_icon = lv_label_create(bat_box);
    lv_label_set_text(bat_charge_icon, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_color(bat_charge_icon, lv_color_hex(0x00E676), 0);
    lv_obj_set_style_text_font(bat_charge_icon, &lv_font_montserrat_10, 0);
    lv_obj_center(bat_charge_icon);
    lv_obj_add_flag(bat_charge_icon, LV_OBJ_FLAG_HIDDEN);

    /* Synchronize newly created status bar against global state machine */
    status_bar_refresh_obj(bar);

    return bar;
}

void status_bar_set_title(lv_obj_t *bar, const char *title)
{
    LV_UNUSED(bar);
    LV_UNUSED(title);
}

void status_bar_set_time(lv_obj_t *bar, const char *time_str)
{
    if (!bar || !lv_obj_is_valid(bar)) return;
    if (lv_obj_get_child_count(bar) > 1) {
        lv_obj_t *clock_lbl = lv_obj_get_child(bar, 1);
        if (clock_lbl && lv_obj_is_valid(clock_lbl)) {
            lv_label_set_text(clock_lbl, time_str ? time_str : "");
        }
    }
}

void status_bar_set_indicators(lv_obj_t *bar, bool bluetooth_on, bool silent_on)
{
    if (s_global_state.bt_enabled == bluetooth_on && s_global_state.silent_mode == silent_on) return;
    s_global_state.bt_enabled = bluetooth_on;
    s_global_state.silent_mode = silent_on;
    if (bar) {
        status_bar_refresh_obj(bar);
    } else {
        status_bar_update_all();
    }
}

void status_bar_set_bt_state(bool enabled, bool connected)
{
    if (s_global_state.bt_enabled == enabled && s_global_state.bt_connected == connected) return;
    s_global_state.bt_enabled = enabled;
    s_global_state.bt_connected = connected;
    status_bar_update_all();
}

void status_bar_set_usb_mode(uint8_t mode)
{
    if (s_global_state.usb_mode == mode) return;
    s_global_state.usb_mode = mode;
    status_bar_update_all();
}

void status_bar_set_tethering(bool enabled)
{
    if (s_global_state.tethering_active == enabled) return;
    s_global_state.tethering_active = enabled;
    status_bar_update_all();
}

void status_bar_set_silent(bool silent)
{
    if (s_global_state.silent_mode == silent) return;
    s_global_state.silent_mode = silent;
    status_bar_update_all();
}

void status_bar_set_alarm(bool enabled)
{
    if (s_global_state.alarm_enabled == enabled) return;
    s_global_state.alarm_enabled = enabled;
    status_bar_update_all();
}

void status_bar_set_battery(uint8_t level, bool is_charging)
{
    if (s_global_state.battery_level == level && s_global_state.is_charging == is_charging) return;
    s_global_state.battery_level = level;
    s_global_state.is_charging = is_charging;
    status_bar_update_all();
}

void status_bar_update_clock(lv_obj_t *bar, uint8_t hours, uint8_t mins)
{
    if (s_global_state.hours == hours && s_global_state.minutes == mins) return;
    s_global_state.hours = hours;
    s_global_state.minutes = mins;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", (int)hours, (int)mins);
    if (bar) {
        status_bar_set_time(bar, buf);
    }
    status_bar_update_all();
}

void status_bar_get_rtc_time(uint8_t *hours, uint8_t *mins)
{
    if (hours) *hours = s_global_state.hours;
    if (mins) *mins = s_global_state.minutes;
}

void status_bar_set_rtc_time(uint8_t hours, uint8_t mins)
{
    if (s_global_state.hours == hours && s_global_state.minutes == mins) return;
    s_global_state.hours = hours;
    s_global_state.minutes = mins;
    status_bar_update_all();
}

void status_bar_get_rtc_date(uint16_t *year, uint8_t *month, uint8_t *day)
{
    if (year) *year = s_global_state.year;
    if (month) *month = s_global_state.month;
    if (day) *day = s_global_state.day;
}

void status_bar_set_rtc_date(uint16_t year, uint8_t month, uint8_t day)
{
    if (s_global_state.year == year && s_global_state.month == month && s_global_state.day == day) return;
    s_global_state.year = year;
    s_global_state.month = month;
    s_global_state.day = day;
    status_bar_update_all();
}

void status_bar_set_battery_hud_visible(bool visible)
{
    if (visible) {
        const os_theme_tokens_t *tokens = theme_get();
        os_theme_id_t tid = theme_get_palette();
        bool is_mono = (tid == THEME_HIGH_CONTRAST_BW);
        lv_color_t hud_text = is_mono ? lv_color_hex(0xFFFFFF) : (tokens->is_light ? tokens->accent : lv_color_hex(0x00E676));
        lv_color_t hud_border = is_mono ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x334155);
        lv_color_t hud_bg = (is_mono || tid == THEME_OLED_BLACK) ? lv_color_hex(0x000000) : lv_color_hex(0x0F172A);

        if (!s_battery_hud || !lv_obj_is_valid(s_battery_hud)) {
            lv_obj_t *top_layer = lv_layer_top();
            s_battery_hud = lv_obj_create(top_layer);
            lv_obj_set_size(s_battery_hud, 38, 12);
            lv_obj_align(s_battery_hud, LV_ALIGN_TOP_RIGHT, -2, 2);
            lv_obj_set_style_bg_color(s_battery_hud, hud_bg, 0);
            lv_obj_set_style_bg_opa(s_battery_hud, is_mono ? LV_OPA_COVER : LV_OPA_60, 0);
            lv_obj_set_style_border_color(s_battery_hud, hud_border, 0);
            lv_obj_set_style_border_width(s_battery_hud, 1, 0);
            lv_obj_set_style_radius(s_battery_hud, 3, 0);
            lv_obj_set_style_pad_all(s_battery_hud, 0, 0);
            lv_obj_remove_flag(s_battery_hud, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

            s_battery_hud_lbl = lv_label_create(s_battery_hud);
            char hbuf[32];
            snprintf(hbuf, sizeof(hbuf), "%u%% %s",
                     (unsigned int)s_global_state.battery_level,
                     s_global_state.is_charging ? LV_SYMBOL_CHARGE : "");
            lv_label_set_text(s_battery_hud_lbl, hbuf);
            lv_obj_center(s_battery_hud_lbl);
            lv_obj_set_style_text_color(s_battery_hud_lbl, hud_text, 0);
            lv_obj_set_style_text_font(s_battery_hud_lbl, &lv_font_montserrat_10, 0);
        } else if (s_battery_hud_lbl && lv_obj_is_valid(s_battery_hud_lbl)) {
            char hbuf[32];
            snprintf(hbuf, sizeof(hbuf), "%u%% %s",
                     (unsigned int)s_global_state.battery_level,
                     s_global_state.is_charging ? LV_SYMBOL_CHARGE : "");
            lv_label_set_text(s_battery_hud_lbl, hbuf);
            lv_obj_set_style_text_color(s_battery_hud_lbl, hud_text, 0);
            lv_obj_set_style_border_color(s_battery_hud, hud_border, 0);
            lv_obj_set_style_bg_color(s_battery_hud, hud_bg, 0);
        }
        lv_obj_clear_flag(s_battery_hud, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_battery_hud);
    } else {
        if (s_battery_hud && lv_obj_is_valid(s_battery_hud)) {
            lv_obj_add_flag(s_battery_hud, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

bool status_bar_is_battery_hud_visible(void)
{
    return s_battery_hud && lv_obj_is_valid(s_battery_hud) && !lv_obj_has_flag(s_battery_hud, LV_OBJ_FLAG_HIDDEN);
}
