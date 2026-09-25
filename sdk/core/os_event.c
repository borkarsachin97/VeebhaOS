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

#include "veebha_event.h"
#include "veebha_log.h"
#include "veebha_templates.h"
#include "apps/messages/app_messages.h"
#include "apps/common/mock_telephony.h"
#include "apps/home/app_idle.h"
#include "apps/telephony/app_incall.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

#define TAG "OS_EVENT"

static QueueHandle_t s_event_queue = NULL;

void os_event_init(void)
{
    if (!s_event_queue) {
        s_event_queue = xQueueCreate(OS_EVENT_QUEUE_SIZE, sizeof(os_event_t));
    } else {
        xQueueReset(s_event_queue);
    }
    OS_LOGI(TAG, "FreeRTOS Event queue initialized (Capacity: %d, Handle: %p)",
            OS_EVENT_QUEUE_SIZE, (void*)s_event_queue);
}

bool os_event_post(const os_event_t *event)
{
    if (!event) return false;

    if (!s_event_queue) {
        os_event_init();
    }

    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        BaseType_t res = xQueueSend(s_event_queue, event, 0);
        if (res != pdTRUE) {
            OS_LOGW(TAG, "Event queue overflow! Dropping event type %d", (int)event->type);
            return false;
        }
        return true;
    } else {
        return (xQueueSend(s_event_queue, event, 0) == pdTRUE);
    }
}

bool os_event_post_from_isr(const os_event_t *event)
{
    if (!event || !s_event_queue) return false;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t res = xQueueSendFromISR(s_event_queue, event, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    return (res == pdTRUE);
}

bool os_event_poll(os_event_t *event)
{
    if (!event || !s_event_queue) return false;
    return (xQueueReceive(s_event_queue, event, 0) == pdTRUE);
}

bool os_event_wait(os_event_t *event, uint32_t timeout_ms)
{
    if (!event || !s_event_queue) return false;
    TickType_t ticks = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return (xQueueReceive(s_event_queue, event, ticks) == pdTRUE);
}

uint8_t os_event_pending_count(void)
{
    if (!s_event_queue) return 0;
    return (uint8_t)uxQueueMessagesWaiting(s_event_queue);
}

static char s_inc_name[32] = {0};
static char s_inc_number[24] = {0};

static void on_incoming_call_answer(void)
{
    OS_LOGI(TAG, "Incoming call answered");
    app_incall_start(s_inc_name, s_inc_number, CALL_TYPE_INCOMING);
}

static void on_incoming_call_reject(void)
{
    OS_LOGI(TAG, "Incoming call rejected");
    telephony_add_call_log(s_inc_name, s_inc_number, CALL_TYPE_MISSED, 0, "Just now");
    telephony_store_save();
    app_idle_update();
}

static char s_sms_sender[32] = {0};
static char s_sms_preview[64] = {0};

static void on_incoming_sms_view(void)
{
    OS_LOGI(TAG, "Incoming SMS view selected");
    app_messages_open();
}

static void on_incoming_sms_dismiss(void)
{
    OS_LOGI(TAG, "Incoming SMS dismissed");
    app_idle_update();
}

void os_dispatch_system_event(const os_event_t *event)
{
    if (!event) return;

    switch (event->type) {
    case OS_EVT_CALL_INCOMING: {
        strncpy(s_inc_name, event->payload.call.name, sizeof(s_inc_name) - 1);
        s_inc_name[sizeof(s_inc_name) - 1] = '\0';
        strncpy(s_inc_number, event->payload.call.number, sizeof(s_inc_number) - 1);
        s_inc_number[sizeof(s_inc_number) - 1] = '\0';

        char msg_buf[64];
        snprintf(msg_buf, sizeof(msg_buf), "%s\n%s",
                 s_inc_name[0] ? s_inc_name : "Unknown",
                 s_inc_number);
        OS_LOGI(TAG, "Incoming call from %s (%s)", s_inc_name, s_inc_number);

        tpl_dialog_desc_t desc = {
            .title = "Incoming Call",
            .message = msg_buf,
            .lsk_label = "Answer",
            .on_confirm = on_incoming_call_answer,
            .rsk_label = "Reject",
            .on_cancel = on_incoming_call_reject
        };
        tpl_dialog_show(&desc);
        break;
    }

    case OS_EVT_SMS_RECEIVED: {
        strncpy(s_sms_sender, event->payload.sms.sender, sizeof(s_sms_sender) - 1);
        s_sms_sender[sizeof(s_sms_sender) - 1] = '\0';
        strncpy(s_sms_preview, event->payload.sms.preview, sizeof(s_sms_preview) - 1);
        s_sms_preview[sizeof(s_sms_preview) - 1] = '\0';

        OS_LOGI(TAG, "Incoming SMS from '%s': \"%s\"", s_sms_sender, s_sms_preview);
        telephony_receive_sms(s_sms_sender, s_sms_preview);
        app_idle_update();

        static char msg_buf[128];
        snprintf(msg_buf, sizeof(msg_buf), "From: %s\n%s",
                 s_sms_sender[0] ? s_sms_sender : "Unknown",
                 s_sms_preview);

        tpl_dialog_desc_t desc = {
            .title = "New Message",
            .message = msg_buf,
            .icon = LV_SYMBOL_ENVELOPE,
            .lsk_label = "Read",
            .on_confirm = on_incoming_sms_view,
            .rsk_label = "Dismiss",
            .on_cancel = on_incoming_sms_dismiss
        };
        tpl_dialog_show(&desc);
        break;
    }

    case OS_EVT_SDCARD_HOTPLUG: {
        OS_LOGI(TAG, "SD Card %s detected",
                event->payload.sdcard.inserted ? "INSERTION" : "REMOVAL");
        break;
    }

    case OS_EVT_BATTERY_UPDATE: {
        OS_LOGD(TAG, "Battery level update: %u%% (charging: %d)",
                event->payload.battery.percentage, (int)event->payload.battery.is_charging);
        break;
    }

    case OS_EVT_NETWORK_UPDATE: {
        OS_LOGD(TAG, "Network update: %s, %u bars",
                event->payload.network.network_name, event->payload.network.signal_bars);
        break;
    }

    default:
        break;
    }
}
