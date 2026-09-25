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

#include "usb_select.h"
#include "veebha_connectivity.h"
#include "veebha_win_mgr.h"
#include "veebha_softkeys.h"
#include "veebha_theme.h"
#include "veebha_log.h"
#include "drivers/hal_input.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TAG "USB_SELECT"

typedef struct {
    lv_obj_t          *backdrop;
    lv_group_t        *group;
    lv_obj_t          *options[4];
    lv_obj_t          *saved_focus;
    char               saved_lsk_label[WIN_MGR_LABEL_MAX];
    softkey_callback_t saved_lsk_cb;
    char               saved_rsk_label[WIN_MGR_LABEL_MAX];
    softkey_callback_t saved_rsk_cb;
} usb_select_ctx_t;

static usb_select_ctx_t *s_usb_select = NULL;
static lv_group_t        *s_usb_select_group = NULL;

static void apply_usb_mode_selection(uint8_t index)
{
    os_usb_mode_t mode = USB_MODE_CHARGE_ONLY;
    switch (index) {
    case 0: mode = USB_MODE_MASS_STORAGE; break;
    case 1: mode = USB_MODE_TETHERING; break;
    case 2: mode = USB_MODE_CHARGE_ONLY; break;
    case 3: mode = USB_MODE_SERIAL_DEBUG; break;
    default: break;
    }

    connectivity_usb_set_mode(mode);
    usb_select_close();
}

static void on_option_clicked(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    uint8_t idx = (uint8_t)(uintptr_t)lv_obj_get_user_data(target);
    apply_usb_mode_selection(idx);
}

static void on_lsk_action(void)
{
    if (!s_usb_select || !s_usb_select->group) {
        usb_select_close();
        return;
    }
    lv_obj_t *focused = lv_group_get_focused(s_usb_select->group);
    if (!focused) {
        usb_select_close();
        return;
    }
    uint8_t idx = (uint8_t)(uintptr_t)lv_obj_get_user_data(focused);
    apply_usb_mode_selection(idx);
}

static void on_rsk_action(void)
{
    usb_select_close();
}

bool usb_select_is_active(void)
{
    return (s_usb_select != NULL);
}

