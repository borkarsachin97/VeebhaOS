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

#include "veebha_theme.h"
#include "veebha_win_mgr.h"
#include "veebha_status_bar.h"
#include <stdio.h>

static os_theme_tokens_t s_current_theme;
static os_theme_id_t     s_current_theme_id = THEME_DARK_CYAN;
static bool              s_initialized = false;

static const char *s_theme_names[THEME_COUNT] = {
    "Dark Cyan",
    "OLED Black",
    "Clean Light",
    "High-Contrast B&W"
};

static void theme_update_tokens(os_theme_id_t id)
{
    s_current_theme_id = id;

    switch (id) {
    case THEME_OLED_BLACK:
        s_current_theme.bg_color     = lv_color_hex(0x000000); /* Pure OLED Black */
        s_current_theme.card_color   = lv_color_hex(0x0D0D0D); /* Ultra dark card */
        s_current_theme.text_primary = lv_color_hex(0xFFFFFF); /* White primary text */
        s_current_theme.text_muted   = lv_color_hex(0x666666); /* Dark grey secondary */
        s_current_theme.accent       = lv_color_hex(0x00C2FF); /* Electric blue accent */
        s_current_theme.is_light     = false;
        break;

    case THEME_LIGHT_CHALK:
        s_current_theme.bg_color     = lv_color_hex(0xF1F5F9); /* Clean light background */
        s_current_theme.card_color   = lv_color_hex(0xFFFFFF); /* Crisp white cards */
        s_current_theme.text_primary = lv_color_hex(0x0F172A); /* Dark slate text */
        s_current_theme.text_muted   = lv_color_hex(0x64748B); /* Secondary grey text */
        s_current_theme.accent       = lv_color_hex(0x007ACC); /* Vibrant blue accent */
        s_current_theme.is_light     = true;
        break;

    case THEME_HIGH_CONTRAST_BW:
        s_current_theme.bg_color     = lv_color_hex(0x000000); /* Pure Black */
        s_current_theme.card_color   = lv_color_hex(0x000000); /* Pure Black with 1px White border */
        s_current_theme.text_primary = lv_color_hex(0xFFFFFF); /* Stark White text */
        s_current_theme.text_muted   = lv_color_hex(0xCCCCCC); /* Soft Light Gray secondary */
        s_current_theme.accent       = lv_color_hex(0xFFFFFF); /* High-contrast stark white */
        s_current_theme.is_light     = false;
        break;

    case THEME_DARK_CYAN:
    default:
        s_current_theme_id           = THEME_DARK_CYAN;
        s_current_theme.bg_color     = lv_color_hex(0x121212); /* Deep dark slate background */
        s_current_theme.card_color   = lv_color_hex(0x1E1E1E); /* Dark grey cards */
        s_current_theme.text_primary = lv_color_hex(0xFFFFFF); /* White primary text */
        s_current_theme.text_muted   = lv_color_hex(0x888888); /* Muted secondary text */
        s_current_theme.accent       = lv_color_hex(0x00E5FF); /* High-tech cyan accent */
        s_current_theme.is_light     = false;
        break;
    }
}

void theme_init(void)
{
    if (s_initialized) return;
    theme_update_tokens(THEME_DARK_CYAN);
    s_initialized = true;
    printf("[THEME] Theme engine initialized (Default: %s)\n", theme_get_name(s_current_theme_id));
}

os_theme_id_t theme_get_palette(void)
{
    if (!s_initialized) theme_init();
    return s_current_theme_id;
}

const char * theme_get_name(os_theme_id_t theme_id)
{
    if (theme_id >= THEME_COUNT) theme_id = THEME_DARK_CYAN;
    return s_theme_names[theme_id];
}

