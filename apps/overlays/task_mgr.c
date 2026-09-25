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
#include "drivers/hal_input.h"
#include "sdk/include/veebha_i18n.h"
#include "sdk/text/font_fallback.h"
#include "boards/board_config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TASK_MGR_MAX_TASKS WIN_MGR_MAX_DEPTH

typedef struct {
    lv_obj_t          *overlay;
    lv_group_t        *group;
    lv_obj_t          *carousel;
    lv_obj_t          *cards[TASK_MGR_MAX_TASKS];
    uint8_t            task_indices[TASK_MGR_MAX_TASKS];
    uint8_t            card_count;
    lv_obj_t          *saved_focus;
    char               saved_lsk_label[WIN_MGR_LABEL_MAX];
    softkey_callback_t saved_lsk_cb;
    char               saved_rsk_label[WIN_MGR_LABEL_MAX];
    softkey_callback_t saved_rsk_cb;
} task_mgr_ctx_t;

static task_mgr_ctx_t *s_task_mgr = NULL;
static lv_group_t      *s_task_mgr_group = NULL;

#define TASK_MGR_HOME_ID 0xFF

static void on_task_mgr_switch(void)
{
    if (!s_task_mgr) return;

    lv_group_t *g = s_task_mgr->group;
    if (!g) {
        task_mgr_close();
        return;
    }

    lv_obj_t *focused = lv_group_get_focused(g);
    if (!focused) {
        task_mgr_close();
        return;
    }

    uint8_t target_idx = (uint8_t)(uintptr_t)lv_obj_get_user_data(focused);

    /* Close task manager first without restoring original focus, so switch_to can set its own focus */
    lv_obj_t *overlay_to_del = s_task_mgr->overlay;
    lv_indev_set_group(hal_input_get_lv_indev(), win_mgr_get_group());
    if (s_task_mgr_group) {
        lv_group_remove_all_objs(s_task_mgr_group);
    }
    free(s_task_mgr);
    s_task_mgr = NULL;

    if (overlay_to_del && lv_obj_is_valid(overlay_to_del)) {
        lv_obj_delete_async(overlay_to_del);
    }

    if (target_idx == TASK_MGR_HOME_ID) {
        win_mgr_show_home();
        printf("[TASK_MGR] Switched to Home Screen\n");
    } else {
        win_mgr_task_activate(target_idx);
        printf("[TASK_MGR] Switched to task %u\n", target_idx);
    }
}

static void on_task_mgr_close_app(void)
{
    if (!s_task_mgr) return;

    lv_group_t *g = s_task_mgr->group;
    if (!g) {
        task_mgr_close();
        return;
    }

    lv_obj_t *focused = lv_group_get_focused(g);
    if (!focused) {
        task_mgr_close();
        return;
    }

    uint8_t target_idx = (uint8_t)(uintptr_t)lv_obj_get_user_data(focused);

    if (target_idx == TASK_MGR_HOME_ID) {
        printf("[TASK_MGR] Cannot close Root/Home screen\n");
        return;
    }

    /* Close task manager overlay first without attempting to refocus the dead app */
    lv_obj_t *overlay_to_del = s_task_mgr->overlay;
    lv_indev_set_group(hal_input_get_lv_indev(), win_mgr_get_group());
    if (s_task_mgr_group) {
        lv_group_remove_all_objs(s_task_mgr_group);
    }
    free(s_task_mgr);
    s_task_mgr = NULL;

    if (overlay_to_del && lv_obj_is_valid(overlay_to_del)) {
        lv_obj_delete_async(overlay_to_del);
    }

    /* Close the app: win_mgr_task_kill restores Home if it was active */
    win_mgr_task_kill(target_idx);
}

bool task_mgr_is_active(void)
{
    return s_task_mgr != NULL;
}

