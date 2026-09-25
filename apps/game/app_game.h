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

#ifndef APPS_GAME_APP_GAME_H
#define APPS_GAME_APP_GAME_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

void app_game_init(void);
void app_game_open(void);
bool app_game_is_active(void);

#ifdef __cplusplus
}
#endif

#endif /* APPS_GAME_APP_GAME_H */
