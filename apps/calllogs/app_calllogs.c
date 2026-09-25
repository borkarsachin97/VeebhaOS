/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "apps/calllogs/app_calllogs.h"
#include "apps/common/mock_telephony.h"
#include "apps/dialer/app_dialer.h"
#include "veebha_win_mgr.h"
#include "veebha_templates.h"
#include "sdk/include/veebha_i18n.h"
#include <stdio.h>
#include <string.h>

static tpl_list_item_t s_call_items[TELEPHONY_MAX_CALL_LOGS];
static char s_call_subtexts[TELEPHONY_MAX_CALL_LOGS][64];

static void on_calllog_select(uint16_t index)
{
    uint16_t count = 0;
    call_log_entry_t *logs = telephony_get_call_logs(&count);
    if (index < count) {
        call_log_entry_t *entry = &logs[index];
        printf("[CALLLOGS] Selected '%s' (%s) -> Calling back\n", entry->name, entry->number);
        app_dialer_start_call_to(entry->name, entry->number);
    }
}

void app_calllogs_open(void)
{
    uint16_t count = 0;
    call_log_entry_t *logs = telephony_get_call_logs(&count);

    if (count == 0) {
        s_call_items[0].icon = LV_SYMBOL_CALL;
        s_call_items[0].title = "No Recent Calls";
        s_call_items[0].subtext = "Call history is empty";
        count = 1;
    } else {
        for (uint16_t i = 0; i < count && i < TELEPHONY_MAX_CALL_LOGS; i++) {
            call_log_entry_t *entry = &logs[i];

            if (entry->type == CALL_TYPE_OUTGOING) {
                s_call_items[i].icon = LV_SYMBOL_RIGHT;
            } else if (entry->type == CALL_TYPE_INCOMING) {
                s_call_items[i].icon = LV_SYMBOL_LEFT;
            } else {
                s_call_items[i].icon = LV_SYMBOL_WARNING;
            }

            s_call_items[i].title = entry->name;

            if (entry->type == CALL_TYPE_MISSED) {
                snprintf(s_call_subtexts[i], sizeof(s_call_subtexts[0]), "%s • Missed", entry->timestamp);
            } else {
                uint32_t mins = entry->duration / 60;
                uint32_t secs = entry->duration % 60;
                snprintf(s_call_subtexts[i], sizeof(s_call_subtexts[0]), "%s • %02u:%02u",
                         entry->timestamp, (unsigned int)mins, (unsigned int)secs);
            }
            s_call_items[i].subtext = s_call_subtexts[i];
        }
    }

    tpl_list_view_t desc = {
        .title = veebha_i18n_str(STR_CALL_LOGS),
        .items = s_call_items,
        .count = count,
        .on_select = on_calllog_select,
        .on_back = NULL, /* Defaults to win_mgr_pop() */
        .lsk_label = veebha_i18n_str(STR_CALL),
        .rsk_label = veebha_i18n_str(STR_BACK)
    };

    lv_obj_t *screen = tpl_list_create(&desc);
    if (screen) {
        win_mgr_push(screen, veebha_i18n_str(STR_CALL), tpl_list_default_lsk, veebha_i18n_str(STR_BACK), tpl_list_default_rsk);
        printf("[CALLLOGS] Call Logs view opened with %u entries\n", count);
    }
}
