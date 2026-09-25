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

#ifndef APPS_TOOLS_APP_TOOLS_H
#define APPS_TOOLS_APP_TOOLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

void app_tools_init(void);
void app_tools_open(void);
void app_tools_invalidate(void);
void app_tools_open_torch(void);

#ifdef __cplusplus
}
#endif

#endif /* APPS_TOOLS_APP_TOOLS_H */
