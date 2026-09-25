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

#include "sdk/include/veebha_live_pill.h"
#include "sdk/include/veebha_log.h"
#include "sdk/include/veebha_theme.h"
#include <stdio.h>
#include <string.h>

#define TAG "LIVE_PILL"

typedef struct {
    bool                 active;
    char                 icon[16];
    char                 text[64];
    lv_color_t           accent;
    live_pill_click_cb_t on_click;
} live_pill_slot_t;

static live_pill_slot_t s_slots[LIVE_PILL_PRIO_COUNT];
static lv_obj_t        *s_container = NULL;
static lv_obj_t        *s_icon_lbl = NULL;
static lv_obj_t        *s_text_lbl = NULL;

void live_pill_init(void)
{
    memset(s_slots, 0, sizeof(s_slots));
    s_container = NULL;
    s_icon_lbl = NULL;
    s_text_lbl = NULL;
    OS_LOGI(TAG, "Unified Live Pill subsystem initialized");
}

live_pill_priority_t live_pill_get_active_priority(void)
{
    for (int i = LIVE_PILL_PRIO_COUNT - 1; i > (int)LIVE_PILL_PRIO_NONE; i--) {
        if (s_slots[i].active) {
            return (live_pill_priority_t)i;
        }
    }
    return LIVE_PILL_PRIO_NONE;
}

bool live_pill_is_active(void)
{
    return (live_pill_get_active_priority() != LIVE_PILL_PRIO_NONE);
}

const char * live_pill_get_active_text(void)
{
    live_pill_priority_t prio = live_pill_get_active_priority();
    if (prio == LIVE_PILL_PRIO_NONE) return "";
    return s_slots[prio].text;
}

void live_pill_refresh(void)
{
    live_pill_priority_t prio = live_pill_get_active_priority();

    if (!s_container || !lv_obj_is_valid(s_container)) {
        return;
    }

    if (prio == LIVE_PILL_PRIO_NONE) {
        lv_obj_add_flag(s_container, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    const live_pill_slot_t *slot = &s_slots[prio];

    lv_obj_clear_flag(s_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_border_color(s_container, slot->accent, 0);

    if (s_icon_lbl && lv_obj_is_valid(s_icon_lbl)) {
        lv_label_set_text(s_icon_lbl, slot->icon);
        lv_obj_set_style_text_color(s_icon_lbl, slot->accent, 0);
    }

    if (s_text_lbl && lv_obj_is_valid(s_text_lbl)) {
        lv_label_set_text(s_text_lbl, slot->text);
        lv_obj_set_style_text_color(s_text_lbl, slot->accent, 0);
    }
}

void live_pill_publish(live_pill_priority_t prio, const char *icon, const char *text, lv_color_t accent, live_pill_click_cb_t on_click)
{
    if (prio <= LIVE_PILL_PRIO_NONE || prio >= LIVE_PILL_PRIO_COUNT) return;

    live_pill_slot_t *slot = &s_slots[prio];
    slot->active = true;
    strncpy(slot->icon, icon ? icon : "", sizeof(slot->icon) - 1);
    slot->icon[sizeof(slot->icon) - 1] = '\0';
    strncpy(slot->text, text ? text : "", sizeof(slot->text) - 1);
    slot->text[sizeof(slot->text) - 1] = '\0';
    slot->accent = accent;
    slot->on_click = on_click;

    OS_LOGD(TAG, "Published priority %d: icon='%s', text='%s'", (int)prio, slot->icon, slot->text);
    live_pill_refresh();
}

void live_pill_clear(live_pill_priority_t prio)
{
    if (prio <= LIVE_PILL_PRIO_NONE || prio >= LIVE_PILL_PRIO_COUNT) return;

    if (s_slots[prio].active) {
        memset(&s_slots[prio], 0, sizeof(live_pill_slot_t));
        OS_LOGD(TAG, "Cleared priority %d", (int)prio);
        live_pill_refresh();
    }
}

void live_pill_bind_ui(lv_obj_t *container, lv_obj_t *icon_lbl, lv_obj_t *text_lbl)
{
    s_container = container;
    s_icon_lbl = icon_lbl;
    s_text_lbl = text_lbl;
    live_pill_refresh();
}

void live_pill_unbind_ui(void)
{
    s_container = NULL;
    s_icon_lbl = NULL;
    s_text_lbl = NULL;
}

void live_pill_trigger_click(void)
{
    live_pill_priority_t prio = live_pill_get_active_priority();
    if (prio != LIVE_PILL_PRIO_NONE && s_slots[prio].on_click) {
        OS_LOGI(TAG, "Triggered click action for priority %d", (int)prio);
        s_slots[prio].on_click();
    }
}
