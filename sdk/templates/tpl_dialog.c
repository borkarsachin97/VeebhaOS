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

#include "veebha_templates.h"
#include "veebha_win_mgr.h"
#include "veebha_softkeys.h"
#include "veebha_theme.h"
#include "sdk/text/font_fallback.h"
#include "boards/board_config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    lv_obj_t          *overlay;
    lv_obj_t          *card;
    lv_obj_t          *saved_focus;
    char               saved_lsk_label[WIN_MGR_LABEL_MAX];
    softkey_callback_t saved_lsk_cb;
    char               saved_rsk_label[WIN_MGR_LABEL_MAX];
    softkey_callback_t saved_rsk_cb;
    void (*on_confirm)(void);
    void (*on_cancel)(void);
} tpl_dialog_state_t;

static tpl_dialog_state_t *s_dialog = NULL;

static void on_dialog_confirm(void)
{
    if (!s_dialog) return;
    void (*confirm_cb)(void) = s_dialog->on_confirm;
    tpl_dialog_close();
    if (confirm_cb) {
        confirm_cb();
    }
}

static void on_dialog_cancel(void)
{
    if (!s_dialog) return;
    void (*cancel_cb)(void) = s_dialog->on_cancel;
    tpl_dialog_close();
    if (cancel_cb) {
        cancel_cb();
    }
}

static void on_dialog_key_cb(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_ENTER) {
        on_dialog_confirm();
    } else if (key == LV_KEY_ESC) {
        on_dialog_cancel();
    }
}

bool tpl_dialog_is_active(void)
{
    return s_dialog != NULL;
}

lv_obj_t * tpl_dialog_show(const tpl_dialog_desc_t *desc)
{
    if (!desc) return NULL;

    /* If a dialog is already open, close it first */
    if (s_dialog) {
        tpl_dialog_close();
    }

    s_dialog = (tpl_dialog_state_t *)calloc(1, sizeof(tpl_dialog_state_t));
    if (!s_dialog) return NULL;

    s_dialog->on_confirm = desc->on_confirm;
    s_dialog->on_cancel = desc->on_cancel;

    /* Save current softkey state from the active window manager top entry */
    win_mgr_entry_t *top = win_mgr_get_top();
    if (top) {
        strncpy(s_dialog->saved_lsk_label, top->lsk_label, sizeof(s_dialog->saved_lsk_label) - 1);
        s_dialog->saved_lsk_cb = top->lsk_cb;
        strncpy(s_dialog->saved_rsk_label, top->rsk_label, sizeof(s_dialog->saved_rsk_label) - 1);
        s_dialog->saved_rsk_cb = top->rsk_cb;
    }

    /* Save current focus in keypad group */
    lv_group_t *group = win_mgr_get_group();
    if (group) {
        s_dialog->saved_focus = lv_group_get_focused(group);
    }

    /* 1. Modal Overlay on lv_layer_top() */
    lv_obj_t *layer_top = lv_layer_top();
    lv_obj_t *overlay = lv_obj_create(layer_top);
    s_dialog->overlay = overlay;

    lv_obj_set_size(overlay, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_70, 0);
    lv_obj_set_style_border_width(overlay, 0, 0);
    lv_obj_set_style_pad_all(overlay, 0, 0);
    lv_obj_set_style_radius(overlay, 0, 0);
    lv_obj_remove_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(overlay, LV_OBJ_FLAG_SCROLL_ANIMATION);

    /* Flex center alignment to position card right in the middle */
    lv_obj_set_flex_flow(overlay, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(overlay, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 2. Dialog Card Container: 150px wide, elastic height */
    lv_obj_t *card = lv_obj_create(overlay);
    s_dialog->card = card;

    lv_obj_set_width(card, 150);
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, theme_get()->card_color, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, theme_get()->accent, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 0, 0);
    lv_obj_set_style_outline_width(card, 0, 0);
    lv_obj_set_style_pad_hor(card, 8, 0);
    lv_obj_set_style_pad_ver(card, 8, 0);
    lv_obj_set_style_pad_row(card, 4, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLL_ANIMATION);

    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Optional Icon */
    if (desc->icon) {
        lv_obj_t *icon_lbl = lv_label_create(card);
        lv_label_set_text(icon_lbl, (const char *)desc->icon);
        lv_obj_set_style_text_color(icon_lbl, theme_get()->accent, 0);
        lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_16, 0);
    }

    /* Title Label */
    if (desc->title) {
        lv_obj_t *title_lbl = lv_label_create(card);
        lv_label_set_text(title_lbl, desc->title);
        lv_obj_set_style_text_color(title_lbl, theme_get()->text_primary, 0);
        lv_obj_set_style_text_font(title_lbl, veebha_font_get_default(), 0);
        lv_obj_set_style_text_align(title_lbl, LV_TEXT_ALIGN_CENTER, 0);
    }

    /* Message Label with auto-wrapping */
    if (desc->message) {
        lv_obj_t *msg_lbl = lv_label_create(card);
        lv_obj_set_width(msg_lbl, 134); /* 150 - 2 * 8 pad */
        lv_label_set_long_mode(msg_lbl, LV_LABEL_LONG_WRAP);
        lv_label_set_text(msg_lbl, desc->message);
        lv_obj_set_style_text_color(msg_lbl, theme_get()->text_muted, 0);
        lv_obj_set_style_text_font(msg_lbl, veebha_font_get_default(), 0);
        lv_obj_set_style_text_align(msg_lbl, LV_TEXT_ALIGN_CENTER, 0);
    }

    /* 3. Register Card into Keypad Group and hook key events */
    if (group) {
        lv_group_add_obj(group, card);
        lv_group_focus_obj(card);
        lv_obj_add_event_cb(card, on_dialog_key_cb, LV_EVENT_KEY, NULL);
    }

    /* 4. Update Softkey Bar with Dialog Actions */
    const char *lsk = desc->lsk_label ? desc->lsk_label : "OK";
    const char *rsk = desc->rsk_label ? desc->rsk_label : "Cancel";
    softkey_set_actions(lsk, on_dialog_confirm, rsk, on_dialog_cancel);

    printf("[DIALOG] Displayed modal dialog '%s'\n", desc->title ? desc->title : "Notice");
    return overlay;
}

void tpl_dialog_close(void)
{
    if (!s_dialog) return;

    /* Restore previous softkeys */
    softkey_set_actions(s_dialog->saved_lsk_label, s_dialog->saved_lsk_cb,
                        s_dialog->saved_rsk_label, s_dialog->saved_rsk_cb);

    /* Restore previous keypad focus before deleting overlay */
    lv_group_t *group = win_mgr_get_group();
    if (group) {
        if (s_dialog->saved_focus && lv_obj_is_valid(s_dialog->saved_focus)) {
            lv_group_focus_obj(s_dialog->saved_focus);
        } else {
            win_mgr_entry_t *top = win_mgr_get_top();
            if (top && top->screen && lv_obj_is_valid(top->screen)) {
                win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)lv_obj_get_user_data(top->screen);
                if (hdr && hdr->first_item && lv_obj_is_valid(hdr->first_item)) {
                    lv_group_focus_obj(hdr->first_item);
                } else {
                    lv_group_focus_next(group);
                }
            }
        }
    }

    /* Delete overlay and its children on lv_layer_top asynchronously */
    if (s_dialog->overlay && lv_obj_is_valid(s_dialog->overlay)) {
        lv_obj_delete_async(s_dialog->overlay);
        s_dialog->overlay = NULL;
    }

    free(s_dialog);
    s_dialog = NULL;

    printf("[DIALOG] Closed modal dialog\n");
}
