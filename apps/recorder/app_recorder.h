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

#ifndef APPS_RECORDER_APP_RECORDER_H
#define APPS_RECORDER_APP_RECORDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdbool.h>

typedef enum {
    CAPTURE_MODE_PHOTO = 0,
    CAPTURE_MODE_VIDEO,
    CAPTURE_MODE_VOICE,
    CAPTURE_MODE_COUNT
} capture_mode_t;

void app_recorder_init(void);
void app_recorder_open(void);
void app_recorder_set_mode(capture_mode_t mode);
capture_mode_t app_recorder_get_mode(void);
bool app_recorder_is_active(void);

#ifdef __cplusplus
}
#endif

#endif /* APPS_RECORDER_APP_RECORDER_H */
