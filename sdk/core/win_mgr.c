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

#include "veebha_win_mgr.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    lv_obj_t          *softkey_bar;
    lv_obj_t          *first_item;
    veebha_view_type_t view_type;
} win_mgr_screen_hdr_t;

static win_mgr_entry_t s_stack[WIN_MGR_MAX_DEPTH];
static uint8_t s_depth = 0;
static lv_group_t *s_group = NULL;

void win_mgr_init(lv_group_t *group)
{
    s_group = group;
    s_depth = 0;
    memset(s_stack, 0, sizeof(s_stack));
}

uint8_t win_mgr_get_depth(void)
{
    return s_depth;
}

win_mgr_entry_t * win_mgr_get_top(void)
{
    if (s_depth == 0) return NULL;
    return &s_stack[s_depth - 1];
}

lv_group_t * win_mgr_get_group(void)
{
    return s_group;
}

veebha_view_type_t win_mgr_get_active_view_type(void)
{
    if (s_depth == 0) return VEEBHA_VIEW_TYPE_GENERIC;
    return s_stack[s_depth - 1].view_type;
}

void win_mgr_set_active_view_type(veebha_view_type_t type)
{
    if (s_depth > 0) {
        s_stack[s_depth - 1].view_type = type;
    }
}

bool win_mgr_push(lv_obj_t *screen,
                  const char *lsk, softkey_callback_t lsk_cb,
                  const char *rsk, softkey_callback_t rsk_cb)
{
    if (!screen) {
        fprintf(stderr, "[WIN_MGR] Cannot push NULL screen\n");
        return false;
    }

    if (s_depth >= WIN_MGR_MAX_DEPTH) {
        fprintf(stderr, "[WIN_MGR] Stack overflow! Max depth is %d\n", WIN_MGR_MAX_DEPTH);
        return false;
    }

    /* Save current screen focus if a screen is already active */
    if (s_depth > 0) {
        if (s_group) {
            s_stack[s_depth - 1].focused_obj = lv_group_get_focused(s_group);
        }
        /* Hide previous screen so its objects are ignored by group focus */
        lv_obj_add_flag(s_stack[s_depth - 1].screen, LV_OBJ_FLAG_HIDDEN);
    }

    /* Configure new stack entry */
    win_mgr_entry_t *entry = &s_stack[s_depth];
    entry->screen = screen;
    entry->focused_obj = NULL;
    entry->lsk_cb = lsk_cb;
    entry->rsk_cb = rsk_cb;

    if (lsk) {
        strncpy(entry->lsk_label, lsk, sizeof(entry->lsk_label) - 1);
        entry->lsk_label[sizeof(entry->lsk_label) - 1] = '\0';
    } else {
        entry->lsk_label[0] = '\0';
    }

    if (rsk) {
        strncpy(entry->rsk_label, rsk, sizeof(entry->rsk_label) - 1);
        entry->rsk_label[sizeof(entry->rsk_label) - 1] = '\0';
    } else {
        entry->rsk_label[0] = '\0';
    }

    s_depth++;

    /* Ensure screen is visible and load it */
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_HIDDEN);
    lv_screen_load(screen);

    /* Bind the softkey bar of the loaded screen if stored in user_data */
    win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)lv_obj_get_user_data(screen);
    entry->view_type = hdr ? hdr->view_type : VEEBHA_VIEW_TYPE_GENERIC;

    lv_obj_t *bar = hdr ? hdr->softkey_bar : NULL;
    softkey_bar_set_active_widget(bar);
    softkey_set_actions(entry->lsk_label, entry->lsk_cb, entry->rsk_label, entry->rsk_cb);

    /* Focus first item in the newly loaded screen */
    if (s_group) {
        if (hdr && hdr->first_item) {
            lv_group_focus_obj(hdr->first_item);
        } else if (lv_group_get_focused(s_group) == NULL) {
            lv_group_focus_next(s_group);
        }
    }

    printf("[WIN_MGR] Pushed screen %p (New Depth: %u)\n", (void*)screen, s_depth);
    return true;
}

bool win_mgr_pop(void)
{
    if (s_depth <= 1) {
        printf("[WIN_MGR] Cannot pop root/home screen (Depth: %u)\n", s_depth);
        return false;
    }

    /* Screen to be popped */
    win_mgr_entry_t *popped_entry = &s_stack[s_depth - 1];
    lv_obj_t *scr_to_delete = popped_entry->screen;

    s_depth--;
    win_mgr_entry_t *prev_entry = &s_stack[s_depth - 1];

    /* Unhide and load previous screen */
    lv_obj_clear_flag(prev_entry->screen, LV_OBJ_FLAG_HIDDEN);
    lv_screen_load(prev_entry->screen);

    /* Rebind previous screen's softkey bar */
    win_mgr_screen_hdr_t *prev_hdr = (win_mgr_screen_hdr_t *)lv_obj_get_user_data(prev_entry->screen);
    lv_obj_t *prev_bar = prev_hdr ? prev_hdr->softkey_bar : NULL;
    softkey_bar_set_active_widget(prev_bar);
    softkey_set_actions(prev_entry->lsk_label, prev_entry->lsk_cb,
                        prev_entry->rsk_label, prev_entry->rsk_cb);

    /* Cleanly destroy the popped screen and all its children */
    if (scr_to_delete) {
        lv_obj_delete(scr_to_delete);
    }

    /* Restore previous focus */
    if (s_group && prev_entry->focused_obj) {
        lv_group_focus_obj(prev_entry->focused_obj);
    }

    printf("[WIN_MGR] Popped screen (Remaining Depth: %u)\n", s_depth);
    return true;
}

void win_mgr_reset_to_home(void)
{
    while (s_depth > 1) {
        win_mgr_pop();
    }
}