void usb_select_show(void)
{
    if (s_usb_select) return; /* Already open */

    s_usb_select = (usb_select_ctx_t *)calloc(1, sizeof(usb_select_ctx_t));
    if (!s_usb_select) return;

    /* Save active softkeys */
    win_mgr_entry_t *top = win_mgr_get_top();
    if (top) {
        strncpy(s_usb_select->saved_lsk_label, top->lsk_label, sizeof(s_usb_select->saved_lsk_label) - 1);
        s_usb_select->saved_lsk_cb = top->lsk_cb;
        strncpy(s_usb_select->saved_rsk_label, top->rsk_label, sizeof(s_usb_select->saved_rsk_label) - 1);
        s_usb_select->saved_rsk_cb = top->rsk_cb;
    }

    lv_group_t *global_grp = win_mgr_get_group();
    if (global_grp) {
        s_usb_select->saved_focus = lv_group_get_focused(global_grp);
    }

    if (!s_usb_select_group) {
        s_usb_select_group = lv_group_create();
    } else {
        lv_group_remove_all_objs(s_usb_select_group);
    }
    s_usb_select->group = s_usb_select_group;

    /* Modal Backdrop on lv_layer_top() spanning 176x220 */
    lv_obj_t *top_layer = lv_layer_top();
    lv_obj_t *backdrop = lv_obj_create(top_layer);
    s_usb_select->backdrop = backdrop;
    lv_obj_set_size(backdrop, 176, 220);
    lv_obj_align(backdrop, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(backdrop, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(backdrop, LV_OPA_70, 0);
    lv_obj_set_style_border_width(backdrop, 0, 0);
    lv_obj_set_style_pad_all(backdrop, 6, 0);
    lv_obj_set_flex_flow(backdrop, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(backdrop, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(backdrop, LV_OBJ_FLAG_SCROLLABLE);

    /* Modal Dialog Card */
    lv_obj_t *card = lv_obj_create(backdrop);
    lv_obj_set_size(card, 160, 190);
    lv_obj_set_style_bg_color(card, theme_get()->bg_color, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, theme_get()->accent, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 0, 0);
    lv_obj_set_style_outline_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 6, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* Title Bar */
    lv_obj_t *title_lbl = lv_label_create(card);
    lv_label_set_text(title_lbl, LV_SYMBOL_USB " USB Connection");
    lv_obj_set_style_text_color(title_lbl, theme_get()->accent, 0);
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_pad_bottom(title_lbl, 4, 0);

    const char *opt_titles[] = {
        "Mass Storage",
        "USB Tethering",
        "Charge Only",
        "Debug Console"
    };

    const char *opt_icons[] = {
        LV_SYMBOL_SD_CARD,
        LV_SYMBOL_WIFI,
        LV_SYMBOL_CHARGE,
        LV_SYMBOL_SETTINGS
    };

    for (uint8_t i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_button_create(card);
        s_usb_select->options[i] = btn;
        lv_obj_set_size(btn, 144, 28);
        lv_obj_set_style_bg_color(btn, theme_get()->card_color, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(btn, theme_is_light_mode() ? lv_color_hex(0xCCCCCC) : lv_color_hex(0x2D3340), 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_radius(btn, 0, 0);
        lv_obj_set_style_outline_width(btn, 0, 0);
        lv_obj_set_style_pad_hor(btn, 6, 0);
        lv_obj_set_style_pad_ver(btn, 2, 0);
        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_set_style_border_color(btn, theme_get()->accent, LV_STATE_FOCUSED);
        lv_obj_set_style_border_width(btn, 2, LV_STATE_FOCUSED);
        lv_obj_set_style_radius(btn, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_bg_color(btn, theme_get()->accent, LV_STATE_FOCUSED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_30, LV_STATE_FOCUSED);

        lv_obj_t *lbl = lv_label_create(btn);
        char buf[64];
        snprintf(buf, sizeof(buf), "%s %s", opt_icons[i], opt_titles[i]);
        lv_label_set_text(lbl, buf);
        lv_obj_set_style_text_color(lbl, theme_get()->text_primary, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_10, 0);

        lv_obj_set_user_data(btn, (void *)(uintptr_t)i);
        lv_obj_add_event_cb(btn, on_option_clicked, LV_EVENT_CLICKED, NULL);

        lv_group_add_obj(s_usb_select->group, btn);
    }

    /* Switch input focus to modal group */
    lv_indev_set_group(hal_input_get_lv_indev(), s_usb_select->group);
    lv_group_focus_obj(s_usb_select->options[0]);

    softkey_set_actions("Select", on_lsk_action, "Cancel", on_rsk_action);
    OS_LOGI(TAG, "USB Selection modal displayed on lv_layer_top");
}

void usb_select_close(void)
{
    if (!s_usb_select) return;

    lv_obj_t *bd = s_usb_select->backdrop;
    lv_obj_t *saved_focus = s_usb_select->saved_focus;

    char saved_lsk[WIN_MGR_LABEL_MAX];
    char saved_rsk[WIN_MGR_LABEL_MAX];
    softkey_callback_t saved_lsk_cb = s_usb_select->saved_lsk_cb;
    softkey_callback_t saved_rsk_cb = s_usb_select->saved_rsk_cb;
    strncpy(saved_lsk, s_usb_select->saved_lsk_label, sizeof(saved_lsk) - 1);
    saved_lsk[sizeof(saved_lsk) - 1] = '\0';
    strncpy(saved_rsk, s_usb_select->saved_rsk_label, sizeof(saved_rsk) - 1);
    saved_rsk[sizeof(saved_rsk) - 1] = '\0';

    free(s_usb_select);
    s_usb_select = NULL;

    if (bd && lv_obj_is_valid(bd)) {
        lv_obj_delete_async(bd);
    }

    if (s_usb_select_group) {
        lv_group_remove_all_objs(s_usb_select_group);
    }

    /* Restore global group & softkeys */
    lv_group_t *global_grp = win_mgr_get_group();
    if (global_grp) {
        lv_indev_set_group(hal_input_get_lv_indev(), global_grp);
        if (saved_focus && lv_obj_is_valid(saved_focus)) {
            lv_group_focus_obj(saved_focus);
        }
    }

    softkey_set_actions(saved_lsk, saved_lsk_cb, saved_rsk, saved_rsk_cb);
    OS_LOGI(TAG, "USB Selection modal closed");
}
