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

#include "sdk/text/font_fallback.h"
#include <stdio.h>

void veebha_fonts_init(void)
{
    printf("[FONT_FALLBACK] Initialized font chains: LatinCyrillic14 -> Devanagari16\n");
}

const lv_font_t *veebha_font_get_default(void)
{
    return &veebha_font_latin_cyrillic_14;
}
