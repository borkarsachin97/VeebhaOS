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

#ifndef FONTS_VEEBHA_FONT_LATIN_CYRILLIC_14_H
#define FONTS_VEEBHA_FONT_LATIN_CYRILLIC_14_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * 1-Bit Monochrome Latin & Cyrillic 14px Font Descriptor
 * Covers:
 *  - ASCII 0x0020 - 0x007E (Basic Latin)
 *  - Cyrillic 0x0401, 0x0451 (Ё, ё)
 *  - Cyrillic 0x0410 - 0x044F (А-я)
 */
extern const lv_font_t veebha_font_latin_cyrillic_14;

#ifdef __cplusplus
}
#endif

#endif /* FONTS_VEEBHA_FONT_LATIN_CYRILLIC_14_H */
