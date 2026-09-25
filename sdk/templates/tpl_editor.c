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
#include "veebha_status_bar.h"
#include "veebha_t9.h"
#include "veebha_theme.h"
#include "sdk/text/font_fallback.h"
#include "boards/board_config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    lv_obj_t          *softkey_bar;
    lv_obj_t          *first_item; /* Points to textarea */
    veebha_view_type_t view_type;
    char               title[WIN_MGR_LABEL_MAX];
    os_fullscreen_mode_t fullscreen_mode;
    bool               show_battery_hud;
    lv_obj_t          *textarea;
    lv_obj_t          *mode_lbl;
    char              *buffer;
    uint16_t           max_len;
    void (*on_save)(const char *text);
    void (*on_cancel)(void);
    char               lsk_label[WIN_MGR_LABEL_MAX];
    char               rsk_label[WIN_MGR_LABEL_MAX];
} tpl_editor_screen_data_t;

static tpl_editor_screen_data_t *s_active_editor = NULL;
static void on_editor_rsk_long_press(void);

static void update_editor_softkeys(tpl_editor_screen_data_t *data)
{
    if (!data || !data->textarea) return;

    const char *text = lv_textarea_get_text(data->textarea);
    size_t len = text ? strlen(text) : 0;

    const char *lsk = data->lsk_label[0] ? data->lsk_label : "Done";
    const char *rsk = (len > 0) ? "Clear" : (data->rsk_label[0] ? data->rsk_label : "Back");

    softkey_set_actions(lsk, tpl_editor_default_lsk, rsk, tpl_editor_default_rsk);
    softkey_set_rsk_long_action((len > 0) ? on_editor_rsk_long_press : NULL);
}

static void on_t9_char_update(char c, bool is_replace)
{
    if (!s_active_editor || !s_active_editor->textarea) return;

    lv_obj_t *ta = s_active_editor->textarea;
    if (is_replace) {
        lv_textarea_delete_char(ta);
    }
    lv_textarea_add_char(ta, c);
    update_editor_softkeys(s_active_editor);
}

static void on_t9_mode_change(t9_input_mode_t mode)
{
    if (!s_active_editor || !s_active_editor->mode_lbl) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "[%s]", t9_engine_get_mode_str(mode));
    lv_label_set_text(s_active_editor->mode_lbl, buf);
}

static void on_editor_delete_cb(lv_event_t *e)
{
    lv_obj_t *scr = lv_event_get_target(e);
    tpl_editor_screen_data_t *data = (tpl_editor_screen_data_t *)lv_obj_get_user_data(scr);

    if (data) {
        if (s_active_editor == data) {
            s_active_editor = NULL;
            t9_engine_reset();
            softkey_set_rsk_long_action(NULL);
        }
        free(data);
        lv_obj_set_user_data(scr, NULL);
    }
}

void tpl_editor_default_lsk(void)
{
    if (!s_active_editor || !s_active_editor->textarea) return;

    /* Commit any pending multi-tap character */
    t9_engine_commit();

    const char *text = lv_textarea_get_text(s_active_editor->textarea);
    if (s_active_editor->buffer && s_active_editor->max_len > 0) {
        strncpy(s_active_editor->buffer, text ? text : "", s_active_editor->max_len - 1);
        s_active_editor->buffer[s_active_editor->max_len - 1] = '\0';
    }

    if (s_active_editor->on_save) {
        s_active_editor->on_save(text ? text : "");
    }
}

void tpl_editor_default_rsk(void)
{
    if (!s_active_editor || !s_active_editor->textarea) {
        win_mgr_pop();
        return;
    }

    /* If a character is currently cycling in preview, cancel it */
    if (t9_engine_has_pending()) {
        t9_engine_cancel_pending();
        lv_textarea_delete_char(s_active_editor->textarea);
        update_editor_softkeys(s_active_editor);
        return;
    }

    const char *text = lv_textarea_get_text(s_active_editor->textarea);
    size_t len = text ? strlen(text) : 0;

    if (len > 0) {
        /* Backspace: delete character before cursor */
        lv_textarea_delete_char(s_active_editor->textarea);
        const char *new_text = lv_textarea_get_text(s_active_editor->textarea);
        t9_engine_notify_char_deleted(new_text == NULL || new_text[0] == '\0');
        update_editor_softkeys(s_active_editor);
    } else {
        /* Textarea is empty -> cancel/exit */
        if (s_active_editor->on_cancel) {
            s_active_editor->on_cancel();
        } else {
            win_mgr_pop();
        }
    }
}

static void on_editor_rsk_long_press(void)
{
    if (!s_active_editor || !s_active_editor->textarea) return;

    t9_engine_reset();
    lv_textarea_set_text(s_active_editor->textarea, "");
    t9_engine_notify_char_deleted(true);
    update_editor_softkeys(s_active_editor);
    printf("[EDITOR] Cleared entire text buffer via RSK long-press\n");
}

