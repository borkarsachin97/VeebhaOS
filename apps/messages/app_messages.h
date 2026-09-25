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

#ifndef APPS_MESSAGES_APP_MESSAGES_H
#define APPS_MESSAGES_APP_MESSAGES_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open the SMS Messages Inbox application.
 */
void app_messages_open(void);

/**
 * Open the SMS Composer directly for a specific recipient.
 *
 * @param recipient Optional recipient name/number (or NULL for blank).
 */
void app_messages_compose_to(const char *recipient);

#ifdef __cplusplus
}
#endif

#endif /* APPS_MESSAGES_APP_MESSAGES_H */
