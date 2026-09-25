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

#ifndef SDK_INCLUDE_VEEBHA_THEME_H
#define SDK_INCLUDE_VEEBHA_THEME_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * Operating System Theme Palette Identifiers
 */
typedef enum {
    THEME_DARK_CYAN = 0,     /* Default Slate: #121212 bg, #1E1E1E cards, Cyan #00E5FF */
    THEME_OLED_BLACK,        /* Pure OLED Black: #000000 bg, #0D0D0D cards, Electric Blue #00C2FF */
    THEME_LIGHT_CHALK,       /* Clean Light: #F1F5F9 bg, #FFFFFF cards, Slate Blue #007ACC */
    THEME_HIGH_CONTRAST_BW,  /* High-Contrast Monochrome: #000000 bg, #000000 cards, Stark White #FFFFFF */
    THEME_COUNT
} os_theme_id_t;

typedef struct {
    lv_color_t bg_color;
    lv_color_t card_color;
    lv_color_t text_primary;
    lv_color_t text_muted;
    lv_color_t accent;
    bool       is_light;
} os_theme_tokens_t;

/**
 * Initialize theme manager with default or persisted theme.
 */
void theme_init(void);

/**
 * Set active theme palette by identifier and apply across all active UI trees.
 *
 * @param theme_id Selected theme identifier.
 */
void theme_set_palette(os_theme_id_t theme_id);

/**
 * Retrieve current active theme palette identifier.
 */
os_theme_id_t theme_get_palette(void);

/**
 * Retrieve human-readable name for a given theme palette.
 */
const char * theme_get_name(os_theme_id_t theme_id);

/**
 * Backward-compatible helper to switch between Dark (THEME_DARK_CYAN) and Light (THEME_LIGHT_CHALK).
 */
void theme_set_mode(bool is_light_mode);

/**
 * Check if current active theme is Light Mode.
 */
bool theme_is_light_mode(void);

/**
 * Get current theme color tokens.
 */
const os_theme_tokens_t * theme_get(void);

/**
 * Apply theme tokens to a specific screen and its hierarchy.
 */
void theme_apply_to_screen(lv_obj_t *screen, const os_theme_tokens_t *tokens);

/**
 * Recursively apply theme tokens to an object tree.
 */
void theme_apply_to_obj(lv_obj_t *obj, const os_theme_tokens_t *tokens);

#ifdef __cplusplus
}
#endif

#endif /* SDK_INCLUDE_VEEBHA_THEME_H */
