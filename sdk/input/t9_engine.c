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

#include "veebha_t9.h"
#include "lvgl.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#define T9_COMMIT_TIMEOUT_MS 750

/* Multi-tap mappings */
static const char * const s_key_map[10] = {
    [0] = " 0",
    [1] = ".,!?'@1",
    [2] = "abc2",
    [3] = "def3",
    [4] = "ghi4",
    [5] = "jkl5",
    [6] = "mno6",
    [7] = "pqrs7",
    [8] = "tuv8",
    [9] = "wxyz9"
};

static const char * const s_mode_names[T9_MODE_COUNT] = {
    [T9_MODE_SENTENCE] = "Abc",
    [T9_MODE_LOWER]    = "abc",
    [T9_MODE_UPPER]    = "ABC",
    [T9_MODE_NUMBER]   = "123"
};

typedef struct {
    t9_input_mode_t      mode;
    t9_char_update_cb_t  char_cb;
    t9_mode_change_cb_t  mode_cb;
    lv_timer_t          *commit_timer;

    /* Multi-tap cycle state */
    bool                 has_pending;
    uint32_t             pending_key;    /* '0' - '9' */
    uint8_t              cycle_index;
    char                 pending_char;

    /* Sentence mode tracking */
    bool                 sentence_start;
} t9_engine_ctx_t;

static t9_engine_ctx_t s_ctx;

static void on_commit_timer(lv_timer_t *timer)
{
    (void)timer;
    t9_engine_commit();
}

void t9_engine_init(t9_char_update_cb_t char_cb, t9_mode_change_cb_t mode_cb)
{
    s_ctx.mode = T9_MODE_SENTENCE;
    s_ctx.char_cb = char_cb;
    s_ctx.mode_cb = mode_cb;
    s_ctx.has_pending = false;
    s_ctx.pending_key = 0;
    s_ctx.cycle_index = 0;
    s_ctx.pending_char = '\0';
    s_ctx.sentence_start = true;

    if (!s_ctx.commit_timer) {
        s_ctx.commit_timer = lv_timer_create(on_commit_timer, T9_COMMIT_TIMEOUT_MS, NULL);
        lv_timer_pause(s_ctx.commit_timer);
    } else {
        lv_timer_pause(s_ctx.commit_timer);
    }

    if (s_ctx.mode_cb) {
        s_ctx.mode_cb(s_ctx.mode);
    }
}

void t9_engine_reset(void)
{
    if (s_ctx.commit_timer) {
        lv_timer_pause(s_ctx.commit_timer);
    }
    s_ctx.has_pending = false;
    s_ctx.pending_key = 0;
    s_ctx.cycle_index = 0;
    s_ctx.pending_char = '\0';
    s_ctx.sentence_start = true;
}

t9_input_mode_t t9_engine_get_mode(void)
{
    return s_ctx.mode;
}

void t9_engine_set_mode(t9_input_mode_t mode)
{
    if (s_ctx.has_pending) {
        t9_engine_commit();
    }
    s_ctx.mode = (mode < T9_MODE_COUNT) ? mode : T9_MODE_SENTENCE;
    if (s_ctx.mode_cb) {
        s_ctx.mode_cb(s_ctx.mode);
    }
}

void t9_engine_cycle_mode(void)
{
    if (s_ctx.has_pending) {
        t9_engine_commit();
    }
    s_ctx.mode = (t9_input_mode_t)((s_ctx.mode + 1) % T9_MODE_COUNT);
    if (s_ctx.mode_cb) {
        s_ctx.mode_cb(s_ctx.mode);
    }
}

const char * t9_engine_get_mode_str(t9_input_mode_t mode)
{
    if (mode < T9_MODE_COUNT) {
        return s_mode_names[mode];
    }
    return "Abc";
}

bool t9_engine_has_pending(void)
{
    return s_ctx.has_pending;
}

