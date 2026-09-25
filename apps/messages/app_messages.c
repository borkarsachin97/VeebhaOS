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

#include "apps/messages/app_messages.h"
#include "apps/common/mock_telephony.h"
#include "veebha_win_mgr.h"
#include "veebha_templates.h"
#include <stdio.h>
#include <string.h>

static tpl_list_item_t s_inbox_items[1 + TELEPHONY_MAX_MESSAGES];
static char s_inbox_subtexts[TELEPHONY_MAX_MESSAGES][128];
static char s_sms_recipient[32] = {0};
static char s_sms_body[256] = {0};

/* Forward declarations */
static void on_send_sms_done(const char *text);
static void on_sms_sent_confirm(void);
static void on_message_reply(void);

static void on_message_select(uint16_t index)
{
    uint16_t count = 0;
    sms_message_t *msgs = telephony_get_messages(&count);

    if (index == 0) {
        /* [+ New Message] selected */
        app_messages_compose_to(NULL);
    } else if (index - 1 < count) {
        /* Open existing SMS conversation/viewer */
        sms_message_t *msg = &msgs[index - 1];
        msg->is_read = true;

        strncpy(s_sms_recipient, msg->sender, sizeof(s_sms_recipient) - 1);
        s_sms_recipient[sizeof(s_sms_recipient) - 1] = '\0';

        static char s_view_header[64];
        snprintf(s_view_header, sizeof(s_view_header), "%s (%s)", msg->sender, msg->timestamp);

        tpl_dialog_desc_t dlg = {
            .title = s_view_header,
            .message = msg->body,
            .icon = LV_SYMBOL_ENVELOPE,
            .on_confirm = on_message_reply,
            .on_cancel = NULL, /* Close modal */
            .lsk_label = "Reply",
            .rsk_label = "Back"
        };
        tpl_dialog_show(&dlg);
    }
}

static void on_message_reply(void)
{
    /* Reply to current conversation recipient */
    app_messages_compose_to(s_sms_recipient);
}

static void on_send_sms_done(const char *text)
{
    const char *target = (s_sms_recipient[0] != '\0') ? s_sms_recipient : "Alice Smith";
    const char *content = (text && text[0] != '\0') ? text : (s_sms_body[0] != '\0' ? s_sms_body : "Hello from VeebhaOS");

    bool ok = telephony_send_sms(target, content);
    if (ok) {
        static char msg_buf[64];
        snprintf(msg_buf, sizeof(msg_buf), "Sent to %s", target);
        tpl_dialog_desc_t dlg = {
            .title = "Message Sent!",
            .message = msg_buf,
            .icon = LV_SYMBOL_OK,
            .on_confirm = on_sms_sent_confirm,
            .on_cancel = on_sms_sent_confirm,
            .lsk_label = "OK",
            .rsk_label = "OK"
        };
        tpl_dialog_show(&dlg);
    } else {
        win_mgr_pop();
    }
}

static void on_sms_sent_confirm(void)
{
    /* Pop editor screen */
    win_mgr_pop();

    /* If Inbox was under it, refresh inbox */
    win_mgr_pop();
    app_messages_open();
}

#include "sdk/include/veebha_i18n.h"

void app_messages_compose_to(const char *recipient)
{
    if (recipient && recipient != s_sms_recipient) {
        strncpy(s_sms_recipient, recipient, sizeof(s_sms_recipient) - 1);
        s_sms_recipient[sizeof(s_sms_recipient) - 1] = '\0';
    } else if (!recipient) {
        strncpy(s_sms_recipient, "Alice Smith", sizeof(s_sms_recipient) - 1);
        s_sms_recipient[sizeof(s_sms_recipient) - 1] = '\0';
    }
    s_sms_body[0] = '\0';

    tpl_editor_desc_t desc = {
        .title = veebha_i18n_str(STR_NEW_MESSAGE),
        .buffer = s_sms_body,
        .max_len = sizeof(s_sms_body) - 1,
        .on_save = on_send_sms_done,
        .on_cancel = NULL,
        .lsk_label = "Send",
        .rsk_label = veebha_i18n_str(STR_CANCEL)
    };

    lv_obj_t *editor_scr = tpl_editor_create(&desc);
    if (editor_scr) {
        win_mgr_push(editor_scr, "Send", tpl_editor_default_lsk, veebha_i18n_str(STR_CANCEL), tpl_editor_default_rsk);
        printf("[MESSAGES] Opened composer to '%s'\n", s_sms_recipient);
    }
}

void app_messages_open(void)
{
    uint16_t count = 0;
    sms_message_t *msgs = telephony_get_messages(&count);

    /* Row 0: [+ New Message] */
    s_inbox_items[0].icon = LV_SYMBOL_PLUS;
    s_inbox_items[0].title = veebha_i18n_str(STR_NEW_MESSAGE);
    s_inbox_items[0].subtext = NULL;

    /* Rows 1..N: SMS Threads */
    for (uint16_t i = 0; i < count && i < TELEPHONY_MAX_MESSAGES; i++) {
        s_inbox_items[1 + i].icon = msgs[i].is_read ? LV_SYMBOL_ENVELOPE : LV_SYMBOL_BELL;
        s_inbox_items[1 + i].title = msgs[i].sender;

        snprintf(s_inbox_subtexts[i], sizeof(s_inbox_subtexts[0]), "%s • %s",
                 msgs[i].timestamp, msgs[i].preview);
        s_inbox_items[1 + i].subtext = s_inbox_subtexts[i];
    }

    tpl_list_view_t view_desc = {
        .title = veebha_i18n_str(STR_MESSAGES),
        .items = s_inbox_items,
        .count = 1 + count,
        .on_select = on_message_select,
        .on_back = NULL, /* Defaults to win_mgr_pop() */
        .lsk_label = veebha_i18n_str(STR_SELECT),
        .rsk_label = veebha_i18n_str(STR_BACK)
    };

    lv_obj_t *screen = tpl_list_create(&view_desc);
    if (screen) {
        win_mgr_push(screen, veebha_i18n_str(STR_SELECT), tpl_list_default_lsk, veebha_i18n_str(STR_BACK), tpl_list_default_rsk);
        printf("[MESSAGES] Inbox opened with %u threads\n", count);
    }
}
