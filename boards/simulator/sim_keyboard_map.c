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

#include "sim_keyboard_map.h"

veebha_key_t sim_keyboard_map_sdl_key(SDL_Keycode keycode)
{
    switch (keycode) {
    /* 5-Way D-Pad */
    case SDLK_UP:
        return VEEBHA_KEY_UP;
    case SDLK_DOWN:
        return VEEBHA_KEY_DOWN;
    case SDLK_LEFT:
        return VEEBHA_KEY_LEFT;
    case SDLK_RIGHT:
        return VEEBHA_KEY_RIGHT;
    case SDLK_RETURN:
    case SDLK_SPACE:
    case SDLK_KP_ENTER:
        return VEEBHA_KEY_OK;

    /* Softkeys */
    case SDLK_LALT:
    case SDLK_F1:
    case SDLK_LEFTBRACKET:
        return VEEBHA_KEY_LSK;

    case SDLK_RALT:
    case SDLK_F2:
    case SDLK_RIGHTBRACKET:
    case SDLK_BACKSPACE:
    case SDLK_DELETE:
        return VEEBHA_KEY_RSK;

    /* Telephony & Power */
    case SDLK_c:
        return VEEBHA_KEY_CALL;

    case SDLK_e:
    case SDLK_END:
    case SDLK_ESCAPE:
    case SDLK_POWER:
        return VEEBHA_KEY_END;

    /* Numeric Keypad Matrix */
    case SDLK_0:
    case SDLK_KP_0:
        return VEEBHA_KEY_NUM_0;

    case SDLK_1:
    case SDLK_KP_1:
        return VEEBHA_KEY_NUM_1;

    case SDLK_2:
    case SDLK_KP_2:
        return VEEBHA_KEY_NUM_2;

    case SDLK_3:
    case SDLK_KP_3:
        return VEEBHA_KEY_NUM_3;

    case SDLK_4:
    case SDLK_KP_4:
        return VEEBHA_KEY_NUM_4;

    case SDLK_5:
    case SDLK_KP_5:
        return VEEBHA_KEY_NUM_5;

    case SDLK_6:
    case SDLK_KP_6:
        return VEEBHA_KEY_NUM_6;

    case SDLK_7:
    case SDLK_KP_7:
        return VEEBHA_KEY_NUM_7;

    case SDLK_8:
    case SDLK_KP_8:
        return VEEBHA_KEY_NUM_8;

    case SDLK_9:
    case SDLK_KP_9:
        return VEEBHA_KEY_NUM_9;

    case SDLK_ASTERISK:
    case SDLK_KP_MULTIPLY:
        return VEEBHA_KEY_STAR;

    case SDLK_HASH:
        return VEEBHA_KEY_HASH;

    default:
        return VEEBHA_KEY_NONE;
    }
}
