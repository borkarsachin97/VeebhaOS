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

#include "apps/contacts/app_contacts.h"
#include "apps/dialer/app_dialer.h"
#include "apps/common/mock_telephony.h"
#include "veebha_win_mgr.h"
#include "veebha_templates.h"
#include <stdio.h>
#include <string.h>

static tpl_list_item_t s_contact_items[1 + TELEPHONY_MAX_CONTACTS];
static char s_new_contact_name[32] = {0};
static char s_new_contact_number[20] = {0};

/* Forward declarations */
static void open_add_contact_number(const char *name);
static void on_contact_saved_confirm(void);

static void on_contact_select(uint16_t index)
{
    uint16_t count = 0;
    contact_record_t *contacts = telephony_get_contacts(&count);

    if (index == 0) {
        /* [+ Add Contact] selected */
        printf("[CONTACTS] Starting Add Contact flow...\n");
        s_new_contact_name[0] = '\0';
        s_new_contact_number[0] = '\0';

        tpl_editor_desc_t desc = {
            .title = "Contact Name",
            .buffer = s_new_contact_name,
            .max_len = sizeof(s_new_contact_name) - 1,
            .on_save = open_add_contact_number,
            .on_cancel = NULL,
            .lsk_label = "Next",
            .rsk_label = "Cancel"
        };
        lv_obj_t *editor_scr = tpl_editor_create(&desc);
        if (editor_scr) {
            win_mgr_push(editor_scr, "Next", tpl_editor_default_lsk, "Cancel", tpl_editor_default_rsk);
        }
    } else if (index - 1 < count) {
        /* Existing contact selected: initiate direct call */
        contact_record_t *c = &contacts[index - 1];
        printf("[CONTACTS] Selected '%s' (%s) -> Initiating call\n", c->name, c->number);
        app_dialer_start_call_to(c->name, c->number);
    }
}

static void on_save_number_done(const char *number)
{
    if (number && number[0] != '\0') {
        strncpy(s_new_contact_number, number, sizeof(s_new_contact_number) - 1);
        s_new_contact_number[sizeof(s_new_contact_number) - 1] = '\0';
    } else if (s_new_contact_number[0] == '\0') {
        strncpy(s_new_contact_number, "5559999", sizeof(s_new_contact_number) - 1);
    }

    if (s_new_contact_name[0] == '\0') {
        strncpy(s_new_contact_name, "Charlie", sizeof(s_new_contact_name) - 1);
    }

    bool ok = telephony_add_contact(s_new_contact_name, s_new_contact_number);
    if (ok) {
        static char msg_buf[64];
        snprintf(msg_buf, sizeof(msg_buf), "Saved %s (%s)", s_new_contact_name, s_new_contact_number);
        tpl_dialog_desc_t dlg = {
            .title = "Contact Saved",
            .message = msg_buf,
            .icon = LV_SYMBOL_OK,
            .on_confirm = on_contact_saved_confirm,
            .on_cancel = on_contact_saved_confirm,
            .lsk_label = "OK",
            .rsk_label = "OK"
        };
        tpl_dialog_show(&dlg);
    } else {
        win_mgr_pop();
        win_mgr_pop();
    }
}

static void open_add_contact_number(const char *name)
{
    if (name && name[0] != '\0') {
        strncpy(s_new_contact_name, name, sizeof(s_new_contact_name) - 1);
        s_new_contact_name[sizeof(s_new_contact_name) - 1] = '\0';
    } else if (s_new_contact_name[0] == '\0') {
        strncpy(s_new_contact_name, "Charlie", sizeof(s_new_contact_name) - 1);
    }

    printf("[CONTACTS] Name entered: '%s', prompting for number...\n", s_new_contact_name);

    s_new_contact_number[0] = '\0';

    tpl_editor_desc_t desc = {
        .title = "Phone Number",
        .buffer = s_new_contact_number,
        .max_len = sizeof(s_new_contact_number) - 1,
        .on_save = on_save_number_done,
        .on_cancel = NULL,
        .lsk_label = "Save",
        .rsk_label = "Cancel"
    };
    lv_obj_t *editor_scr = tpl_editor_create(&desc);
    if (editor_scr) {
        win_mgr_push(editor_scr, "Save", tpl_editor_default_lsk, "Cancel", tpl_editor_default_rsk);
    }
}

static void on_contact_saved_confirm(void)
{
    /* Pop number editor and name editor back to contacts list */
    win_mgr_pop();
    win_mgr_pop();

    /* Re-open contacts list to refresh with the new contact */
    win_mgr_pop();
    app_contacts_open();
}

#include "sdk/include/veebha_i18n.h"

void app_contacts_open(void)
{
    uint16_t count = 0;
    contact_record_t *contacts = telephony_get_contacts(&count);

    /* Row 0: [+ Add Contact] */
    s_contact_items[0].icon = LV_SYMBOL_PLUS;
    s_contact_items[0].title = "+ Contact";
    s_contact_items[0].subtext = NULL;

    /* Rows 1..N: Contacts directory */
    for (uint16_t i = 0; i < count && i < TELEPHONY_MAX_CONTACTS; i++) {
        s_contact_items[1 + i].icon = LV_SYMBOL_CALL;
        s_contact_items[1 + i].title = contacts[i].name;
        s_contact_items[1 + i].subtext = contacts[i].number;
    }

    tpl_list_view_t view_desc = {
        .title = veebha_i18n_str(STR_CONTACTS),
        .items = s_contact_items,
        .count = 1 + count,
        .on_select = on_contact_select,
        .on_back = NULL, /* Defaults to win_mgr_pop() */
        .lsk_label = veebha_i18n_str(STR_CALL),
        .rsk_label = veebha_i18n_str(STR_BACK)
    };

    lv_obj_t *screen = tpl_list_create(&view_desc);
    if (screen) {
        win_mgr_push(screen, veebha_i18n_str(STR_CALL), tpl_list_default_lsk, veebha_i18n_str(STR_BACK), tpl_list_default_rsk);
        printf("[CONTACTS] Directory opened with %u contacts\n", count);
    }
}
