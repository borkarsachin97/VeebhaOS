/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "vapp_unit_converter.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_templates.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_log.h"
#include "sdk/text/font_fallback.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "VAPP_CONV"

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *content;
    uint8_t   category; /* 0: Length, 1: Weight, 2: Temp */
    int32_t   value;
    lv_obj_t *lbl_display;
} unit_conv_state_t;

static void unit_conv_render(unit_conv_state_t *st)
{
    if (!st || !st->lbl_display) return;
    char buf[128];
    if (st->category == 0) {
        /* Meters to Feet: 1m = 3.28 ft */
        int32_t ft_int = (st->value * 328) / 100;
        int32_t ft_dec = ((st->value * 328) % 100);
        snprintf(buf, sizeof(buf), "LENGTH\n\n%d m =\n%d.%02d ft\n\n[0-9] Input\n[LSK] Next Unit", (int)st->value, (int)ft_int, (int)ft_dec);
    } else if (st->category == 1) {
        /* Kg to Lbs: 1kg = 2.20 lbs */
        int32_t lbs_int = (st->value * 220) / 100;
        int32_t lbs_dec = ((st->value * 220) % 100);
        snprintf(buf, sizeof(buf), "WEIGHT\n\n%d kg =\n%d.%02d lbs\n\n[0-9] Input\n[LSK] Next Unit", (int)st->value, (int)lbs_int, (int)lbs_dec);
    } else {
        /* Celsius to Fahrenheit: F = (C * 9 / 5) + 32 */
        int32_t f = (st->value * 9) / 5 + 32;
        snprintf(buf, sizeof(buf), "TEMP\n\n%d °C =\n%d °F\n\n[0-9] Input\n[LSK] Next Unit", (int)st->value, (int)f);
    }
    lv_label_set_text(st->lbl_display, buf);
}

static void unit_conv_on_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    unit_conv_state_t *st = (unit_conv_state_t *)lv_event_get_user_data(e);
    if (!st) return;

    if (code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if (key >= '0' && key <= '9') {
            st->value = (st->value * 10) + (key - '0');
            if (st->value > 9999) st->value = (int32_t)(key - '0');
            unit_conv_render(st);
        } else if (key == LV_KEY_UP || key == LV_KEY_RIGHT || key == LV_KEY_NEXT) {
            st->value += 1;
            unit_conv_render(st);
        } else if (key == LV_KEY_DOWN || key == LV_KEY_LEFT || key == LV_KEY_PREV) {
            if (st->value >= 1) st->value -= 1;
            unit_conv_render(st);
        }
    } else if (code == LV_EVENT_DELETE) {
        lv_group_t *g = win_mgr_get_group();
        if (g) {
            lv_group_set_editing(g, false);
        }
        win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)lv_obj_get_user_data(st->screen);
        if (hdr) {
            free(hdr);
            lv_obj_set_user_data(st->screen, NULL);
        }
        free(st);
    }
}

static void unit_conv_toggle_cat(void)
{
    lv_obj_t *top = lv_scr_act();
    unit_conv_state_t *st = (unit_conv_state_t *)lv_obj_get_user_data(top);
    if (!st) return;
    st->category = (st->category + 1) % 3;
    unit_conv_render(st);
}

static void on_conv_exit_action(void)
{
    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_set_editing(g, false);
    }
    win_mgr_pop();
}

void vapp_unit_converter_launch(const vapp_package_t *pkg)
{
    unit_conv_state_t *st = (unit_conv_state_t *)calloc(1, sizeof(unit_conv_state_t));
    if (!st) return;
    st->value = 10;
    st->category = 0;

    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, 176, 220);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x121212), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)calloc(1, sizeof(win_mgr_screen_hdr_t));
    if (hdr) {
        hdr->view_type = VEEBHA_VIEW_TYPE_GENERIC;
        strncpy(hdr->title, pkg ? pkg->header.name : "Converter", sizeof(hdr->title) - 1);
        lv_obj_set_user_data(screen, hdr);
    }
    st->screen = screen;
    lv_obj_add_event_cb(screen, unit_conv_on_event, LV_EVENT_ALL, st);

    lv_obj_t *content = lv_button_create(screen);
    st->content = content;
    lv_obj_set_size(content, lv_pct(100), 0);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(content, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 8, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_add_event_cb(content, unit_conv_on_event, LV_EVENT_KEY, st);

    st->lbl_display = lv_label_create(content);
    lv_obj_set_style_text_color(st->lbl_display, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(st->lbl_display, veebha_font_get_default(), 0);
    lv_obj_set_style_text_align(st->lbl_display, LV_TEXT_ALIGN_CENTER, 0);
    unit_conv_render(st);

    lv_obj_t *sk = softkey_bar_create(screen, "Unit", "Back");
    if (hdr) hdr->softkey_bar = sk;

    win_mgr_push(screen, "Unit", unit_conv_toggle_cat, "Back", on_conv_exit_action);

    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_add_obj(g, content);
        lv_group_focus_obj(content);
        lv_group_set_editing(g, true);
    }
}
