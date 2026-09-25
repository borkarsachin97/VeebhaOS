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

#ifndef SDK_INCLUDE_VEEBHA_I18N_H
#define SDK_INCLUDE_VEEBHA_I18N_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * Supported System Languages
 */
typedef enum {
    LANG_EN = 0,  /* English */
    LANG_HI = 1,  /* Hindi (हिन्दी) */
    LANG_RU = 2,  /* Russian (Русский) */
    LANG_COUNT
} language_id_t;

/**
 * String Resource Identifiers
 */
typedef enum {
    STR_APP_NAME = 0,
    STR_MENU,
    STR_BACK,
    STR_SELECT,
    STR_OPTIONS,
    STR_SETTINGS,
    STR_MESSAGES,
    STR_CONTACTS,
    STR_CALL_LOGS,
    STR_CALCULATOR,
    STR_CALENDAR,
    STR_TOOLS,
    STR_GALLERY,
    STR_MUSIC,
    STR_SNAKE,
    STR_ALARM,
    STR_TIMER,
    STR_STOPWATCH,
    STR_TEXT_VIEWER,
    STR_LANGUAGE,
    STR_THEME,
    STR_WALLPAPER,
    STR_AUDIO_PROFILES,
    STR_CONNECTIVITY,
    STR_DISPLAY,
    STR_DATE_TIME,
    STR_STORAGE,
    STR_ABOUT,
    STR_LANG_ENGLISH,
    STR_LANG_HINDI,
    STR_LANG_RUSSIAN,
    STR_SAVE,
    STR_CANCEL,
    STR_OK,
    STR_DELETE,
    STR_NEW_MESSAGE,
    STR_INBOX,
    STR_SENT,
    STR_CALL,
    STR_PHONE,
    STR_END_CALL,
    STR_TASK_SWITCHER,
    STR_BROWSER,
    STR_ENTER_URL,
    STR_BOOKMARKS,
    STR_NO_CONNECTION,
    STR_TETHERING,
    STR_RELOAD,
    STR_HISTORY,
    STR_OFFLINE_SAVED,
    STR_TETRIS,
    STR_BRICK_BREAKER,
    STR_KEY_COUNT
} string_key_t;

/**
 * Initialize the system localization engine.
 *
 * @param initial_lang Language ID loaded from NVRAM or default.
 */
void veebha_i18n_init(language_id_t initial_lang);

/**
 * Get the currently active system language.
 *
 * @return Current language ID.
 */
language_id_t veebha_i18n_get_language(void);

/**
 * Set the system language and trigger UI re-render notifications.
 *
 * @param lang Target language ID.
 */
void veebha_i18n_set_language(language_id_t lang);

/**
 * Get the localized display name for a language (e.g. "English", "हिन्दी", "Русский").
 *
 * @param lang Language ID.
 * @return Language name string.
 */
const char *veebha_i18n_get_language_name(language_id_t lang);

/**
 * Retrieve a localized string by key in the currently active system language.
 *
 * @param key String resource key.
 * @return Localized string in UTF-8.
 */
const char *veebha_i18n_str(string_key_t key);

/**
 * Retrieve and shape a localized string (e.g. Indic matra transposition).
 * Safe for zero-heap stack buffer writing.
 *
 * @param key String resource key.
 * @param out_buf Destination buffer.
 * @param max_len Buffer capacity including null terminator.
 * @return Number of bytes written.
 */
size_t veebha_i18n_format_shaped(string_key_t key, char *out_buf, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* SDK_INCLUDE_VEEBHA_I18N_H */