lv_obj_t * tpl_editor_create(const tpl_editor_desc_t *desc)
{
    if (!desc) return NULL;

    /* 1. Root Screen Container */
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, CONFIG_DISP_HOR_RES, CONFIG_DISP_VER_RES);
    lv_obj_set_style_bg_color(screen, theme_get()->bg_color, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Allocate screen context */
    tpl_editor_screen_data_t *data = (tpl_editor_screen_data_t *)calloc(1, sizeof(tpl_editor_screen_data_t));
    if (!data) return NULL;

    data->view_type = VEEBHA_VIEW_TYPE_EDITOR;
    strncpy(data->title, desc->title ? desc->title : "Editor", sizeof(data->title) - 1);
    data->title[sizeof(data->title) - 1] = '\0';
    data->buffer = desc->buffer;
    data->max_len = desc->max_len;
    data->on_save = desc->on_save;
    data->on_cancel = desc->on_cancel;

    if (desc->lsk_label) {
        strncpy(data->lsk_label, desc->lsk_label, sizeof(data->lsk_label) - 1);
    } else {
        strcpy(data->lsk_label, "Done");
    }

    if (desc->rsk_label) {
        strncpy(data->rsk_label, desc->rsk_label, sizeof(data->rsk_label) - 1);
    } else {
        strcpy(data->rsk_label, "Clear");
    }

    lv_obj_set_user_data(screen, data);
    lv_obj_add_event_cb(screen, on_editor_delete_cb, LV_EVENT_DELETE, NULL);

    /* 2. Zone A: Fixed 18px Top Status Bar via unified status_bar_create */
    status_bar_create(screen, desc->title ? desc->title : "Compose");

    /* 3. Zone B: Elastic 184px Content Viewport with Header and lv_textarea */
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_size(content, lv_pct(100), 0);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_color(content, theme_get()->bg_color, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_radius(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);

    /* Editor Sub-Header: Mode Badge */
    lv_obj_t *subhdr = lv_obj_create(content);
    lv_obj_set_size(subhdr, lv_pct(100), 16);
    lv_obj_set_style_bg_color(subhdr, theme_is_light_mode() ? lv_color_hex(0xE2E8F0) : lv_color_hex(0x181A20), 0);
    lv_obj_set_style_bg_opa(subhdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(subhdr, 0, 0);
    lv_obj_set_style_pad_hor(subhdr, 6, 0);
    lv_obj_set_style_pad_ver(subhdr, 0, 0);
    lv_obj_remove_flag(subhdr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(subhdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(subhdr, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *mode_lbl = lv_label_create(subhdr);
    data->mode_lbl = mode_lbl;
    lv_label_set_text(mode_lbl, "[Abc]");
    lv_obj_set_style_text_color(mode_lbl, lv_color_hex(0xFFD54F), 0);
    lv_obj_set_style_text_font(mode_lbl, veebha_font_get_default(), 0);

    /* Textarea fills remainder of content viewport */
    lv_obj_t *ta = lv_textarea_create(content);
    data->textarea = ta;
    data->first_item = ta;

    lv_obj_set_size(ta, lv_pct(100), 0);
    lv_obj_set_flex_grow(ta, 1);
    lv_obj_set_style_bg_color(ta, theme_get()->card_color, 0);
    lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ta, theme_is_light_mode() ? lv_color_hex(0xCCCCCC) : lv_color_hex(0x282C35), 0);
    lv_obj_set_style_border_width(ta, 1, 0);
    lv_obj_set_style_radius(ta, 0, 0);
    lv_obj_set_style_pad_all(ta, 6, 0);
    lv_obj_set_style_text_color(ta, theme_get()->text_primary, 0);
    lv_obj_set_style_text_font(ta, veebha_font_get_default(), 0);

    /* Focused border accent */
    lv_obj_set_style_border_color(ta, theme_get()->accent, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(ta, 1, LV_STATE_FOCUSED);
    lv_obj_set_style_radius(ta, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(ta, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(ta, 0, LV_STATE_FOCUSED);

    lv_obj_set_style_border_color(ta, theme_get()->accent, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(ta, 1, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_radius(ta, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(ta, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(ta, 0, LV_STATE_FOCUS_KEY);
    lv_obj_remove_flag(ta, LV_OBJ_FLAG_SCROLL_ANIMATION);

    /* Cursor */
    lv_textarea_set_cursor_click_pos(ta, true);
    if (desc->max_len > 0) {
        lv_textarea_set_max_length(ta, desc->max_len);
    }

    if (desc->buffer && desc->buffer[0] != '\0') {
        lv_textarea_set_text(ta, desc->buffer);
    } else {
        lv_textarea_set_text(ta, "");
    }

    /* Add to Keypad Group */
    lv_group_t *group = win_mgr_get_group();
    if (group) {
        lv_group_add_obj(group, ta);
    }

    /* 4. Zone C: Fixed 20px Bottom Softkey Bar */
    const char *init_rsk = (desc->buffer && desc->buffer[0] != '\0') ? "Clear" : "Back";
    lv_obj_t *bar = softkey_bar_create(screen, data->lsk_label, init_rsk);
    data->softkey_bar = bar;

    s_active_editor = data;

    /* Initialize T9 engine and bind callbacks */
    t9_engine_init(on_t9_char_update, on_t9_mode_change);

    /* Set RSK long-press hook for Clear All */
    softkey_set_rsk_long_action(on_editor_rsk_long_press);

    return screen;
}
