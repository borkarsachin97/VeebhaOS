/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "vapp_sys_monitor.h"
#include "boards/board_config.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_templates.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_log.h"
#include "sdk/text/font_fallback.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "VAPP_SYSMON"

typedef struct {
    win_mgr_screen_hdr_t hdr;
    lv_obj_t            *screen;
    lv_obj_t            *lbl;
} sysmon_state_t;

static void update_sysmon_text(sysmon_state_t *st)
{
    if (!st || !st->lbl || !lv_obj_is_valid(st->lbl)) return;

    char buf[192];
    snprintf(buf, sizeof(buf),
             LV_SYMBOL_SETTINGS " SYSTEM MONITOR\n\nMemory Pool: %u KB\nWindow Depth: %u\nTasks Active: %u\nKernel: FreeRTOS v10\nStatus: Online",
             (unsigned int)(CONFIG_LV_MEM_SIZE / 1024),
             (unsigned int)win_mgr_get_depth(),
             (unsigned int)win_mgr_get_task_count());
    lv_label_set_text(st->lbl, buf);
}

static void on_sysmon_refresh_action(void)
{
    win_mgr_entry_t *top = win_mgr_get_top();
    if (top && top->screen && lv_obj_is_valid(top->screen)) {
        sysmon_state_t *st = (sysmon_state_t *)lv_obj_get_user_data(top->screen);
        if (st) {
            update_sysmon_text(st);
            OS_LOGI(TAG, "System Monitor stats refreshed (Depth: %u)", (unsigned int)win_mgr_get_depth());
        }
    }
}

static void on_sysmon_key_cb(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_ENTER || key == '5' || key == ' ') {
        on_sysmon_refresh_action();
    }
}

static void on_sysmon_delete_cb(lv_event_t *e)
{
    lv_obj_t *scr = lv_event_get_target(e);
    sysmon_state_t *st = (sysmon_state_t *)lv_obj_get_user_data(scr);
    if (st) {
        free(st);
        lv_obj_set_user_data(scr, NULL);
    }
}

void vapp_sys_monitor_launch(const vapp_package_t *pkg)
{
    sysmon_state_t *st = (sysmon_state_t *)calloc(1, sizeof(sysmon_state_t));
    if (!st) return;

    lv_obj_t *screen = lv_obj_create(NULL);
    st->screen = screen;
    lv_obj_set_size(screen, 176, 220);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0A0E17), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);

    st->hdr.view_type = VEEBHA_VIEW_TYPE_GENERIC;
    st->hdr.fullscreen_mode = OS_FULLSCREEN_FULL;
    strncpy(st->hdr.title, pkg ? pkg->header.name : "Sys Monitor", sizeof(st->hdr.title) - 1);
    lv_obj_set_user_data(screen, st);
    lv_obj_add_event_cb(screen, on_sysmon_delete_cb, LV_EVENT_DELETE, NULL);

    lv_obj_t *content = lv_button_create(screen);
    st->hdr.first_item = content;
    lv_obj_set_size(content, lv_pct(100), 0);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(content, lv_color_hex(0x131A29), 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 8, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_add_event_cb(content, on_sysmon_key_cb, LV_EVENT_KEY, st);

    lv_obj_t *lbl = lv_label_create(content);
    st->lbl = lbl;
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x00FF88), 0);
    lv_obj_set_style_text_font(lbl, veebha_font_get_default(), 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);

    update_sysmon_text(st);

    lv_obj_t *sk = softkey_bar_create(screen, "Refresh", "Back");
    st->hdr.softkey_bar = sk;

    win_mgr_push(screen, "Refresh", on_sysmon_refresh_action, "Back", tpl_list_default_rsk);

    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_add_obj(g, content);
        lv_group_focus_obj(content);
        lv_group_set_editing(g, true);
    }
}
