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

#ifndef SDK_TEXT_FONT_FALLBACK_H
#define SDK_TEXT_FONT_FALLBACK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "fonts/veebha_font_devanagari_16.h"
#include "fonts/veebha_font_latin_cyrillic_14.h"

/**
 * Initialize font fallback chains across the system.
 * Links default and 1-bit typography font descriptors:
 *   veebha_font_latin_cyrillic_14 -> veebha_font_devanagari_16
 *   lv_font_montserrat_14 -> veebha_font_latin_cyrillic_14 -> veebha_font_devanagari_16
 */
void veebha_fonts_init(void);

/**
 * Get the system default multilingual font descriptor with chained fallbacks.
 *
 * @return Pointer to primary font descriptor.
 */
const lv_font_t *veebha_font_get_default(void);

#ifdef __cplusplus
}
#endif

#endif /* SDK_TEXT_FONT_FALLBACK_H */
