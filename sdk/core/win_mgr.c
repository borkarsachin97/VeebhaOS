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
#include "veebha_templates.h"
#include "veebha_overlays.h"
#include "veebha_status_bar.h"
#include "veebha_softkeys.h"
#include <stdio.h>
#include <string.h>

static int os_strcasecmp(const char *s1, const char *s2)
{
    if (!s1 || !s2) return (s1 == s2) ? 0 : -1;
    while (*s1 && *s2) {
        char c1 = *s1;
        char c2 = *s2;
        if (c1 >= 'A' && c1 <= 'Z') c1 += ('a' - 'A');
        if (c2 >= 'A' && c2 <= 'Z') c2 += ('a' - 'A');
        if (c1 != c2) return (int)((unsigned char)c1 - (unsigned char)c2);
        s1++;
        s2++;
    }
    return (int)((unsigned char)*s1 - (unsigned char)*s2);
}


typedef struct {
    uint8_t            id;
    char               name[24];
    lv_obj_t          *screen_obj;
    bool               is_backgrounded;
    win_mgr_entry_t    stack[WIN_MGR_MAX_DEPTH];
    uint8_t            stack_depth;
} os_task_internal_t;

static os_task_internal_t s_tasks[OS_MAX_TASKS];
static uint8_t            s_task_count = 0;
static int8_t             s_active_task_idx = -1; /* -1 = Home is in foreground */
static uint8_t            s_next_task_id = 1;

static win_mgr_entry_t    s_home_entry;
static lv_obj_t          *s_home_screen = NULL;
static lv_obj_t          *s_launcher_screen = NULL;
static lv_group_t        *s_group = NULL;

void win_mgr_set_launcher_screen(lv_obj_t *screen)
{
    s_launcher_screen = screen;
}

lv_obj_t * win_mgr_get_launcher_screen(void)
{
    return s_launcher_screen;
}

static void win_mgr_apply_fullscreen_mode(lv_obj_t *screen, os_fullscreen_mode_t mode, bool show_battery_hud)
{
    if (!screen || !lv_obj_is_valid(screen)) {
        status_bar_set_battery_hud_visible(false);
        return;
    }

    win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)lv_obj_get_user_data(screen);
    lv_obj_t *status_bar = NULL;
    lv_obj_t *softkey_bar = (hdr && hdr->softkey_bar) ? hdr->softkey_bar : NULL;

    uint32_t cnt = lv_obj_get_child_count(screen);
    for (uint32_t i = 0; i < cnt; i++) {
        lv_obj_t *ch = lv_obj_get_child(screen, i);
        if (!status_bar && status_bar_is_status_bar(ch)) {
            status_bar = ch;
        }
        if (!softkey_bar && softkey_bar_is_softkey_bar(ch)) {
            softkey_bar = ch;
        }
    }

    switch (mode) {
    case OS_FULLSCREEN_NONE:
        if (status_bar && lv_obj_is_valid(status_bar)) {
            lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_HIDDEN);
        }
        if (softkey_bar && lv_obj_is_valid(softkey_bar)) {
            lv_obj_clear_flag(softkey_bar, LV_OBJ_FLAG_HIDDEN);
        }
        status_bar_set_battery_hud_visible(false);
        break;

    case OS_FULLSCREEN_PARTIAL:
        if (status_bar && lv_obj_is_valid(status_bar)) {
            lv_obj_add_flag(status_bar, LV_OBJ_FLAG_HIDDEN);
        }
        if (softkey_bar && lv_obj_is_valid(softkey_bar)) {
            lv_obj_clear_flag(softkey_bar, LV_OBJ_FLAG_HIDDEN);
        }
        status_bar_set_battery_hud_visible(show_battery_hud);
        break;

    case OS_FULLSCREEN_FULL:
        if (status_bar && lv_obj_is_valid(status_bar)) {
            lv_obj_add_flag(status_bar, LV_OBJ_FLAG_HIDDEN);
        }
        if (softkey_bar && lv_obj_is_valid(softkey_bar)) {
            lv_obj_add_flag(softkey_bar, LV_OBJ_FLAG_HIDDEN);
        }
        status_bar_set_battery_hud_visible(show_battery_hud);
        break;
    }
}

