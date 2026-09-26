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

#include "apps/dialer/app_dialer.h"
#include "apps/telephony/app_incall.h"
#include "apps/common/mock_telephony.h"
#include "veebha_win_mgr.h"
#include "veebha_status_bar.h"
#include "veebha_softkeys.h"
#include "veebha_templates.h"
#include "sdk/include/veebha_i18n.h"
#include "sdk/text/font_fallback.h"
#include "boards/board_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIALER_MAX_DIGITS 32

typedef struct {
    lv_obj_t          *softkey_bar;
    lv_obj_t          *first_item;
    veebha_view_type_t view_type;
    char               title[WIN_MGR_LABEL_MAX];
    os_fullscreen_mode_t fullscreen_mode;
    bool               show_battery_hud;
    bool               keep_alive;
} dialer_screen_hdr_t;

static char s_dial_buffer[DIALER_MAX_DIGITS + 1] = {0};
static lv_obj_t *s_dial_screen = NULL;
static lv_obj_t *s_digit_label = NULL;
static lv_obj_t *s_match_label = NULL;
static dialer_screen_hdr_t *s_dialer_hdr = NULL;

static void update_dialer_display(void)
{
    if (!s_digit_label || !lv_obj_is_valid(s_digit_label)) return;

    if (s_dial_buffer[0] != '\0') {
        lv_label_set_text(s_digit_label, s_dial_buffer);
        softkey_set_actions("Call", app_dialer_start_call, "Clear", app_dialer_backspace);
    } else {
        lv_label_set_text(s_digit_label, "Enter Number");
        softkey_set_actions("Call", app_dialer_start_call, "Back", (softkey_callback_t)win_mgr_pop);
    }

    if (s_match_label && lv_obj_is_valid(s_match_label)) {
        if (s_dial_buffer[0] != '\0') {
            const contact_record_t *matched = telephony_find_contact_by_number(s_dial_buffer);
            if (matched) {
                char match_text[64];
                snprintf(match_text, sizeof(match_text), "%s (%s)", matched->name, matched->number);
                lv_label_set_text(s_match_label, match_text);
                lv_obj_clear_flag(s_match_label, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(s_match_label, LV_OBJ_FLAG_HIDDEN);
            }
        } else {
            lv_obj_add_flag(s_match_label, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void on_dialer_screen_delete(lv_event_t *e)
{
    (void)e;
    s_dial_screen = NULL;
    s_digit_label = NULL;
    s_match_label = NULL;
    if (s_dialer_hdr) {
        free(s_dialer_hdr);
        s_dialer_hdr = NULL;
    }
    s_dial_buffer[0] = '\0';
}

void app_dialer_open(const char *initial_digits)
{
    /* If dialer is already open, simply append or replace */
    if (s_dial_screen && lv_obj_is_valid(s_dial_screen) && app_dialer_is_active()) {
        if (initial_digits) {
            for (size_t i = 0; initial_digits[i]; i++) {
                app_dialer_handle_digit(initial_digits[i]);
            }
        }
        return;
    }

    /* 1. Root Screen Container */
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, CONFIG_DISP_HOR_RES, CONFIG_DISP_VER_RES);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x121212), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    s_dial_screen = screen;
    s_dial_buffer[0] = '\0';

    s_dialer_hdr = (dialer_screen_hdr_t *)calloc(1, sizeof(dialer_screen_hdr_t));
    if (s_dialer_hdr) {
        s_dialer_hdr->softkey_bar = NULL;
        s_dialer_hdr->first_item = NULL;
        s_dialer_hdr->view_type = VEEBHA_VIEW_TYPE_DIALER;
        s_dialer_hdr->fullscreen_mode = OS_FULLSCREEN_NONE;
        s_dialer_hdr->show_battery_hud = false;
        strncpy(s_dialer_hdr->title, "Phone", sizeof(s_dialer_hdr->title) - 1);
        s_dialer_hdr->title[sizeof(s_dialer_hdr->title) - 1] = '\0';
    }
    lv_obj_set_user_data(screen, s_dialer_hdr);
    lv_obj_add_event_cb(screen, on_dialer_screen_delete, LV_EVENT_DELETE, NULL);

    /* 2. Zone A: Fixed 18px Top Status Bar */
    status_bar_create(screen, "Phone");

    /* 3. Zone B: Elastic 184px Content Viewport */
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_size(content, lv_pct(100), 0);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_hor(content, 6, 0);
    lv_obj_set_style_pad_ver(content, 0, 0);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Phone icon accent */
    lv_obj_t *icon_lbl = lv_label_create(content);
    lv_label_set_text(icon_lbl, LV_SYMBOL_CALL);
    lv_obj_set_style_text_color(icon_lbl, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(icon_lbl, veebha_font_get_default(), 0);
    lv_obj_set_style_pad_bottom(icon_lbl, 8, 0);

    /* Large Centered Digit Display */
    s_digit_label = lv_label_create(content);
    lv_obj_set_width(s_digit_label, lv_pct(100));
    lv_label_set_long_mode(s_digit_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(s_digit_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(s_digit_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(s_digit_label, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(s_digit_label, "Enter Number");

    /* Matching Contact Label */
    s_match_label = lv_label_create(content);
    lv_obj_set_width(s_match_label, lv_pct(100));
    lv_label_set_long_mode(s_match_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(s_match_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(s_match_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_match_label, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_pad_top(s_match_label, 6, 0);
    lv_obj_add_flag(s_match_label, LV_OBJ_FLAG_HIDDEN);

    /* 4. Zone C: Bottom 20px Softkey Bar */
    lv_obj_t *bar = softkey_bar_create(screen, "Call", "Back");
    if (s_dialer_hdr) {
        s_dialer_hdr->softkey_bar = bar;
    }

    win_mgr_push(screen, "Call", app_dialer_start_call, "Back", (softkey_callback_t)win_mgr_pop);

    /* Setup RSK long-press for clear all */
    softkey_set_rsk_long_action(app_dialer_clear_all);

    if (initial_digits) {
        for (size_t i = 0; initial_digits[i]; i++) {
            app_dialer_handle_digit(initial_digits[i]);
        }
    } else {
        update_dialer_display();
    }
}

void app_dialer_handle_digit(char digit)
{
    if ((digit < '0' || digit > '9') && digit != '*' && digit != '#' && digit != '+') {
        return;
    }

    size_t len = strlen(s_dial_buffer);
    if (len < DIALER_MAX_DIGITS) {
        s_dial_buffer[len] = digit;
        s_dial_buffer[len + 1] = '\0';
        update_dialer_display();
    }
}

void app_dialer_backspace(void)
{
    size_t len = strlen(s_dial_buffer);
    if (len > 0) {
        s_dial_buffer[len - 1] = '\0';
        update_dialer_display();
    } else {
        win_mgr_pop();
    }
}

void app_dialer_clear_all(void)
{
    s_dial_buffer[0] = '\0';
    update_dialer_display();
}

void app_dialer_end_call(void)
{
    app_incall_end();
}

void app_dialer_start_call_to(const char *name, const char *number)
{
    if (!number || number[0] == '\0') return;
    app_incall_start(name, number, CALL_TYPE_OUTGOING);
}

void app_dialer_start_call(void)
{
    if (s_dial_buffer[0] == '\0') {
        printf("[CALL] Dial buffer empty, cannot initiate call\n");
        return;
    }
    app_dialer_start_call_to(NULL, s_dial_buffer);
}

bool app_dialer_is_active(void)
{
    return win_mgr_get_active_view_type() == VEEBHA_VIEW_TYPE_DIALER;
}

bool app_dialer_is_in_call(void)
{
    return app_incall_is_active();
}

const char * app_dialer_get_digits(void)
{
    return s_dial_buffer;
}
