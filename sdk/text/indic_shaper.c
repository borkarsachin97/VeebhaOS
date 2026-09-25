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

#include "sdk/text/indic_shaper.h"
#include <string.h>
#include <stdio.h>

#define DEVANAGARI_SHORT_I 0x093F
#define DEVANAGARI_VIRAMA  0x094D
#define DEVANAGARI_NUKTA   0x093C
#define DEVANAGARI_RA      0x0930

static uint32_t utf8_decode_char(const char *s, size_t *byte_len)
{
    if (!s || *s == '\0') {
        if (byte_len) *byte_len = 0;
        return 0;
    }

    const unsigned char *p = (const unsigned char *)s;
    if (p[0] < 0x80) {
        if (byte_len) *byte_len = 1;
        return p[0];
    }
    if ((p[0] & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80) {
        if (byte_len) *byte_len = 2;
        return ((uint32_t)(p[0] & 0x1F) << 6) | (p[1] & 0x3F);
    }
    if ((p[0] & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
        if (byte_len) *byte_len = 3;
        return ((uint32_t)(p[0] & 0x0F) << 12) | ((uint32_t)(p[1] & 0x3F) << 6) | (p[2] & 0x3F);
    }
    if ((p[0] & 0xF8) == 0xF0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80 && (p[3] & 0xC0) == 0x80) {
        if (byte_len) *byte_len = 4;
        return ((uint32_t)(p[0] & 0x07) << 18) | ((uint32_t)(p[1] & 0x3F) << 12) | ((uint32_t)(p[2] & 0x3F) << 6) | (p[3] & 0x3F);
    }

    if (byte_len) *byte_len = 1;
    return p[0];
}

static size_t utf8_encode_char(uint32_t cp, char *out)
{
    if (cp <= 0x7F) {
        out[0] = (char)cp;
        return 1;
    }
    if (cp <= 0x7FF) {
        out[0] = (char)(0xC0 | ((cp >> 6) & 0x1F));
        out[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp <= 0xFFFF) {
        out[0] = (char)(0xE0 | ((cp >> 12) & 0x0F));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    }
    out[0] = (char)(0xF0 | ((cp >> 18) & 0x07));
    out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[3] = (char)(0x80 | (cp & 0x3F));
    return 4;
}

bool indic_shaper_is_devanagari(const char *utf8_str)
{
    if (!utf8_str) return false;

    const char *p = utf8_str;
    while (*p) {
        size_t blen = 0;
        uint32_t cp = utf8_decode_char(p, &blen);
        if (blen == 0) break;
        if (cp >= 0x0900 && cp <= 0x097F) {
            return true;
        }
        p += blen;
    }
    return false;
}

static bool is_devanagari_consonant(uint32_t cp)
{
    return (cp >= 0x0915 && cp <= 0x0939) || (cp >= 0x0958 && cp <= 0x095F);
}

typedef struct {
    uint32_t c1;
    uint32_t c2;
    const char *replacement;
} conjunct_rule_t;

static const conjunct_rule_t s_conjunct_rules[] = {
    { 0x0915, 0x0937, "क्ष" }, /* k + ssa -> ksha */
    { 0x0924, 0x0930, "त्र" }, /* t + r -> tra */
    { 0x091C, 0x091E, "ज्ञ" }, /* j + nya -> jnya */
    { 0x0936, 0x0930, "श्र" }, /* sh + r -> shra */
    { 0x0926, 0x0927, "द्ध" }, /* d + dh -> ddh */
    { 0x0924, 0x0924, "त्त" }, /* t + t -> tta */
    { 0x0926, 0x092F, "द्य" }, /* d + y -> dya */
    { 0x0926, 0x0935, "द्व" }, /* d + v -> dva */
    { 0x0939, 0x092E, "ह्म" }, /* h + m -> hma */
};

#define CONJUNCT_RULE_COUNT (sizeof(s_conjunct_rules) / sizeof(s_conjunct_rules[0]))

size_t indic_shaper_process(const char *in_utf8, char *out_utf8, size_t max_len)
{
    if (!in_utf8 || !out_utf8 || max_len == 0) return 0;

    if (!indic_shaper_is_devanagari(in_utf8)) {
        size_t len = strlen(in_utf8);
        if (len >= max_len) len = max_len - 1;
        memcpy(out_utf8, in_utf8, len);
        out_utf8[len] = '\0';
        return len;
    }

    /* Tokenize into Unicode Codepoint array */
    #define MAX_CODEPOINTS 512
    uint32_t cps[MAX_CODEPOINTS];
    size_t cp_count = 0;

    const char *p = in_utf8;
    while (*p && cp_count < MAX_CODEPOINTS) {
        size_t blen = 0;
        uint32_t cp = utf8_decode_char(p, &blen);
        if (blen == 0) break;
        cps[cp_count++] = cp;
        p += blen;
    }

    /* Pass 1: Short 'i' (0x093F) transposition */
    for (size_t i = 0; i < cp_count; i++) {
        if (cps[i] == DEVANAGARI_SHORT_I) {
            /* Find cluster boundary preceding short 'i' */
            if (i > 0) {
                size_t cluster_start = i - 1;
                while (cluster_start > 0) {
                    if (cluster_start >= 2 && cps[cluster_start - 1] == DEVANAGARI_VIRAMA &&
                        is_devanagari_consonant(cps[cluster_start - 2])) {
                        cluster_start -= 2;
                    } else if (cps[cluster_start] == DEVANAGARI_NUKTA && cluster_start > 0) {
                        cluster_start--;
                    } else {
                        break;
                    }
                }

                /* Transpose short 'i' to cluster_start */
                uint32_t short_i = cps[i];
                for (size_t k = i; k > cluster_start; k--) {
                    cps[k] = cps[k - 1];
                }
                cps[cluster_start] = short_i;
            }
        }
    }

    /* Pass 2: Serialize back to UTF-8 applying conjunct ligature substitutions */
    size_t out_idx = 0;
    size_t i = 0;

    while (i < cp_count && out_idx + 4 < max_len) {
        /* Check for consonant + virama + consonant conjunct */
        bool conjunct_matched = false;
        if (i + 2 < cp_count && is_devanagari_consonant(cps[i]) && cps[i + 1] == DEVANAGARI_VIRAMA && is_devanagari_consonant(cps[i + 2])) {
            for (size_t r = 0; r < CONJUNCT_RULE_COUNT; r++) {
                if (s_conjunct_rules[r].c1 == cps[i] && s_conjunct_rules[r].c2 == cps[i + 2]) {
                    size_t rlen = strlen(s_conjunct_rules[r].replacement);
                    if (out_idx + rlen < max_len) {
                        memcpy(&out_utf8[out_idx], s_conjunct_rules[r].replacement, rlen);
                        out_idx += rlen;
                        i += 3;
                        conjunct_matched = true;
                        break;
                    }
                }
            }
        }

        if (!conjunct_matched) {
            char temp[8];
            size_t elen = utf8_encode_char(cps[i], temp);
            if (out_idx + elen < max_len) {
                memcpy(&out_utf8[out_idx], temp, elen);
                out_idx += elen;
            } else {
                break;
            }
            i++;
        }
    }

    out_utf8[out_idx] = '\0';
    return out_idx;
}