void win_mgr_set_fullscreen_mode(lv_obj_t *screen, os_fullscreen_mode_t mode, bool show_battery_hud)
{
    if (!screen) return;
    win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)lv_obj_get_user_data(screen);
    if (hdr) {
        hdr->fullscreen_mode = mode;
        hdr->show_battery_hud = show_battery_hud;
    }

    win_mgr_entry_t *top = win_mgr_get_top();
    if (top && top->screen == screen) {
        top->fullscreen_mode = mode;
        top->show_battery_hud = show_battery_hud;
        win_mgr_apply_fullscreen_mode(screen, mode, show_battery_hud);
    }
}

os_fullscreen_mode_t win_mgr_get_fullscreen_mode(void)
{
    win_mgr_entry_t *top = win_mgr_get_top();
    return top ? top->fullscreen_mode : OS_FULLSCREEN_NONE;
}

bool win_mgr_is_battery_hud_enabled(void)
{
    win_mgr_entry_t *top = win_mgr_get_top();
    return top ? top->show_battery_hud : false;
}

void win_mgr_init(lv_group_t *group)
{
    s_group = group;
    s_task_count = 0;
    s_active_task_idx = -1;
    s_home_screen = NULL;
    s_launcher_screen = NULL;
    s_next_task_id = 1;
    memset(&s_home_entry, 0, sizeof(s_home_entry));
    memset(s_tasks, 0, sizeof(s_tasks));
    status_bar_set_battery_hud_visible(false);
}

uint8_t win_mgr_get_depth(void)
{
    if (s_active_task_idx >= 0 && s_active_task_idx < s_task_count) {
        return 1 + s_tasks[s_active_task_idx].stack_depth;
    }
    if (s_home_screen) {
        return 1;
    }
    return 0;
}

win_mgr_entry_t * win_mgr_get_top(void)
{
    if (s_active_task_idx >= 0 && s_active_task_idx < s_task_count) {
        os_task_internal_t *t = &s_tasks[s_active_task_idx];
        if (t->stack_depth > 0) {
            return &t->stack[t->stack_depth - 1];
        }
    }
    return &s_home_entry;
}

lv_group_t * win_mgr_get_group(void)
{
    return s_group;
}

veebha_view_type_t win_mgr_get_active_view_type(void)
{
    if (s_active_task_idx >= 0 && s_active_task_idx < s_task_count) {
        os_task_internal_t *t = &s_tasks[s_active_task_idx];
        if (t->stack_depth > 0) {
            return t->stack[t->stack_depth - 1].view_type;
        }
    }
    return s_home_entry.view_type;
}

void win_mgr_set_active_view_type(veebha_view_type_t type)
{
    if (s_active_task_idx >= 0 && s_active_task_idx < s_task_count) {
        os_task_internal_t *t = &s_tasks[s_active_task_idx];
        if (t->stack_depth > 0) {
            t->stack[t->stack_depth - 1].view_type = type;
            return;
        }
    }
    s_home_entry.view_type = type;
}

