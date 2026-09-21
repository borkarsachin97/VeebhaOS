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

#ifndef SDK_INCLUDE_VEEBHA_T9_H
#define SDK_INCLUDE_VEEBHA_T9_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * T9 / Multi-tap Input Modes
 */
typedef enum {
    T9_MODE_LOWER = 0,    /* "abc" - all lowercase */
    T9_MODE_SENTENCE,     /* "Abc" - initial capital, followed by lower */
    T9_MODE_UPPER,        /* "ABC" - all uppercase */
    T9_MODE_NUMBER,       /* "123" - direct numeric input */
    T9_MODE_COUNT
} t9_input_mode_t;

/**
 * Callback invoked when a character is previewed or updated.
 *
 * @param c          The character to display.
 * @param is_replace If true, replace previous preview character in textarea;
 *                   If false, append new character at cursor position.
 */
typedef void (*t9_char_update_cb_t)(char c, bool is_replace);

/**
 * Callback invoked when the active input mode changes.
 *
 * @param mode The new input mode.
 */
typedef void (*t9_mode_change_cb_t)(t9_input_mode_t mode);

/**
 * Initialize or re-bind the T9 Multi-tap engine with output callbacks.
 *
 * @param char_cb Callback for character additions / updates.
 * @param mode_cb Callback for input mode changes.
 */
void t9_engine_init(t9_char_update_cb_t char_cb, t9_mode_change_cb_t mode_cb);

/**
 * Reset the T9 engine state (clears pending key, cancels timeout timer).
 */
void t9_engine_reset(void);

/**
 * Handle a keypad character or keycode ('0'-'9', '#', '*').
 *
 * @param key_code Character key code.
 * @return true if handled by T9, false otherwise.
 */
bool t9_engine_handle_key(uint32_t key_code);

/**
 * Explicitly commit any pending character currently in multi-tap cycling.
 */
void t9_engine_commit(void);

/**
 * Check if a multi-tap character is currently pending commit.
 */
bool t9_engine_has_pending(void);

/**
 * Cancel and discard any pending multi-tap preview without committing.
 */
void t9_engine_cancel_pending(void);

/**
 * Get current input mode.
 */
t9_input_mode_t t9_engine_get_mode(void);

/**
 * Set current input mode explicitly.
 */
void t9_engine_set_mode(t9_input_mode_t mode);

/**
 * Cycle to the next input mode (Abc -> ABC -> 123 -> abc -> Abc).
 */
void t9_engine_cycle_mode(void);

/**
 * Get display string for an input mode (e.g. "Abc", "abc", "ABC", "123").
 */
const char * t9_engine_get_mode_str(t9_input_mode_t mode);

/**
 * Inform engine that a character was deleted (e.g. backspace) to update sentence capitalization state.
 *
 * @param buffer_empty true if the text buffer is now empty.
 */
void t9_engine_notify_char_deleted(bool buffer_empty);

#ifdef __cplusplus
}
#endif

#endif /* SDK_INCLUDE_VEEBHA_T9_H */