void t9_engine_cancel_pending(void)
{
    if (s_ctx.commit_timer) {
        lv_timer_pause(s_ctx.commit_timer);
    }
    s_ctx.has_pending = false;
    s_ctx.pending_key = 0;
    s_ctx.cycle_index = 0;
    s_ctx.pending_char = '\0';
}

void t9_engine_commit(void)
{
    if (!s_ctx.has_pending) return;

    if (s_ctx.commit_timer) {
        lv_timer_pause(s_ctx.commit_timer);
    }

    /* Update sentence capitalization based on committed character */
    if (s_ctx.mode == T9_MODE_SENTENCE) {
        if (s_ctx.pending_char == '.' || s_ctx.pending_char == '!' || s_ctx.pending_char == '?') {
            s_ctx.sentence_start = true;
        } else if (isalpha((unsigned char)s_ctx.pending_char)) {
            s_ctx.sentence_start = false;
        }
    }

    s_ctx.has_pending = false;
    s_ctx.pending_key = 0;
    s_ctx.cycle_index = 0;
    s_ctx.pending_char = '\0';
}

void t9_engine_notify_char_deleted(bool buffer_empty)
{
    if (buffer_empty) {
        s_ctx.sentence_start = true;
    }
}

static char transform_char_for_mode(char raw_char)
{
    if (!isalpha((unsigned char)raw_char)) {
        return raw_char;
    }

    switch (s_ctx.mode) {
    case T9_MODE_UPPER:
        return (char)toupper((unsigned char)raw_char);
    case T9_MODE_LOWER:
        return (char)tolower((unsigned char)raw_char);
    case T9_MODE_SENTENCE:
        if (s_ctx.sentence_start) {
            return (char)toupper((unsigned char)raw_char);
        } else {
            return (char)tolower((unsigned char)raw_char);
        }
    default:
        return raw_char;
    }
}

bool t9_engine_handle_key(uint32_t key_code)
{
    /* Handle mode cycle key (#) */
    if (key_code == '#') {
        t9_engine_cycle_mode();
        return true;
    }

    /* Numeric direct input mode */
    if (s_ctx.mode == T9_MODE_NUMBER) {
        if (key_code >= '0' && key_code <= '9') {
            if (s_ctx.char_cb) {
                s_ctx.char_cb((char)key_code, false);
            }
            return true;
        }
        return false;
    }

    /* Multi-tap keys: '0' to '9' */
    if (key_code < '0' || key_code > '9') {
        return false;
    }

    uint8_t digit = (uint8_t)(key_code - '0');
    const char *chars = s_key_map[digit];
    if (!chars || chars[0] == '\0') {
        return false;
    }

    size_t char_count = strlen(chars);

    if (s_ctx.has_pending && s_ctx.pending_key == key_code) {
        /* Successive press on SAME key within timeout -> advance cycle */
        s_ctx.cycle_index = (uint8_t)((s_ctx.cycle_index + 1) % char_count);
        char c = transform_char_for_mode(chars[s_ctx.cycle_index]);
        s_ctx.pending_char = c;

        if (s_ctx.commit_timer) {
            lv_timer_reset(s_ctx.commit_timer);
            lv_timer_resume(s_ctx.commit_timer);
        }

        if (s_ctx.char_cb) {
            s_ctx.char_cb(c, true); /* Replace previous preview */
        }
        return true;
    }

    /* Different key or no key pending: commit any existing pending character */
    if (s_ctx.has_pending) {
        t9_engine_commit();
    }

    /* Start new multi-tap cycle */
    s_ctx.has_pending = true;
    s_ctx.pending_key = key_code;
    s_ctx.cycle_index = 0;

    char c = transform_char_for_mode(chars[0]);
    s_ctx.pending_char = c;

    if (s_ctx.commit_timer) {
        lv_timer_reset(s_ctx.commit_timer);
        lv_timer_resume(s_ctx.commit_timer);
    }

    if (s_ctx.char_cb) {
        s_ctx.char_cb(c, false); /* Append new preview character */
    }

    return true;
}