void theme_apply_to_screen(lv_obj_t *screen, const os_theme_tokens_t *tokens)
{
    if (!screen || !lv_obj_is_valid(screen) || !tokens) return;

    /* 1. Root Screen Background */
    lv_obj_set_style_bg_color(screen, tokens->bg_color, 0);

    uint32_t top_child_count = lv_obj_get_child_count(screen);

    /* 2. Status Bar (Child 0 if present) */
    if (top_child_count > 0) {
        lv_obj_t *status_bar = lv_obj_get_child(screen, 0);
        if (status_bar && lv_obj_is_valid(status_bar)) {
            lv_color_t sb_bg = (s_current_theme_id == THEME_HIGH_CONTRAST_BW || s_current_theme_id == THEME_OLED_BLACK)
                                ? lv_color_hex(0x000000)
                                : (tokens->is_light ? lv_color_hex(0xDCDCDC) : lv_color_hex(0x181A20));
            lv_obj_set_style_bg_color(status_bar, sb_bg, 0);

            uint32_t sb_cnt = lv_obj_get_child_count(status_bar);
            /* Left tray: signal bars and 4G label */
            if (sb_cnt > 0) {
                lv_obj_t *left_tray = lv_obj_get_child(status_bar, 0);
                if (left_tray && lv_obj_is_valid(left_tray) && lv_obj_get_child_count(left_tray) > 1) {
                    lv_obj_t *net_lbl = lv_obj_get_child(left_tray, 1);
                    if (net_lbl && lv_obj_is_valid(net_lbl)) {
                        lv_obj_set_style_text_color(net_lbl, (s_current_theme_id == THEME_HIGH_CONTRAST_BW) ? lv_color_hex(0xFFFFFF) : tokens->text_muted, 0);
                    }
                }
            }
            /* Zone 2: Center Clock Label */
            if (sb_cnt > 1) {
                lv_obj_t *clock_lbl = lv_obj_get_child(status_bar, 1);
                if (clock_lbl && lv_obj_is_valid(clock_lbl)) {
                    lv_obj_set_style_text_color(clock_lbl, tokens->text_primary, 0);
                }
            }
            /* Zone 3: Right Tray: Bluetooth, Headset, USB, Silent, Battery */
            if (sb_cnt > 2) {
                lv_obj_t *right_tray = lv_obj_get_child(status_bar, 2);
                if (right_tray && lv_obj_is_valid(right_tray)) {
                    uint32_t rt_cnt = lv_obj_get_child_count(right_tray);
                    if (rt_cnt > 1) {
                        lv_obj_t *bt = lv_obj_get_child(right_tray, 1);
                        if (bt && lv_obj_is_valid(bt)) {
                            lv_obj_set_style_text_color(bt, (s_current_theme_id == THEME_HIGH_CONTRAST_BW) ? lv_color_hex(0xFFFFFF) : tokens->accent, 0);
                        }
                    }
                }
            }
        }
    }

    /* 3. Intermediate Children (Sub-Screen Header / Dynamic Banner and Content Container) */
    if (top_child_count > 2) {
        for (uint32_t c_idx = 1; c_idx < top_child_count - 1; c_idx++) {
            lv_obj_t *child = lv_obj_get_child(screen, c_idx);
            if (!child || !lv_obj_is_valid(child)) continue;

            /* Check if slim header/banner (height <= 20) */
            if (lv_obj_get_height(child) <= 20) {
                lv_obj_set_style_bg_color(child, tokens->card_color, 0);
                lv_color_t hdr_border = (s_current_theme_id == THEME_HIGH_CONTRAST_BW)
                                        ? lv_color_hex(0xFFFFFF)
                                        : (tokens->is_light ? lv_color_hex(0xE2E8F0) : lv_color_hex(0x282C35));
                lv_obj_set_style_border_color(child, hdr_border, 0);

                uint32_t h_cnt = lv_obj_get_child_count(child);
                for (uint32_t h = 0; h < h_cnt; h++) {
                    lv_obj_t *hlbl = lv_obj_get_child(child, h);
                    if (hlbl && lv_obj_is_valid(hlbl) && lv_obj_check_type(hlbl, &lv_label_class)) {
                        lv_obj_set_style_text_color(hlbl, tokens->accent, 0);
                    }
                }
            } else {
                /* Content Container */
                lv_obj_set_style_bg_color(child, tokens->bg_color, 0);

                uint32_t item_count = lv_obj_get_child_count(child);
                for (uint32_t i = 0; i < item_count; i++) {
                    lv_obj_t *item = lv_obj_get_child(child, i);
                    if (!item || !lv_obj_is_valid(item)) continue;

                    /* Button (Grid cell or List row) */
                    if (lv_obj_check_type(item, &lv_button_class)) {
                        lv_obj_set_style_bg_color(item, tokens->card_color, 0);
                        lv_color_t item_border = (s_current_theme_id == THEME_HIGH_CONTRAST_BW)
                                                 ? lv_color_hex(0xFFFFFF)
                                                 : (tokens->is_light ? lv_color_hex(0xE2E8F0) : lv_color_hex(0x282C35));
                        lv_obj_set_style_border_color(item, item_border, 0);
                        lv_obj_set_style_border_width(item, 1, 0);
                        lv_obj_set_style_radius(item, 0, 0);
                        lv_obj_set_style_outline_width(item, 0, 0);

                        lv_color_t foc_bg = (s_current_theme_id == THEME_HIGH_CONTRAST_BW)
                                            ? lv_color_hex(0x222222)
                                            : (tokens->is_light ? lv_color_hex(0xD0E8FF) : lv_color_hex(0x1A3555));
                        lv_obj_set_style_bg_color(item, foc_bg, LV_STATE_FOCUSED);
                        lv_obj_set_style_border_color(item, tokens->accent, LV_STATE_FOCUSED);
                        lv_obj_set_style_border_width(item, 2, LV_STATE_FOCUSED);
                        lv_obj_set_style_radius(item, 0, LV_STATE_FOCUSED);
                        lv_obj_set_style_outline_width(item, 0, LV_STATE_FOCUSED);

                        lv_obj_set_style_bg_color(item, foc_bg, LV_STATE_FOCUS_KEY);
                        lv_obj_set_style_border_color(item, tokens->accent, LV_STATE_FOCUS_KEY);
                        lv_obj_set_style_border_width(item, 2, LV_STATE_FOCUS_KEY);
                        lv_obj_set_style_radius(item, 0, LV_STATE_FOCUS_KEY);
                        lv_obj_set_style_outline_width(item, 0, LV_STATE_FOCUS_KEY);

                        uint32_t btn_kids = lv_obj_get_child_count(item);
                        for (uint32_t k = 0; k < btn_kids; k++) {
                            lv_obj_t *kobj = lv_obj_get_child(item, k);
                            if (!kobj || !lv_obj_is_valid(kobj)) continue;

                            if (lv_obj_check_type(kobj, &lv_label_class)) {
                                /* Icon or simple title */
                                const char *txt = lv_label_get_text(kobj);
                                if (txt && (unsigned char)txt[0] >= 0xEF) {
                                    /* Likely an LV_SYMBOL UTF-8 sequence */
                                    lv_obj_set_style_text_color(kobj, tokens->accent, 0);
                                } else {
                                    lv_obj_set_style_text_color(kobj, tokens->text_primary, 0);
                                    lv_obj_set_style_text_color(kobj, tokens->accent, LV_STATE_FOCUSED);
                                }
                            } else {
                                /* Container inside button (like text_col) */
                                uint32_t sub_cnt = lv_obj_get_child_count(kobj);
                                for (uint32_t s = 0; s < sub_cnt; s++) {
                                    lv_obj_t *sub_lbl = lv_obj_get_child(kobj, s);
                                    if (!sub_lbl || !lv_obj_is_valid(sub_lbl)) continue;
                                    if (s == 0) {
                                        /* Title */
                                        lv_obj_set_style_text_color(sub_lbl, tokens->text_primary, 0);
                                        lv_obj_set_style_text_color(sub_lbl, tokens->accent, LV_STATE_FOCUSED);
                                    } else {
                                        /* Subtitle */
                                        lv_obj_set_style_text_color(sub_lbl, tokens->text_muted, 0);
                                    }
                                }
                            }
                        }
                    } else if (lv_obj_check_type(item, &lv_textarea_class)) {
                        /* Editor Textarea */
                        lv_obj_set_style_bg_color(item, tokens->card_color, 0);
                        lv_obj_set_style_text_color(item, tokens->text_primary, 0);
                        lv_color_t ta_border = (s_current_theme_id == THEME_HIGH_CONTRAST_BW)
                                               ? lv_color_hex(0xFFFFFF)
                                               : (tokens->is_light ? lv_color_hex(0xCCCCCC) : lv_color_hex(0x282C35));
                        lv_obj_set_style_border_color(item, ta_border, 0);
                        lv_obj_set_style_border_width(item, 1, 0);
                        lv_obj_set_style_border_color(item, tokens->accent, LV_STATE_FOCUSED);
                        lv_obj_set_style_border_width(item, 2, LV_STATE_FOCUSED);
                    } else if (lv_obj_check_type(item, &lv_label_class)) {
                        /* Standalone label (headline / subline in media or dashboard) */
                        const char *txt = lv_label_get_text(item);
                        if (txt && (unsigned char)txt[0] >= 0xEF) {
                            lv_obj_set_style_text_color(item, tokens->accent, 0);
                        } else {
                            lv_obj_set_style_text_color(item, tokens->text_primary, 0);
                        }
                    } else if (lv_obj_get_child_count(item) > 0) {
                        /* Generic container or editor sub-header */
                        lv_color_t gc_bg = (s_current_theme_id == THEME_HIGH_CONTRAST_BW || s_current_theme_id == THEME_OLED_BLACK)
                                           ? lv_color_hex(0x000000)
                                           : (tokens->is_light ? lv_color_hex(0xE2E8F0) : lv_color_hex(0x181A20));
                        lv_obj_set_style_bg_color(item, gc_bg, 0);
                    }
                }
            }
        }
    }

    /* 4. Softkey Bar (Last child if present) */
    if (top_child_count > 1) {
        lv_obj_t *softkey_bar = lv_obj_get_child(screen, top_child_count - 1);
        if (softkey_bar && lv_obj_is_valid(softkey_bar)) {
            lv_color_t sk_bg = (s_current_theme_id == THEME_HIGH_CONTRAST_BW || s_current_theme_id == THEME_OLED_BLACK)
                                ? lv_color_hex(0x000000)
                                : (tokens->is_light ? lv_color_hex(0xDCDCDC) : lv_color_hex(0x181A20));
            lv_color_t sk_border = (s_current_theme_id == THEME_HIGH_CONTRAST_BW)
                                   ? lv_color_hex(0xFFFFFF)
                                   : (tokens->is_light ? lv_color_hex(0xBBBBBB) : lv_color_hex(0x282C35));
            lv_obj_set_style_bg_color(softkey_bar, sk_bg, 0);
            lv_obj_set_style_border_color(softkey_bar, sk_border, 0);
            lv_obj_set_style_border_width(softkey_bar, 1, 0);

            uint32_t sk_cnt = lv_obj_get_child_count(softkey_bar);
            for (uint32_t i = 0; i < sk_cnt; i++) {
                lv_obj_t *lbl = lv_obj_get_child(softkey_bar, i);
                if (lbl && lv_obj_is_valid(lbl)) {
                    lv_obj_set_style_text_color(lbl, tokens->text_primary, 0);
                }
            }
        }
    }

    lv_obj_invalidate(screen);
}

