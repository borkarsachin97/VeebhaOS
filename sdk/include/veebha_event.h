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

#ifndef SDK_INCLUDE_VEEBHA_EVENT_H
#define SDK_INCLUDE_VEEBHA_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "drivers/hal_input.h"

#define OS_EVENT_QUEUE_SIZE 32

typedef enum {
    OS_EVT_NONE = 0,
    OS_EVT_KEY_DOWN,
    OS_EVT_KEY_UP,
    OS_EVT_CALL_INCOMING,
    OS_EVT_SMS_RECEIVED,
    OS_EVT_BATTERY_UPDATE,
    OS_EVT_NETWORK_UPDATE,
    OS_EVT_SDCARD_HOTPLUG,
    OS_EVT_ALARM_FIRED,
    OS_EVT_BT_STATE_CHANGED,
    OS_EVT_BT_PAIR_REQUEST,
    OS_EVT_USB_INSERTED,
    OS_EVT_USB_REMOVED,
    OS_EVT_CUSTOM
} os_event_type_t;

typedef struct {
    os_event_type_t type;
    uint32_t timestamp;
    union {
        struct {
            veebha_key_t key;
            uint32_t duration_ms;
        } key;
        struct {
            char number[20];
            char name[24];
        } call;
        struct {
            char sender[24];
            char preview[48];
        } sms;
        struct {
            uint8_t percentage;
            bool is_charging;
        } battery;
        struct {
            uint8_t signal_bars;
            char network_name[16];
        } network;
        struct {
            bool inserted;
        } sdcard;
        struct {
            uint8_t bt_state;
        } bt;
        struct {
            char dev_name[32];
            char passkey[8];
        } bt_pair;
        struct {
            uint8_t usb_mode;
        } usb;
        void *custom_payload;
    } payload;
} os_event_t;

/**
 * Initialize the system event queue static circular buffer.
 */
void os_event_init(void);

/**
 * Post an event to the system event queue (thread-safe).
 *
 * @param event Pointer to event structure to copy into queue.
 * @return true if enqueued, false if queue is full.
 */
bool os_event_post(const os_event_t *event);

/**
 * Post an event from an ISR context (lockless/critical section).
 *
 * @param event Pointer to event structure.
 * @return true if enqueued, false if queue is full.
 */
bool os_event_post_from_isr(const os_event_t *event);

/**
 * Poll the oldest event from the queue.
 *
 * @param event Pointer to destination event structure.
 * @return true if an event was popped, false if queue is empty.
 */
bool os_event_poll(os_event_t *event);

/**
 * Retrieve number of pending events currently queued.
 */
uint8_t os_event_pending_count(void);

/**
 * Dispatch system-level event to corresponding subsystems (incoming calls,
 * SMS notifications, SD card hotplug, battery status).
 *
 * @param event Pointer to the event to dispatch.
 */
void os_dispatch_system_event(const os_event_t *event);

#ifdef __cplusplus
}
#endif

#endif /* SDK_INCLUDE_VEEBHA_EVENT_H */
