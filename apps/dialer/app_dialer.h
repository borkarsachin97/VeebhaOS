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

#ifndef APPS_DIALER_APP_DIALER_H
#define APPS_DIALER_APP_DIALER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * Open the Phone Dialer screen.
 *
 * @param initial_digits Optional string of digits to pre-fill (or NULL for empty dialer).
 */
void app_dialer_open(const char *initial_digits);

/**
 * Handle a dialed digit (0-9, *, #) input from keypad.
 *
 * @param digit Character representing the dialed key.
 */
void app_dialer_handle_digit(char digit);

/**
 * Erase the last entered digit (Backspace).
 * If buffer becomes empty, updates RSK label to "Back".
 */
void app_dialer_backspace(void);

/**
 * Erase all entered digits (triggered on RSK long-press).
 */
void app_dialer_clear_all(void);

/**
 * Initiate an outgoing call to the current dialed number.
 */
void app_dialer_start_call(void);

/**
 * Initiate an outgoing call directly to a specified contact name and number.
 *
 * @param name   Contact name (or NULL).
 * @param number Phone number string.
 */
void app_dialer_start_call_to(const char *name, const char *number);

/**
 * Terminate the active outgoing call and return to previous screen.
 */
void app_dialer_end_call(void);

/**
 * Check if the dialer input screen is currently the active top screen.
 */
bool app_dialer_is_active(void);

/**
 * Check if an active outgoing call screen is displayed.
 */
bool app_dialer_is_in_call(void);

/**
 * Retrieve the current dialer digit buffer string.
 */
const char * app_dialer_get_digits(void);

#ifdef __cplusplus
}
#endif

#endif /* APPS_DIALER_APP_DIALER_H */
