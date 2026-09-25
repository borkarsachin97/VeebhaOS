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

#ifndef SDK_TEXT_INDIC_SHAPER_H
#define SDK_TEXT_INDIC_SHAPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Check whether a UTF-8 string contains any Devanagari Unicode codepoints (0x0900 - 0x097F).
 *
 * @param utf8_str Input UTF-8 string.
 * @return true if Devanagari characters are present, false otherwise.
 */
bool indic_shaper_is_devanagari(const char *utf8_str);

/**
 * Lightweight Devanagari text shaper pre-processor for resource-constrained feature phones.
 *
 * Implements:
 * 1. Short 'i' Matra (U+093F, '\xE0\xA4\xBF') transposition:
 *    Transposes the short 'i' matra before its preceding base consonant or consonant cluster.
 * 2. Halant (U+094D, '\xE0\xA5\x8D') conjunct ligature substitutions:
 *    Maps common clusters (e.g. ksha, tra, jnya, shra, ddh, tta, dya) to pre-rendered glyphs.
 * 3. Repha (U+0930 + U+094D, 'र्') positioning above the following consonant.
 *
 * Safe for zero dynamic memory allocation (0 bytes heap allocated).
 *
 * @param in_utf8  Source UTF-8 string in logical Unicode order.
 * @param out_utf8 Destination buffer for shaped UTF-8 string.
 * @param max_len  Capacity of out_utf8 buffer in bytes including null terminator.
 * @return Number of bytes written to out_utf8 (excluding null terminator).
 */
size_t indic_shaper_process(const char *in_utf8, char *out_utf8, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* SDK_TEXT_INDIC_SHAPER_H */