bool win_mgr_push(lv_obj_t *screen,
                  const char *lsk, softkey_callback_t lsk_cb,
                  const char *rsk, softkey_callback_t rsk_cb)
{
    if (!screen) {
        fprintf(stderr, "[WIN_MGR] Cannot push NULL screen\n");
        return false;
    }

    win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)lv_obj_get_user_data(screen);

    /* 1. First push initializes Home / Idle Screen */
    if (s_home_screen == NULL) {
        s_home_screen = screen;
        s_home_entry.screen = screen;
        s_home_entry.view_type = hdr ? hdr->view_type : VEEBHA_VIEW_TYPE_IDLE;
        const char *tname = (hdr && hdr->title[0]) ? hdr->title : "Idle";
        strncpy(s_home_entry.title, tname, sizeof(s_home_entry.title) - 1);
        strncpy(s_home_entry.lsk_label, lsk ? lsk : "Menu", sizeof(s_home_entry.lsk_label) - 1);
        s_home_entry.lsk_cb = lsk_cb;
        strncpy(s_home_entry.rsk_label, rsk ? rsk : "Contacts", sizeof(s_home_entry.rsk_label) - 1);
        s_home_entry.rsk_cb = rsk_cb;
        s_home_entry.fullscreen_mode = OS_FULLSCREEN_NONE;
        s_home_entry.show_battery_hud = false;
        s_active_task_idx = -1;

        lv_obj_clear_flag(screen, LV_OBJ_FLAG_HIDDEN);
        lv_screen_load(screen);
        softkey_bar_set_active_widget(hdr ? hdr->softkey_bar : NULL);
        softkey_set_actions(s_home_entry.lsk_label, lsk_cb, s_home_entry.rsk_label, rsk_cb);
        win_mgr_apply_fullscreen_mode(screen, OS_FULLSCREEN_NONE, false);

        if (s_group && hdr && hdr->first_item) {
            lv_group_focus_obj(hdr->first_item);
        }
        printf("[WIN_MGR] Pushed root screen '%s' %p (New Depth: 1)\n", s_home_entry.title, (void*)screen);
        return true;
    }

    if (screen == s_home_screen) {
        win_mgr_show_home();
        return true;
    }

    /* 2. Opening an app from Home */
    if (s_active_task_idx == -1) {
        /* Check if screen already belongs to an existing task */
        for (uint8_t i = 0; i < s_task_count; i++) {
            if (s_tasks[i].screen_obj == screen) {
                return win_mgr_task_activate(i);
            }
        }

        if (s_task_count >= OS_MAX_TASKS) {
            win_mgr_task_kill(0);
        }

        int8_t task_idx = (int8_t)s_task_count;
        os_task_internal_t *t = &s_tasks[task_idx];
        memset(t, 0, sizeof(os_task_internal_t));
        t->id = s_next_task_id++;

        const char *title = (hdr && hdr->title[0]) ? hdr->title : "App";
        strncpy(t->name, title, sizeof(t->name) - 1);
        t->screen_obj = screen;
        t->is_backgrounded = false;
        t->stack_depth = 1;

        win_mgr_entry_t *entry = &t->stack[0];
        entry->screen = screen;
        entry->view_type = hdr ? hdr->view_type : VEEBHA_VIEW_TYPE_GENERIC;
        entry->fullscreen_mode = hdr ? hdr->fullscreen_mode : OS_FULLSCREEN_NONE;
        entry->show_battery_hud = hdr ? hdr->show_battery_hud : false;
        strncpy(entry->title, title, sizeof(entry->title) - 1);
        strncpy(entry->lsk_label, lsk ? lsk : "", sizeof(entry->lsk_label) - 1);
        entry->lsk_cb = lsk_cb;
        strncpy(entry->rsk_label, rsk ? rsk : "", sizeof(entry->rsk_label) - 1);
        entry->rsk_cb = rsk_cb;

        s_task_count++;
        s_active_task_idx = task_idx;

        /* Hide Home Screen */
        if (s_group) {
            lv_obj_t *cur_f = lv_group_get_focused(s_group);
            if (cur_f && lv_obj_get_screen(cur_f) == s_home_screen) {
                s_home_entry.focused_obj = cur_f;
            }
        }
        lv_obj_add_flag(s_home_screen, LV_OBJ_FLAG_HIDDEN);

        /* Load new screen */
        lv_obj_clear_flag(screen, LV_OBJ_FLAG_HIDDEN);
        lv_screen_load(screen);
        softkey_bar_set_active_widget(hdr ? hdr->softkey_bar : NULL);
        softkey_set_actions(lsk, lsk_cb, rsk, rsk_cb);
        win_mgr_apply_fullscreen_mode(screen, entry->fullscreen_mode, entry->show_battery_hud);

        if (s_group) {
            if (hdr && hdr->first_item) {
                lv_group_focus_obj(hdr->first_item);
            } else if (lv_group_get_focused(s_group) == NULL) {
                lv_group_focus_next(s_group);
            }
        }

        printf("[WIN_MGR] Pushed screen '%s' %p (New Depth: %u)\n", entry->title, (void*)screen, win_mgr_get_depth());
        return true;
    }

    /* 3. Sub-screen pushed inside active task */
    os_task_internal_t *t = &s_tasks[s_active_task_idx];
    if (t->stack_depth >= WIN_MGR_MAX_DEPTH) {
        fprintf(stderr, "[WIN_MGR] Sub-screen depth exceeded\n");
        return false;
    }

    /* Hide previous sub-screen */
    if (s_group) {
        lv_obj_t *cur_f = lv_group_get_focused(s_group);
        lv_obj_t *prev_scr = t->stack[t->stack_depth - 1].screen;
        if (cur_f && lv_obj_get_screen(cur_f) == prev_scr) {
            t->stack[t->stack_depth - 1].focused_obj = cur_f;
        }
    }
    lv_obj_add_flag(t->stack[t->stack_depth - 1].screen, LV_OBJ_FLAG_HIDDEN);

    win_mgr_entry_t *entry = &t->stack[t->stack_depth];
    entry->screen = screen;
    entry->view_type = hdr ? hdr->view_type : VEEBHA_VIEW_TYPE_GENERIC;
    entry->fullscreen_mode = hdr ? hdr->fullscreen_mode : OS_FULLSCREEN_NONE;
    entry->show_battery_hud = hdr ? hdr->show_battery_hud : false;
    const char *title = (hdr && hdr->title[0]) ? hdr->title : t->name;
    strncpy(entry->title, title, sizeof(entry->title) - 1);
    strncpy(entry->lsk_label, lsk ? lsk : "", sizeof(entry->lsk_label) - 1);
    entry->lsk_cb = lsk_cb;
    strncpy(entry->rsk_label, rsk ? rsk : "", sizeof(entry->rsk_label) - 1);
    entry->rsk_cb = rsk_cb;

    t->stack_depth++;
    t->screen_obj = screen;

    /* Load sub-screen */
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_HIDDEN);
    lv_screen_load(screen);
    softkey_bar_set_active_widget(hdr ? hdr->softkey_bar : NULL);
    softkey_set_actions(lsk, lsk_cb, rsk, rsk_cb);
    win_mgr_apply_fullscreen_mode(screen, entry->fullscreen_mode, entry->show_battery_hud);

    if (s_group) {
        if (hdr && hdr->first_item) {
            lv_group_focus_obj(hdr->first_item);
        } else if (lv_group_get_focused(s_group) == NULL) {
            lv_group_focus_next(s_group);
        }
    }

    printf("[WIN_MGR] Pushed screen '%s' %p (New Depth: %u)\n", entry->title, (void*)screen, win_mgr_get_depth());
    return true;
}

