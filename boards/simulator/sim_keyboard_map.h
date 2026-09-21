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

#ifndef BOARDS_SIMULATOR_SIM_KEYBOARD_MAP_H
#define BOARDS_SIMULATOR_SIM_KEYBOARD_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <SDL2/SDL.h>
#include "drivers/hal_input.h"

/**
 * Translates an SDL keycode into a VeebhaOS feature phone key code.
 *
 * @param keycode SDL keycode from SDL_KeyboardEvent.
 * @return Mapped veebha_key_t, or VEEBHA_KEY_NONE if unmapped.
 */
veebha_key_t sim_keyboard_map_sdl_key(SDL_Keycode keycode);

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_SIMULATOR_SIM_KEYBOARD_MAP_H */
