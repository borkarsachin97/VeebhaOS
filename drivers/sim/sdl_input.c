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

#include "drivers/hal_input.h"
#include "boards/simulator/sim_keyboard_map.h"
#include "veebha_softkeys.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

#define EVENT_QUEUE_SIZE 32

static lv_indev_t *s_indev = NULL;
static veebha_key_callback_t s_user_cb = NULL;

/* Key state tracking for long-press calculation */
static struct {
    veebha_key_t key;
    uint32_t press_start_ms;
    bool is_pressed;
    bool long_press_fired;
} s_key_state[VEEBHA_KEY_MAX];

/* Current key and state for LVGL indev read_cb */
static uint32_t s_last_lv_key = 0;
static lv_indev_state_t s_last_lv_state = LV_INDEV_STATE_RELEASED;

static uint32_t veebha_key_to_lv_key(veebha_key_t key)
{
    switch (key) {
    case VEEBHA_KEY_UP:
        return LV_KEY_UP;
    case VEEBHA_KEY_DOWN:
        return LV_KEY_DOWN;
    case VEEBHA_KEY_LEFT:
        return LV_KEY_LEFT;
    case VEEBHA_KEY_RIGHT:
        return LV_KEY_RIGHT;
    case VEEBHA_KEY_OK:
        return LV_KEY_ENTER;
    case VEEBHA_KEY_RSK:
        return LV_KEY_ESC;
    case VEEBHA_KEY_LSK:
        return LV_KEY_HOME;
    case VEEBHA_KEY_NUM_0:
        return '0';
    case VEEBHA_KEY_NUM_1:
        return '1';
    case VEEBHA_KEY_NUM_2:
        return '2';
    case VEEBHA_KEY_NUM_3:
        return '3';
    case VEEBHA_KEY_NUM_4:
        return '4';
    case VEEBHA_KEY_NUM_5:
        return '5';
    case VEEBHA_KEY_NUM_6:
        return '6';
    case VEEBHA_KEY_NUM_7:
        return '7';
    case VEEBHA_KEY_NUM_8:
        return '8';
    case VEEBHA_KEY_NUM_9:
        return '9';
    case VEEBHA_KEY_STAR:
        return '*';
    case VEEBHA_KEY_HASH:
        return '#';
    default:
        return 0;
    }
}

static void sdl_keypad_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    data->key = s_last_lv_key;
    data->state = s_last_lv_state;
}

void hal_input_set_callback(veebha_key_callback_t cb)
{
    s_user_cb = cb;
}

static void dispatch_key_event(veebha_key_t key, veebha_key_state_t state, veebha_press_type_t press_type, uint32_t ts)
{
    veebha_key_event_t ev = {
        .key = key,
        .state = state,
        .press_type = press_type,
        .timestamp_ms = ts
    };

    if (s_user_cb) {
        s_user_cb(&ev);
    }
}

void hal_input_push_event(veebha_key_t key, veebha_key_state_t state)
{
    if (key <= VEEBHA_KEY_NONE || key >= VEEBHA_KEY_MAX) return;

    uint32_t now = SDL_GetTicks();
    uint32_t lv_key = veebha_key_to_lv_key(key);

    if (state == VEEBHA_KEY_STATE_PRESSED) {
        if (!s_key_state[key].is_pressed) {
            s_key_state[key].is_pressed = true;
            s_key_state[key].press_start_ms = now;
            s_key_state[key].long_press_fired = false;

            if (lv_key != 0) {
                s_last_lv_key = lv_key;
                s_last_lv_state = LV_INDEV_STATE_PRESSED;
            }

            dispatch_key_event(key, VEEBHA_KEY_STATE_PRESSED, VEEBHA_PRESS_SHORT, now);

            if (key == VEEBHA_KEY_LSK) {
                softkey_trigger_lsk();
            } else if (key == VEEBHA_KEY_RSK) {
                softkey_trigger_rsk();
            }
        }
    } else {
        if (s_key_state[key].is_pressed) {
            uint32_t duration = now - s_key_state[key].press_start_ms;
            veebha_press_type_t ptype = (duration >= VEEBHA_LONG_PRESS_MS || s_key_state[key].long_press_fired)
                                        ? VEEBHA_PRESS_LONG
                                        : VEEBHA_PRESS_SHORT;

            s_key_state[key].is_pressed = false;

            if (lv_key != 0 && s_last_lv_key == lv_key) {
                s_last_lv_state = LV_INDEV_STATE_RELEASED;
            }

            dispatch_key_event(key, VEEBHA_KEY_STATE_RELEASED, ptype, now);
        }
    }
}

bool hal_input_init(void)
{
    memset(s_key_state, 0, sizeof(s_key_state));

    s_indev = lv_indev_create();
    if (!s_indev) {
        fprintf(stderr, "[HAL_INPUT] Failed to create LVGL indev\n");
        return false;
    }

    lv_indev_set_type(s_indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(s_indev, sdl_keypad_read_cb);

    return true;
}

void hal_input_deinit(void)
{
    s_indev = NULL;
    s_user_cb = NULL;
}

bool hal_input_poll(void)
{
    SDL_Event event;
    uint32_t now = SDL_GetTicks();

    /* Check for long-press expiration while keys remain pressed */
    for (int i = 1; i < VEEBHA_KEY_MAX; i++) {
        if (s_key_state[i].is_pressed && !s_key_state[i].long_press_fired) {
            if ((now - s_key_state[i].press_start_ms) >= VEEBHA_LONG_PRESS_MS) {
                s_key_state[i].long_press_fired = true;
                dispatch_key_event((veebha_key_t)i, VEEBHA_KEY_STATE_PRESSED, VEEBHA_PRESS_LONG, now);
            }
        }
    }

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        }

        if (event.type == SDL_KEYDOWN) {
            /* Ignore key repeats generated by SDL */
            if (event.key.repeat != 0) continue;

            veebha_key_t vkey = sim_keyboard_map_sdl_key(event.key.keysym.sym);
            if (vkey != VEEBHA_KEY_NONE) {
                hal_input_push_event(vkey, VEEBHA_KEY_STATE_PRESSED);
            }
        } else if (event.type == SDL_KEYUP) {
            veebha_key_t vkey = sim_keyboard_map_sdl_key(event.key.keysym.sym);
            if (vkey != VEEBHA_KEY_NONE) {
                hal_input_push_event(vkey, VEEBHA_KEY_STATE_RELEASED);
            }
        }
    }

    return true;
}

lv_indev_t * hal_input_get_lv_indev(void)
{
    return s_indev;
}