bool win_mgr_pop(void)
{
    if (tpl_dialog_is_active()) {
        tpl_dialog_close();
    }

    if (s_active_task_idx < 0) {
        printf("[WIN_MGR] Cannot pop root/home screen (Depth: %u)\n", win_mgr_get_depth());
        return false;
    }

    os_task_internal_t *t = &s_tasks[s_active_task_idx];

    /* Case A: Popping sub-screen within active task */
    if (t->stack_depth > 1) {
        lv_obj_t *scr_to_delete = t->stack[t->stack_depth - 1].screen;
        t->stack_depth--;

        win_mgr_entry_t *prev = &t->stack[t->stack_depth - 1];
        t->screen_obj = prev->screen;
        if (prev->screen == s_launcher_screen) {
            strncpy(t->name, "VeebhaOS", sizeof(t->name) - 1);
            t->name[sizeof(t->name) - 1] = '\0';
        }

        lv_obj_clear_flag(prev->screen, LV_OBJ_FLAG_HIDDEN);
        lv_screen_load(prev->screen);

        win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)lv_obj_get_user_data(prev->screen);
        softkey_bar_set_active_widget(hdr ? hdr->softkey_bar : NULL);
        softkey_set_actions(prev->lsk_label, prev->lsk_cb, prev->rsk_label, prev->rsk_cb);
        os_fullscreen_mode_t fsm = hdr ? hdr->fullscreen_mode : prev->fullscreen_mode;
        bool fhud = hdr ? hdr->show_battery_hud : prev->show_battery_hud;
        win_mgr_apply_fullscreen_mode(prev->screen, fsm, fhud);

        if (scr_to_delete && lv_obj_is_valid(scr_to_delete)) {
            win_mgr_screen_hdr_t *shdr = (win_mgr_screen_hdr_t *)lv_obj_get_user_data(scr_to_delete);
            if (scr_to_delete == s_launcher_screen || (shdr && shdr->keep_alive)) {
                lv_obj_add_flag(scr_to_delete, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_delete_async(scr_to_delete);
            }
        }

        if (s_group) {
            if (prev->focused_obj && lv_obj_is_valid(prev->focused_obj)) {
                lv_group_focus_obj(prev->focused_obj);
            } else if (hdr && hdr->first_item && lv_obj_is_valid(hdr->first_item)) {
                lv_group_focus_obj(hdr->first_item);
            } else {
                lv_group_focus_next(s_group);
            }
        }

        printf("[WIN_MGR] Popped screen (Remaining Depth: %u, Current Focused: %ld)\n",
               win_mgr_get_depth(), (long)(uintptr_t)(s_group && lv_group_get_focused(s_group) ? lv_obj_get_user_data(lv_group_get_focused(s_group)) : 0));
        return true;
    }

    /* Case B: Popping root screen of active task -> Task terminates */
    lv_obj_t *scr_to_delete = t->stack[0].screen;

    /* Remove task from registry */
    for (uint8_t i = (uint8_t)s_active_task_idx; i < s_task_count - 1; i++) {
        s_tasks[i] = s_tasks[i + 1];
    }
    s_task_count--;
    s_active_task_idx = -1;

    /* Restore Home Screen */
    if (s_home_screen && lv_obj_is_valid(s_home_screen)) {
        lv_obj_clear_flag(s_home_screen, LV_OBJ_FLAG_HIDDEN);
        lv_screen_load(s_home_screen);
        win_mgr_screen_hdr_t *home_hdr = (win_mgr_screen_hdr_t *)lv_obj_get_user_data(s_home_screen);
        softkey_bar_set_active_widget(home_hdr ? home_hdr->softkey_bar : NULL);
        softkey_set_actions(s_home_entry.lsk_label, s_home_entry.lsk_cb,
                            s_home_entry.rsk_label, s_home_entry.rsk_cb);
        win_mgr_apply_fullscreen_mode(s_home_screen, OS_FULLSCREEN_NONE, false);

        if (s_group) {
            if (s_home_entry.focused_obj && lv_obj_is_valid(s_home_entry.focused_obj)) {
                lv_group_focus_obj(s_home_entry.focused_obj);
            } else if (home_hdr && home_hdr->first_item && lv_obj_is_valid(home_hdr->first_item)) {
                lv_group_focus_obj(home_hdr->first_item);
            } else {
                lv_group_focus_next(s_group);
            }
        }
    }

    if (scr_to_delete && lv_obj_is_valid(scr_to_delete)) {
        win_mgr_screen_hdr_t *shdr = (win_mgr_screen_hdr_t *)lv_obj_get_user_data(scr_to_delete);
        if (scr_to_delete == s_launcher_screen || (shdr && shdr->keep_alive)) {
            lv_obj_add_flag(scr_to_delete, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_delete_async(scr_to_delete);
        }
    }

    printf("[WIN_MGR] Popped screen (Remaining Depth: %u, Current Focused: %ld)\n",
           win_mgr_get_depth(), (long)(uintptr_t)(s_group && lv_group_get_focused(s_group) ? lv_obj_get_user_data(lv_group_get_focused(s_group)) : 0));
    return true;
}

void win_mgr_reset_to_home(void)
{
    /* Pop active task completely down to Home */
    while (s_active_task_idx >= 0 && s_tasks[s_active_task_idx].stack_depth > 1) {
        win_mgr_pop();
    }
    if (s_active_task_idx >= 0) {
        win_mgr_pop();
    }
}

bool win_mgr_is_home(uint8_t index)
{
    return (index == 0) || (s_active_task_idx == -1);
}

void win_mgr_show_home(void)
{
    /* Close overlays if open */
    if (tpl_dialog_is_active()) {
        tpl_dialog_close();
    }
    if (notif_panel_is_active()) {
        notif_panel_close();
    }
    if (task_mgr_is_active()) {
        task_mgr_close();
    }

    /* Background active task without deleting */
    if (s_active_task_idx >= 0 && s_active_task_idx < s_task_count) {
        os_task_internal_t *t = &s_tasks[s_active_task_idx];
        if (t->stack_depth == 1 && (t->stack[0].view_type == VEEBHA_VIEW_TYPE_GRID || strcmp(t->name, "VeebhaOS") == 0 || t->stack[0].screen == s_launcher_screen)) {
            win_mgr_task_kill(s_active_task_idx);
        } else {
            /* If launcher screen is at stack[0] under an app, detach it so launcher isn't locked */
            if (t->stack_depth > 1 && t->stack[0].screen == s_launcher_screen) {
                lv_obj_add_flag(s_launcher_screen, LV_OBJ_FLAG_HIDDEN);
                for (uint8_t d = 0; d < t->stack_depth - 1; d++) {
                    t->stack[d] = t->stack[d + 1];
                }
                t->stack_depth--;
            }
            t->is_backgrounded = true;
            if (s_group && t->stack_depth > 0) {
                lv_obj_t *cur_f = lv_group_get_focused(s_group);
                lv_obj_t *prev_scr = t->stack[t->stack_depth - 1].screen;
                if (cur_f && lv_obj_get_screen(cur_f) == prev_scr) {
                    t->stack[t->stack_depth - 1].focused_obj = cur_f;
                }
            }
            if (t->screen_obj && lv_obj_is_valid(t->screen_obj)) {
                lv_obj_add_flag(t->screen_obj, LV_OBJ_FLAG_HIDDEN);
            }
            s_active_task_idx = -1;
        }
    }

    /* Unhide and activate Home Screen */
    if (s_home_screen) {
        lv_obj_clear_flag(s_home_screen, LV_OBJ_FLAG_HIDDEN);
        lv_screen_load(s_home_screen);
        win_mgr_apply_fullscreen_mode(s_home_screen, OS_FULLSCREEN_NONE, false);
        win_mgr_screen_hdr_t *home_hdr = (win_mgr_screen_hdr_t *)lv_obj_get_user_data(s_home_screen);
        softkey_bar_set_active_widget(home_hdr ? home_hdr->softkey_bar : NULL);
        softkey_set_actions(s_home_entry.lsk_label, s_home_entry.lsk_cb,
                            s_home_entry.rsk_label, s_home_entry.rsk_cb);

        if (s_group) {
            if (s_home_entry.focused_obj && lv_obj_is_valid(s_home_entry.focused_obj)) {
                lv_group_focus_obj(s_home_entry.focused_obj);
            } else if (home_hdr && home_hdr->first_item && lv_obj_is_valid(home_hdr->first_item)) {
                lv_group_focus_obj(home_hdr->first_item);
            } else {
                lv_group_focus_next(s_group);
            }
        }
    }

    printf("[WIN_MGR] Switched to '%s' (Depth: %u)\n", s_home_entry.title, win_mgr_get_depth());
}

win_mgr_entry_t * win_mgr_get_entry(uint8_t index)
{
    if (index == 0) return &s_home_entry;

    if (s_active_task_idx >= 0 && s_active_task_idx < s_task_count) {
        os_task_internal_t *t = &s_tasks[s_active_task_idx];
        if (index - 1 < t->stack_depth) {
            return &t->stack[index - 1];
        }
    } else if (index - 1 < s_task_count) {
        os_task_internal_t *t = &s_tasks[index - 1];
        if (t->stack_depth > 0) {
            return &t->stack[t->stack_depth - 1];
        }
    }
    return &s_home_entry;
}

const char * win_mgr_get_title(uint8_t index)
{
    win_mgr_entry_t *e = win_mgr_get_entry(index);
    return (e && e->title[0]) ? e->title : "App";
}

void win_mgr_set_title(const char *title)
{
    if (!title) return;
    win_mgr_entry_t *top = win_mgr_get_top();
    if (top) {
        strncpy(top->title, title, sizeof(top->title) - 1);
        top->title[sizeof(top->title) - 1] = '\0';
    }
}

bool win_mgr_switch_to(uint8_t index)
{
    if (index == 0) {
        win_mgr_show_home();
        return true;
    }
    return win_mgr_task_activate(index - 1);
}

bool win_mgr_close_at(uint8_t index)
{
    if (index == 0) {
        printf("[WIN_MGR] Cannot close Root/Home screen\n");
        return false;
    }
    return win_mgr_task_kill(index - 1);
}

/* ============================================================================
 * Task Pool Management APIs
 * ============================================================================ */

uint8_t win_mgr_get_task_count(void)
{
    return s_task_count;
}

os_task_entry_t * win_mgr_get_task(uint8_t index)
{
    if (index >= s_task_count) return NULL;
    return (os_task_entry_t *)&s_tasks[index];
}

const char * win_mgr_get_task_title(uint8_t index)
{
    if (index >= s_task_count) return "App";
    os_task_internal_t *t = &s_tasks[index];
    if (t->stack_depth > 0 && t->stack[t->stack_depth - 1].title[0] != '\0') {
        return t->stack[t->stack_depth - 1].title;
    }
    return (t->name[0] != '\0') ? t->name : "App";
}

os_task_entry_t * win_mgr_find_task(const char *name)
{
    if (!name || name[0] == '\0') return NULL;
    for (uint8_t i = 0; i < s_task_count; i++) {
        if (os_strcasecmp(s_tasks[i].name, name) == 0) {
            return (os_task_entry_t *)&s_tasks[i];
        }
        for (uint8_t d = 0; d < s_tasks[i].stack_depth; d++) {
            if (os_strcasecmp(s_tasks[i].stack[d].title, name) == 0) {
                return (os_task_entry_t *)&s_tasks[i];
            }
        }
    }
    return NULL;
}

os_task_entry_t * win_mgr_get_active_task(void)
{
    if (s_active_task_idx >= 0 && s_active_task_idx < s_task_count) {
        return (os_task_entry_t *)&s_tasks[s_active_task_idx];
    }
    return NULL;
}

bool win_mgr_task_resume(const char *name)
{
    if (!name || name[0] == '\0') return false;
    for (uint8_t i = 0; i < s_task_count; i++) {
        if (os_strcasecmp(s_tasks[i].name, name) == 0) {
            return win_mgr_task_activate(i);
        }
        for (uint8_t d = 0; d < s_tasks[i].stack_depth; d++) {
            if (os_strcasecmp(s_tasks[i].stack[d].title, name) == 0) {
                return win_mgr_task_activate(i);
            }
        }
    }
    return false;
}

bool win_mgr_task_activate(uint8_t task_idx)
{
    if (task_idx >= s_task_count) return false;

    if (s_active_task_idx == (int8_t)task_idx) {
        return true;
    }

    /* Background currently active foreground view */
    if (s_active_task_idx >= 0 && s_active_task_idx < s_task_count) {
        os_task_internal_t *cur = &s_tasks[s_active_task_idx];
        cur->is_backgrounded = true;
        if (s_group && cur->stack_depth > 0) {
            lv_obj_t *cur_f = lv_group_get_focused(s_group);
            lv_obj_t *prev_scr = cur->stack[cur->stack_depth - 1].screen;
            if (cur_f && lv_obj_get_screen(cur_f) == prev_scr) {
                cur->stack[cur->stack_depth - 1].focused_obj = cur_f;
            }
        }
        if (cur->screen_obj && lv_obj_is_valid(cur->screen_obj)) {
            lv_obj_add_flag(cur->screen_obj, LV_OBJ_FLAG_HIDDEN);
        }
    } else if (s_home_screen) {
        if (s_group) {
            lv_obj_t *cur_f = lv_group_get_focused(s_group);
            if (cur_f && lv_obj_get_screen(cur_f) == s_home_screen) {
                s_home_entry.focused_obj = cur_f;
            }
        }
        lv_obj_add_flag(s_home_screen, LV_OBJ_FLAG_HIDDEN);
    }

    /* Activate target task */
    s_active_task_idx = (int8_t)task_idx;
    os_task_internal_t *t = &s_tasks[task_idx];
    t->is_backgrounded = false;

    lv_obj_clear_flag(t->screen_obj, LV_OBJ_FLAG_HIDDEN);
    lv_screen_load(t->screen_obj);

    win_mgr_entry_t *top = &t->stack[t->stack_depth - 1];
    win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)lv_obj_get_user_data(t->screen_obj);
    os_fullscreen_mode_t fsm = hdr ? hdr->fullscreen_mode : top->fullscreen_mode;
    bool fhud = hdr ? hdr->show_battery_hud : top->show_battery_hud;
    win_mgr_apply_fullscreen_mode(t->screen_obj, fsm, fhud);
    softkey_bar_set_active_widget(hdr ? hdr->softkey_bar : NULL);
    softkey_set_actions(top->lsk_label, top->lsk_cb, top->rsk_label, top->rsk_cb);

    if (s_group) {
        if (top->focused_obj && lv_obj_is_valid(top->focused_obj)) {
            lv_group_focus_obj(top->focused_obj);
        } else if (hdr && hdr->first_item && lv_obj_is_valid(hdr->first_item)) {
            lv_group_focus_obj(hdr->first_item);
        } else {
            lv_group_focus_next(s_group);
        }
    }

    printf("[WIN_MGR] Switched to '%s' (Depth: %u)\n", t->name, win_mgr_get_depth());
    return true;
}