void task_mgr_show(void)
{
    if (s_task_mgr) return; /* Already open */

    s_task_mgr = (task_mgr_ctx_t *)calloc(1, sizeof(task_mgr_ctx_t));
    if (!s_task_mgr) return;

    /* Save softkeys */
    win_mgr_entry_t *top = win_mgr_get_top();
    if (top) {
        strncpy(s_task_mgr->saved_lsk_label, top->lsk_label, sizeof(s_task_mgr->saved_lsk_label) - 1);
        s_task_mgr->saved_lsk_cb = top->lsk_cb;
        strncpy(s_task_mgr->saved_rsk_label, top->rsk_label, sizeof(s_task_mgr->saved_rsk_label) - 1);
        s_task_mgr->saved_rsk_cb = top->rsk_cb;
    }

    /* Save focus */
    lv_group_t *group = win_mgr_get_group();
    if (group) {
        s_task_mgr->saved_focus = lv_group_get_focused(group);
    }

    /* Create or reuse dedicated focus group for the switcher */
    if (!s_task_mgr_group) {
        s_task_mgr_group = lv_group_create();
    } else {
        lv_group_remove_all_objs(s_task_mgr_group);
    }
    s_task_mgr->group = s_task_mgr_group;

    /* 1. Modal container covering middle 184px area on lv_layer_top() */
    lv_obj_t *layer_top = lv_layer_top();
    lv_obj_t *overlay = lv_obj_create(layer_top);
    s_task_mgr->overlay = overlay;

    lv_obj_set_size(overlay, 176, 184);
    lv_obj_align(overlay, LV_ALIGN_TOP_MID, 0, 18);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x14171E), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_90, 0);
    lv_obj_set_style_border_color(overlay, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_border_width(overlay, 1, 0);
    lv_obj_set_style_radius(overlay, 0, 0);
    lv_obj_set_style_pad_all(overlay, 4, 0);
    lv_obj_remove_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_flex_flow(overlay, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(overlay, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Header Title: Running Tasks (N) */
    uint8_t total_tasks = win_mgr_get_task_count();
    char hdr_buf[64];
    snprintf(hdr_buf, sizeof(hdr_buf), "%s (%u)", veebha_i18n_str(STR_TASK_SWITCHER), (unsigned int)(1 + total_tasks));

    lv_obj_t *hdr = lv_label_create(overlay);
    lv_label_set_text(hdr, hdr_buf);
    lv_obj_set_style_text_color(hdr, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(hdr, veebha_font_get_default(), 0);
    lv_obj_set_style_pad_bottom(hdr, 4, 0);

    /* 2. Vertical Task List Container */
    lv_obj_t *list_box = lv_obj_create(overlay);
    lv_obj_set_size(list_box, lv_pct(100), 0);
    lv_obj_set_flex_grow(list_box, 1);
    lv_obj_set_style_bg_opa(list_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list_box, 0, 0);
    lv_obj_set_style_pad_all(list_box, 0, 0);
    lv_obj_set_style_pad_row(list_box, 2, 0);
    lv_obj_set_flex_flow(list_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list_box, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(list_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(list_box, LV_SCROLLBAR_MODE_OFF);

    s_task_mgr->card_count = 0;

    bool home_is_active = (win_mgr_get_active_task() == NULL);

    /* Row 0: Always Home Screen */
    lv_obj_t *home_row = lv_button_create(list_box);
    s_task_mgr->cards[s_task_mgr->card_count] = home_row;
    s_task_mgr->task_indices[s_task_mgr->card_count] = TASK_MGR_HOME_ID;

    lv_obj_set_size(home_row, lv_pct(100), 28);
    lv_obj_set_flex_flow(home_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(home_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(home_row, 6, 0);
    lv_obj_set_style_pad_ver(home_row, 0, 0);
    lv_obj_set_style_radius(home_row, 0, 0);
    lv_obj_set_style_outline_width(home_row, 0, 0);
    lv_obj_set_style_bg_color(home_row, lv_color_hex(0x1C2028), 0);
    lv_obj_set_style_bg_opa(home_row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(home_row, 0, 0);
    lv_obj_set_style_bg_color(home_row, lv_color_hex(0x1A3555), LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(home_row, lv_color_hex(0x00E5FF), LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(home_row, 1, LV_STATE_FOCUSED);
    lv_obj_set_style_radius(home_row, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(home_row, 0, LV_STATE_FOCUSED);
    lv_obj_remove_flag(home_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(home_row, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    lv_obj_t *h_icon = lv_label_create(home_row);
    lv_label_set_text(h_icon, LV_SYMBOL_HOME);
    lv_obj_set_style_text_color(h_icon, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(h_icon, veebha_font_get_default(), 0);
    lv_obj_set_style_pad_right(h_icon, 4, 0);

    lv_obj_t *h_title = lv_label_create(home_row);
    lv_label_set_text(h_title, "Idle");
    lv_obj_set_style_text_color(h_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(h_title, veebha_font_get_default(), 0);
    lv_obj_set_flex_grow(h_title, 1);

    lv_obj_t *h_state = lv_label_create(home_row);
    lv_label_set_text(h_state, home_is_active ? "[Active]" : "[Idle]");
    lv_obj_set_style_text_color(h_state, home_is_active ? lv_color_hex(0x00E5FF) : lv_color_hex(0x888E9B), 0);
    lv_obj_set_style_text_font(h_state, veebha_font_get_default(), 0);

    lv_obj_set_user_data(home_row, (void*)(uintptr_t)TASK_MGR_HOME_ID);
    if (s_task_mgr->group) {
        lv_group_add_obj(s_task_mgr->group, home_row);
    }
    s_task_mgr->card_count++;

    lv_obj_t *active_card = home_is_active ? home_row : NULL;

    /* Rows 1..N: Dynamically populated running background tasks */
    for (uint8_t i = 0; i < total_tasks && s_task_mgr->card_count < TASK_MGR_MAX_TASKS; i++) {
        os_task_entry_t *entry = win_mgr_get_task(i);
        if (!entry) continue;

        const char *tname = win_mgr_get_task_title(i);

        lv_obj_t *row = lv_button_create(list_box);
        s_task_mgr->cards[s_task_mgr->card_count] = row;
        s_task_mgr->task_indices[s_task_mgr->card_count] = i;

        lv_obj_set_size(row, lv_pct(100), 28);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_hor(row, 6, 0);
        lv_obj_set_style_pad_ver(row, 0, 0);
        lv_obj_set_style_radius(row, 0, 0);
        lv_obj_set_style_outline_width(row, 0, 0);

        /* Default styling */
        lv_obj_set_style_bg_color(row, lv_color_hex(0x1C2028), 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(row, 0, 0);

        /* Focused styling */
        lv_obj_set_style_bg_color(row, lv_color_hex(0x1A3555), LV_STATE_FOCUSED);
        lv_obj_set_style_border_color(row, lv_color_hex(0x00E5FF), LV_STATE_FOCUSED);
        lv_obj_set_style_border_width(row, 1, LV_STATE_FOCUSED);
        lv_obj_set_style_radius(row, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(row, 0, LV_STATE_FOCUSED);

        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(row, LV_OBJ_FLAG_SCROLL_ON_FOCUS);




        /* Title */
        lv_obj_t *title_lbl = lv_label_create(row);
        lv_label_set_text(title_lbl, tname);
        lv_obj_set_style_text_color(title_lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(title_lbl, veebha_font_get_default(), 0);
        lv_obj_set_flex_grow(title_lbl, 1);

        /* State: [Active] for top app, [Paused] for background */
        lv_obj_t *state_lbl = lv_label_create(row);
        bool is_active = !entry->is_backgrounded;
        lv_label_set_text(state_lbl, is_active ? "[Active]" : "[Paused]");
        lv_obj_set_style_text_color(state_lbl, is_active ? lv_color_hex(0x00E5FF) : lv_color_hex(0x888E9B), 0);
        lv_obj_set_style_text_font(state_lbl, veebha_font_get_default(), 0);

        lv_obj_set_user_data(row, (void*)(uintptr_t)i);

        if (s_task_mgr->group) {
            lv_group_add_obj(s_task_mgr->group, row);
        }

        if (is_active) {
            active_card = row;
        }

        s_task_mgr->card_count++;
    }

    /* Redirect keypad indev to isolated group */
    if (s_task_mgr->group) {
        lv_group_set_wrap(s_task_mgr->group, true);
        lv_group_set_editing(s_task_mgr->group, false);
        lv_indev_set_group(hal_input_get_lv_indev(), s_task_mgr->group);
        if (active_card) {
            lv_group_focus_obj(active_card);
        } else if (s_task_mgr->card_count > 0) {
            lv_group_focus_obj(s_task_mgr->cards[0]);
        }
    }

    softkey_set_actions(veebha_i18n_str(STR_SELECT), on_task_mgr_switch, veebha_i18n_str(STR_DELETE), on_task_mgr_close_app);

    printf("[TASK_MGR] Multitasking Switcher opened with %u tasks\n", s_task_mgr->card_count);
}

void task_mgr_close(void)
{
    if (!s_task_mgr) return;

    /* Restore softkeys */
    softkey_set_actions(s_task_mgr->saved_lsk_label, s_task_mgr->saved_lsk_cb,
                        s_task_mgr->saved_rsk_label, s_task_mgr->saved_rsk_cb);

    /* Restore indev to window manager group */
    lv_indev_set_group(hal_input_get_lv_indev(), win_mgr_get_group());

    /* Restore focus */
    lv_group_t *group = win_mgr_get_group();
    if (group && s_task_mgr->saved_focus && lv_obj_is_valid(s_task_mgr->saved_focus)) {
        lv_group_focus_obj(s_task_mgr->saved_focus);
    }

    /* Delete overlay from lv_layer_top() asynchronously */
    if (s_task_mgr->overlay && lv_obj_is_valid(s_task_mgr->overlay)) {
        lv_obj_delete_async(s_task_mgr->overlay);
        s_task_mgr->overlay = NULL;
    }

    if (s_task_mgr_group) {
        lv_group_remove_all_objs(s_task_mgr_group);
    }
    s_task_mgr->group = NULL;

    free(s_task_mgr);
    s_task_mgr = NULL;

    printf("[TASK_MGR] Multitasking Switcher closed\n");
}

void task_mgr_toggle(void)
{
    if (task_mgr_is_active()) {
        task_mgr_close();
    } else {
        task_mgr_show();
    }
}
