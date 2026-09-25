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

static void on_sysmon_delete_cb(lv_event_t *e)
{
    lv_obj_t *scr = lv_event_get_target(e);
    win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)lv_obj_get_user_data(scr);
    if (hdr) {
        free(hdr);
        lv_obj_set_user_data(scr, NULL);
    }
}

void vapp_sys_monitor_launch(const vapp_package_t *pkg)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, 176, 220);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0A0E17), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);

    win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)calloc(1, sizeof(win_mgr_screen_hdr_t));
    if (hdr) {
        hdr->view_type = VEEBHA_VIEW_TYPE_GENERIC;
        strncpy(hdr->title, pkg ? pkg->header.name : "Sys Monitor", sizeof(hdr->title) - 1);
        lv_obj_set_user_data(screen, hdr);
    }
    lv_obj_add_event_cb(screen, on_sysmon_delete_cb, LV_EVENT_DELETE, NULL);

    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_size(content, lv_pct(100), 0);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(content, lv_color_hex(0x131A29), 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 8, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    char buf[160];
    snprintf(buf, sizeof(buf),
             LV_SYMBOL_SETTINGS " SYSTEM MONITOR\n\nMemory Pool: %u KB\nWindow Depth: %u\nKernel: FreeRTOS v10\nStatus: Online",
             (unsigned int)(CONFIG_LV_MEM_SIZE / 1024),
             (unsigned int)win_mgr_get_depth());

    lv_obj_t *lbl = lv_label_create(content);
    lv_label_set_text(lbl, buf);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x00FF88), 0);
    lv_obj_set_style_text_font(lbl, veebha_font_get_default(), 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *sk = softkey_bar_create(screen, "Refresh", "Back");
    if (hdr) hdr->softkey_bar = sk;

    win_mgr_push(screen, "Refresh", NULL, "Back", tpl_list_default_rsk);
}