bool win_mgr_task_kill(uint8_t task_idx)
{
    if (task_idx >= s_task_count) return false;

    os_task_internal_t *t = &s_tasks[task_idx];

    /* Delete all screens for this task asynchronously */
    for (uint8_t i = 0; i < t->stack_depth; i++) {
        lv_obj_t *scr = t->stack[i].screen;
        if (scr && lv_obj_is_valid(scr)) {
            win_mgr_screen_hdr_t *shdr = (win_mgr_screen_hdr_t *)lv_obj_get_user_data(scr);
            if (scr == s_launcher_screen || (shdr && shdr->keep_alive)) {
                lv_obj_add_flag(scr, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_delete_async(scr);
            }
        }
    }

    bool was_active = (s_active_task_idx == (int8_t)task_idx);

    /* Shift remaining tasks down */
    for (uint8_t i = task_idx; i < s_task_count - 1; i++) {
        s_tasks[i] = s_tasks[i + 1];
    }
    s_task_count--;

    if (was_active) {
        s_active_task_idx = -1;
        if (s_home_screen) {
            lv_obj_clear_flag(s_home_screen, LV_OBJ_FLAG_HIDDEN);
            lv_screen_load(s_home_screen);
            win_mgr_apply_fullscreen_mode(s_home_screen, OS_FULLSCREEN_NONE, false);
            win_mgr_screen_hdr_t *home_hdr = (win_mgr_screen_hdr_t *)lv_obj_get_user_data(s_home_screen);
            softkey_bar_set_active_widget(home_hdr ? home_hdr->softkey_bar : NULL);
            softkey_set_actions(s_home_entry.lsk_label, s_home_entry.lsk_cb,
                                s_home_entry.rsk_label, s_home_entry.rsk_cb);
            if (s_group) {
                if (s_home_entry.focused_obj && lv_obj_is_valid(s_home_entry.focused_obj)) {
                    lv_group_focus_obj(s_home_entry.focused_obj);
                } else if (home_hdr && home_hdr->first_item && lv_obj_is_valid(home_hdr->first_item)) {
                    lv_group_focus_obj(home_hdr->first_item);
                } else {
                    lv_group_focus_next(s_group);
                }
            }
        }
    } else if (s_active_task_idx > (int8_t)task_idx) {
        s_active_task_idx--;
    }

    printf("[WIN_MGR] Closed screen at index %u (New Depth: %u)\n", task_idx + 1, win_mgr_get_depth());
    return true;
}

void win_mgr_for_each_screen(void (*cb)(lv_obj_t *screen, void *user_data), void *user_data)
{
    if (!cb) return;

    if (s_home_screen && lv_obj_is_valid(s_home_screen)) {
        cb(s_home_screen, user_data);
    }

    for (uint8_t i = 0; i < s_task_count; i++) {
        for (uint8_t j = 0; j < s_tasks[i].stack_depth; j++) {
            lv_obj_t *scr = s_tasks[i].stack[j].screen;
            if (scr && lv_obj_is_valid(scr)) {
                cb(scr, user_data);
            }
        }
    }
}

bool win_mgr_is_screen_in_stack(lv_obj_t *screen)
{
    if (!screen) return false;
    if (s_home_screen == screen) return true;
    for (uint8_t i = 0; i < s_task_count; i++) {
        for (uint8_t j = 0; j < s_tasks[i].stack_depth; j++) {
            if (s_tasks[i].stack[j].screen == screen) {
                return true;
            }
        }
    }
    return false;
}
