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

#include "veebha_irq.h"
#include "veebha_event.h"
#include "veebha_log.h"
#include <stdio.h>
#include <string.h>

#define TAG "IRQ_MGR"

typedef struct {
    os_isr_handler_t handler;
    void            *arg;
} irq_slot_t;

static irq_slot_t s_irq_table[IRQ_MAX_VECTORS];
static uint16_t   s_irq_enabled_mask = 0;
static bool       s_sd_mounted = false;

/* Static storage for simulator trigger arguments */
static struct {
    char number[20];
    char name[24];
} s_mock_call_data;

static struct {
    char sender[24];
    char preview[48];
} s_mock_sms_data;

/* Default Top-Half Interrupt Service Routines */
static void default_modem_ring_isr(void *arg)
{
    (void)arg;
    os_event_t evt;
    memset(&evt, 0, sizeof(evt));
    evt.type = OS_EVT_CALL_INCOMING;
    strncpy(evt.payload.call.number, s_mock_call_data.number[0] ? s_mock_call_data.number : "+15550123", sizeof(evt.payload.call.number) - 1);
    strncpy(evt.payload.call.name, s_mock_call_data.name[0] ? s_mock_call_data.name : "Alice Smith", sizeof(evt.payload.call.name) - 1);
    os_event_post_from_isr(&evt);
}

static void default_modem_sms_isr(void *arg)
{
    (void)arg;
    os_event_t evt;
    memset(&evt, 0, sizeof(evt));
    evt.type = OS_EVT_SMS_RECEIVED;
    strncpy(evt.payload.sms.sender, s_mock_sms_data.sender[0] ? s_mock_sms_data.sender : "Bob Jones", sizeof(evt.payload.sms.sender) - 1);
    strncpy(evt.payload.sms.preview, s_mock_sms_data.preview[0] ? s_mock_sms_data.preview : "Meeting at 3pm?", sizeof(evt.payload.sms.preview) - 1);
    os_event_post_from_isr(&evt);
}

static void default_sdmmc_isr(void *arg)
{
    (void)arg;
    s_sd_mounted = !s_sd_mounted;
    os_event_t evt;
    memset(&evt, 0, sizeof(evt));
    evt.type = OS_EVT_SDCARD_HOTPLUG;
    evt.payload.sdcard.inserted = s_sd_mounted;
    os_event_post_from_isr(&evt);
}

void os_irq_init(void)
{
    memset(s_irq_table, 0, sizeof(s_irq_table));
    s_irq_enabled_mask = 0;
    s_sd_mounted = false;

    /* Register default hardware driver ISRs */
    os_irq_register(IRQ_MODEM_RING, default_modem_ring_isr, NULL);
    os_irq_enable(IRQ_MODEM_RING);

    os_irq_register(IRQ_MODEM_SMS, default_modem_sms_isr, NULL);
    os_irq_enable(IRQ_MODEM_SMS);

    os_irq_register(IRQ_SDMMC_DETECT, default_sdmmc_isr, NULL);
    os_irq_enable(IRQ_SDMMC_DETECT);

    OS_LOGI(TAG, "Hardware IRQ dispatcher initialized (RDA8809 INTC: 0x%08lX)", RDA8809_INTC_BASE);
}

bool os_irq_register(os_irq_vector_t vector, os_isr_handler_t handler, void *arg)
{
    if (vector == IRQ_NONE || vector >= IRQ_MAX_VECTORS) return false;

    s_irq_table[vector].handler = handler;
    s_irq_table[vector].arg = arg;
    return true;
}

void os_irq_enable(os_irq_vector_t vector)
{
    if (vector < IRQ_MAX_VECTORS) {
        s_irq_enabled_mask |= (1 << vector);
    }
}

void os_irq_disable(os_irq_vector_t vector)
{
    if (vector < IRQ_MAX_VECTORS) {
        s_irq_enabled_mask &= ~(1 << vector);
    }
}

void os_irq_dispatch(os_irq_vector_t vector)
{
    if (vector < IRQ_MAX_VECTORS && (s_irq_enabled_mask & (1 << vector))) {
        if (s_irq_table[vector].handler) {
            s_irq_table[vector].handler(s_irq_table[vector].arg);
        }
    }
}

void os_irq_sim_trigger_call(const char *number, const char *caller_name)
{
    if (number) {
        strncpy(s_mock_call_data.number, number, sizeof(s_mock_call_data.number) - 1);
        s_mock_call_data.number[sizeof(s_mock_call_data.number) - 1] = '\0';
    } else {
        s_mock_call_data.number[0] = '\0';
    }

    if (caller_name) {
        strncpy(s_mock_call_data.name, caller_name, sizeof(s_mock_call_data.name) - 1);
        s_mock_call_data.name[sizeof(s_mock_call_data.name) - 1] = '\0';
    } else {
        s_mock_call_data.name[0] = '\0';
    }

    OS_LOGI(TAG, "Hardware IRQ triggered: IRQ_MODEM_RING (Vector %d)", IRQ_MODEM_RING);
    os_irq_dispatch(IRQ_MODEM_RING);
}

void os_irq_sim_trigger_sms(const char *sender, const char *preview)
{
    if (sender) {
        strncpy(s_mock_sms_data.sender, sender, sizeof(s_mock_sms_data.sender) - 1);
        s_mock_sms_data.sender[sizeof(s_mock_sms_data.sender) - 1] = '\0';
    } else {
        s_mock_sms_data.sender[0] = '\0';
    }

    if (preview) {
        strncpy(s_mock_sms_data.preview, preview, sizeof(s_mock_sms_data.preview) - 1);
        s_mock_sms_data.preview[sizeof(s_mock_sms_data.preview) - 1] = '\0';
    } else {
        s_mock_sms_data.preview[0] = '\0';
    }

    OS_LOGI(TAG, "Hardware IRQ triggered: IRQ_MODEM_SMS (Vector %d)", IRQ_MODEM_SMS);
    os_irq_dispatch(IRQ_MODEM_SMS);
}

void os_irq_sim_trigger_sdcard_toggle(void)
{
    OS_LOGI(TAG, "Hardware IRQ triggered: IRQ_SDMMC_DETECT (Vector %d)", IRQ_SDMMC_DETECT);
    os_irq_dispatch(IRQ_SDMMC_DETECT);
}
