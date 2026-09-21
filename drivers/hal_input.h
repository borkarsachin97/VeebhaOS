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

#ifndef DRIVERS_HAL_INPUT_H
#define DRIVERS_HAL_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"

/**
 * Standard Feature Phone Keypad Codes
 */
typedef enum {
    VEEBHA_KEY_NONE = 0,

    /* Softkeys */
    VEEBHA_KEY_LSK,         /* Left Softkey (Action / Options) */
    VEEBHA_KEY_RSK,         /* Right Softkey (Back / Clear) */

    /* 5-Way D-Pad */
    VEEBHA_KEY_UP,
    VEEBHA_KEY_DOWN,
    VEEBHA_KEY_LEFT,
    VEEBHA_KEY_RIGHT,
    VEEBHA_KEY_OK,          /* Center / OK / Select */

    /* Telephony & Power */
    VEEBHA_KEY_CALL,        /* Call / Dial key */
    VEEBHA_KEY_END,         /* End / Power / Hangup key */

    /* 12-Key Alphanumeric Matrix */
    VEEBHA_KEY_NUM_0,       /* 0 (Space / +) */
    VEEBHA_KEY_NUM_1,       /* 1 (.,!?) */
    VEEBHA_KEY_NUM_2,       /* 2 (abc) */
    VEEBHA_KEY_NUM_3,       /* 3 (def) */
    VEEBHA_KEY_NUM_4,       /* 4 (ghi) */
    VEEBHA_KEY_NUM_5,       /* 5 (jkl) */
    VEEBHA_KEY_NUM_6,       /* 6 (mno) */
    VEEBHA_KEY_NUM_7,       /* 7 (pqrs) */
    VEEBHA_KEY_NUM_8,       /* 8 (tuv) */
    VEEBHA_KEY_NUM_9,       /* 9 (wxyz) */
    VEEBHA_KEY_STAR,        /* * (Special symbols / Lock) */
    VEEBHA_KEY_HASH,        /* # (Multi-tap mode cycling / Silent) */

    VEEBHA_KEY_MAX
} veebha_key_t;

/**
 * Key Action States
 */
typedef enum {
    VEEBHA_KEY_STATE_RELEASED = 0,
    VEEBHA_KEY_STATE_PRESSED  = 1
} veebha_key_state_t;

/**
 * Key Press Classification
 */
typedef enum {
    VEEBHA_PRESS_SHORT = 0,
    VEEBHA_PRESS_LONG  = 1
} veebha_press_type_t;

/**
 * Key Event Packet
 */
typedef struct {
    veebha_key_t        key;
    veebha_key_state_t  state;
    veebha_press_type_t press_type;
    uint32_t            timestamp_ms;
} veebha_key_event_t;

/**
 * Keypad Event Callback Type
 */
typedef void (*veebha_key_callback_t)(const veebha_key_event_t *event);

#define VEEBHA_LONG_PRESS_MS 600

/**
 * Initialize the hardware/simulator input subsystem and LVGL keypad device.
 *
 * @return true on success, false on error.
 */
bool hal_input_init(void);

/**
 * Deinitialize the input subsystem.
 */
void hal_input_deinit(void);

/**
 * Poll input events (for simulator event loops or OS task runners).
 *
 * @return true if events are actively running, false if termination requested.
 */
bool hal_input_poll(void);

/**
 * Register a listener callback for key events.
 */
void hal_input_set_callback(veebha_key_callback_t cb);

/**
 * Enqueue a key event into the system input queue.
 */
void hal_input_push_event(veebha_key_t key, veebha_key_state_t state);

/**
 * Retrieve the active LVGL keypad input device.
 */
lv_indev_t * hal_input_get_lv_indev(void);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_HAL_INPUT_H */
