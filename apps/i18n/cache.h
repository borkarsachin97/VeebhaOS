/*
 * VeebhaOS - i18n string cache helper
 * SPDX-License-Identifier: MIT
 *
 * Provides VEEBHA_I18N_CACHED(key) — a zero-overhead alias that
 * returns the translated string for the given key.  The underlying
 * veebha_i18n_str() already returns a pointer into a static table
 * (no malloc, no I/O), so this macro is purely a readability aid and
 * lets us swap the implementation later without touching every call
 * site.
 */
#ifndef APPS_I18N_CACHE_H
#define APPS_I18N_CACHE_H

#include "sdk/include/veebha_i18n.h"

/* Use VEEBHA_I18N_CACHED(STR_FOO) wherever you would have written
 * veebha_i18n_str(STR_FOO).  It maps 1:1 with zero extra overhead. */
#define VEEBHA_I18N_CACHED(key) veebha_i18n_str(key)

#endif /* APPS_I18N_CACHE_H */
