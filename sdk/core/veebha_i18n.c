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

#include "sdk/include/veebha_i18n.h"
#include "sdk/text/indic_shaper.h"
#include <stdio.h>
#include <string.h>

extern const char * const g_veebha_strings_en[STR_KEY_COUNT];
extern const char * const g_veebha_strings_hi[STR_KEY_COUNT];
extern const char * const g_veebha_strings_ru[STR_KEY_COUNT];

static language_id_t s_current_language = LANG_EN;

static const char * const * const s_catalogs[LANG_COUNT] = {
    [LANG_EN] = g_veebha_strings_en,
    [LANG_HI] = g_veebha_strings_hi,
    [LANG_RU] = g_veebha_strings_ru,
};

static const char * const s_lang_names[LANG_COUNT] = {
    [LANG_EN] = "English",
    [LANG_HI] = "हिन्दी",
    [LANG_RU] = "Русский",
};

void veebha_i18n_init(language_id_t initial_lang)
{
    if (initial_lang >= LANG_COUNT) {
        s_current_language = LANG_EN;
    } else {
        s_current_language = initial_lang;
    }
    printf("[I18N] Initialized system language: %s (ID: %d)\n",
           s_lang_names[s_current_language], s_current_language);
}

language_id_t veebha_i18n_get_language(void)
{
    return s_current_language;
}

void veebha_i18n_set_language(language_id_t lang)
{
    if (lang >= LANG_COUNT) return;
    s_current_language = lang;
    printf("[I18N] Switched system language to: %s (ID: %d)\n",
           s_lang_names[s_current_language], s_current_language);
}

const char *veebha_i18n_get_language_name(language_id_t lang)
{
    if (lang >= LANG_COUNT) return "Unknown";
    return s_lang_names[lang];
}

const char *veebha_i18n_str(string_key_t key)
{
    if (key >= STR_KEY_COUNT) return "";
    const char *str = s_catalogs[s_current_language][key];
    if (!str || str[0] == '\0') {
        /* Fallback to English */
        str = s_catalogs[LANG_EN][key];
    }
    return str ? str : "";
}

size_t veebha_i18n_format_shaped(string_key_t key, char *out_buf, size_t max_len)
{
    const char *str = veebha_i18n_str(key);
    if (!str || !out_buf || max_len == 0) return 0;

    if (s_current_language == LANG_HI || indic_shaper_is_devanagari(str)) {
        return indic_shaper_process(str, out_buf, max_len);
    }

    size_t len = strlen(str);
    if (len >= max_len) len = max_len - 1;
    memcpy(out_buf, str, len);
    out_buf[len] = '\0';
    return len;
}