void theme_apply_to_obj(lv_obj_t *obj, const os_theme_tokens_t *tokens)
{
    if (!obj || !lv_obj_is_valid(obj) || !tokens) return;

    if (lv_obj_get_parent(obj) == NULL) {
        lv_obj_set_style_bg_color(obj, tokens->bg_color, 0);
    }

    uint32_t child_count = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < child_count; i++) {
        lv_obj_t *child = lv_obj_get_child(obj, i);
        if (child && lv_obj_is_valid(child)) {
            theme_apply_to_obj(child, tokens);
        }
    }

    lv_obj_invalidate(obj);
}

static void screen_walker_cb(lv_obj_t *screen, void *user_data)
{
    const os_theme_tokens_t *tokens = (const os_theme_tokens_t *)user_data;
    theme_apply_to_screen(screen, tokens);
}

void theme_set_palette(os_theme_id_t theme_id)
{
    if (theme_id >= THEME_COUNT) theme_id = THEME_DARK_CYAN;
    theme_update_tokens(theme_id);
    s_initialized = true;
    printf("[THEME] Theme switched to: %s\n", theme_get_name(theme_id));

    /* 1. Update LVGL default display theme */
    lv_display_t *disp = lv_display_get_default();
    if (disp) {
        lv_theme_t *th = lv_theme_default_init(disp,
                                               s_current_theme.accent,
                                               s_current_theme.accent,
                                               !s_current_theme.is_light,
                                               LV_FONT_DEFAULT);
        lv_display_set_theme(disp, th);
    }

    /* 2. Walk all registered root screens in win_mgr */
    win_mgr_for_each_screen(screen_walker_cb, &s_current_theme);

    /* 3. Re-tint all status bar indicator instances */
    status_bar_update_all();

    /* 4. Walk lv_layer_top() overlays (dialogs, drawers, switchers) */
    lv_obj_t *top = lv_layer_top();
    if (top && lv_obj_is_valid(top)) {
        uint32_t top_cnt = lv_obj_get_child_count(top);
        for (uint32_t i = 0; i < top_cnt; i++) {
            lv_obj_t *overlay = lv_obj_get_child(top, i);
            if (overlay && lv_obj_is_valid(overlay)) {
                uint32_t card_cnt = lv_obj_get_child_count(overlay);
                for (uint32_t c = 0; c < card_cnt; c++) {
                    lv_obj_t *card = lv_obj_get_child(overlay, c);
                    if (card && lv_obj_is_valid(card)) {
                        lv_obj_set_style_bg_color(card, s_current_theme.card_color, 0);
                        lv_obj_set_style_border_color(card, s_current_theme.accent, 0);
                        lv_obj_set_style_radius(card, 0, 0);
                        lv_obj_set_style_outline_width(card, 0, 0);
                    }
                }
                lv_obj_invalidate(overlay);
            }
        }
    }

    /* 5. Invalidate active screen */
    lv_obj_t *active = lv_screen_active();
    if (active && lv_obj_is_valid(active)) {
        lv_obj_invalidate(active);
    }
}

void theme_set_mode(bool is_light_mode)
{
    theme_set_palette(is_light_mode ? THEME_LIGHT_CHALK : THEME_DARK_CYAN);
}

bool theme_is_light_mode(void)
{
    if (!s_initialized) theme_init();
    return s_current_theme.is_light;
}

const os_theme_tokens_t * theme_get(void)
{
    if (!s_initialized) theme_init();
    return &s_current_theme;
}
