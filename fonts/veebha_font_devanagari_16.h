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

#ifndef FONTS_VEEBHA_FONT_DEVANAGARI_16_H
#define FONTS_VEEBHA_FONT_DEVANAGARI_16_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * 1-Bit Monochrome Devanagari 16px Font Descriptor
 * 3-Tier Vertical Metric Budget:
 *  - Upper Tier (Pixels 0-3): Upper matras (e, ai, anusvara, candrabindu)
 *  - Shirorekha Baseline (Pixel 4): 1-pixel continuous horizontal top line
 *  - Middle Tier (Pixels 5-12): 8-pixel envelope for base consonants
 *  - Lower Tier (Pixels 13-15): Bottom matras (u, uu, ri, halant)
 */
extern const lv_font_t veebha_font_devanagari_16;

#ifdef __cplusplus
}
#endif

#endif /* FONTS_VEEBHA_FONT_DEVANAGARI_16_H */
