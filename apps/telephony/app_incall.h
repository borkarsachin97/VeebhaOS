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

#ifndef APPS_TELEPHONY_APP_INCALL_H
#define APPS_TELEPHONY_APP_INCALL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "apps/common/mock_telephony.h"
#include "drivers/hal_input.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    INCALL_STATE_IDLE = 0,
    INCALL_STATE_CALLING,     /* Ringing / dialing */
    INCALL_STATE_CONNECTED,   /* In active conversation */
    INCALL_STATE_ENDED        /* Call terminated */
} incall_state_t;

/**
 * Initialize In-Call Telephony module.
 */
void app_incall_init(void);

/**
 * Start an outgoing or incoming call session and open the In-Call screen.
 *
 * @param name   Caller / contact name (or NULL).
 * @param number Phone number string.
 * @param type   CALL_TYPE_OUTGOING or CALL_TYPE_INCOMING.
 */
void app_incall_start(const char *name, const char *number, call_type_t type);

/**
 * Terminate the active call session, display "Call Ended", log call in persistent store,
 * and return to the home screen.
 */
void app_incall_end(void);

/**
 * Check if a call session is currently active (either in foreground or background).
 */
bool app_incall_is_active(void);

/**
 * Check if the In-Call screen is currently the foreground top screen.
 */
bool app_incall_is_foreground(void);

/**
 * Retrieve total connected call duration in seconds.
 */
uint32_t app_incall_get_duration_sec(void);

/**
 * Retrieve formatted call duration string (e.g. "02:45" or "Calling...").
 */
const char * app_incall_get_duration_str(void);

/**
 * Retrieve active contact / caller name string.
 */
const char * app_incall_get_name(void);

/**
 * Retrieve active contact / caller number string.
 */
const char * app_incall_get_number(void);

/**
 * Re-open / bring the active In-Call screen to the foreground.
 */
void app_incall_show(void);

/**
 * Handle keypad keypress when In-Call screen is active (e.g. DTMF digits, speaker, mute).
 *
 * @param key Keypad key identifier.
 */
void app_incall_handle_key(veebha_key_t key);

/**
 * Toggle microphone mute state.
 */
void app_incall_toggle_mute(void);

/**
 * Toggle hands-free speakerphone mode.
 */
void app_incall_toggle_speaker(void);

#ifdef __cplusplus
}
#endif

#endif /* APPS_TELEPHONY_APP_INCALL_H */
