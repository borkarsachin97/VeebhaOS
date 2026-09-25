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

#include "app_calc.h"
#include "sdk/include/veebha_status_bar.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TAG "APP_CALC"

typedef struct {
    lv_obj_t          *softkey_bar;
    lv_obj_t          *first_item;
    veebha_view_type_t view_type;
    char               title[WIN_MGR_LABEL_MAX];
    os_fullscreen_mode_t fullscreen_mode;
    bool               show_battery_hud;

    lv_obj_t          *expr_lbl;
    lv_obj_t          *result_lbl;
    lv_obj_t          *op_badges[4];
    lv_obj_t          *op_labels[4];

    char               operand1[32];
    char               operand2[32];
    char               saved_expr[128];
    int                active_op; /* -1: none, 0: '+', 1: '-', 2: '×', 3: '÷' */
    bool               has_result;
    bool               has_error;
} calc_screen_data_t;

static const char *s_op_symbols[4] = { "+", "-", "*", "/" };
static lv_obj_t *s_calc_active_scr = NULL;

bool app_calc_is_active(void)
{
    return s_calc_active_scr != NULL && win_mgr_get_active_view_type() == VEEBHA_VIEW_TYPE_CALC;
}

const char * app_calc_get_result_str(void)
{
    if (!s_calc_active_scr) return "";
    calc_screen_data_t *data = (calc_screen_data_t *)lv_obj_get_user_data(s_calc_active_scr);
    if (!data) return "";
    return data->operand1;
}

bool app_calc_has_error(void)
{
    if (!s_calc_active_scr) return false;
    calc_screen_data_t *data = (calc_screen_data_t *)lv_obj_get_user_data(s_calc_active_scr);
    if (!data) return false;
    return data->has_error;
}

static void on_calc_back_action(void);
static void on_calc_clear_action(void);
static void on_calc_clear_all_action(void);
static void on_calc_equals_action(void);

static void update_calc_ui(calc_screen_data_t *data)
{
    if (!data) return;

    /* Update Expression Label */
    if (data->has_result || data->has_error) {
        lv_label_set_text(data->expr_lbl, data->saved_expr);
    } else if (data->active_op >= 0 && data->active_op < 4) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%s %s", data->operand1, s_op_symbols[data->active_op]);
        lv_label_set_text(data->expr_lbl, buf);
    } else {
        lv_label_set_text(data->expr_lbl, "");
    }

    /* Update Bottom Entry / Result Label */
    if (data->has_error) {
        lv_label_set_text(data->result_lbl, "Error");
        lv_obj_set_style_text_color(data->result_lbl, lv_color_hex(0xFF5555), 0);
    } else if (data->has_result) {
        char buf[40];
        snprintf(buf, sizeof(buf), "= %s", data->operand1);
        lv_label_set_text(data->result_lbl, buf);
        lv_obj_set_style_text_color(data->result_lbl, theme_get()->accent, 0);
    } else {
        lv_obj_set_style_text_color(data->result_lbl, theme_get()->accent, 0);
        if (data->active_op >= 0 && strlen(data->operand2) > 0) {
            lv_label_set_text(data->result_lbl, data->operand2);
        } else if (strlen(data->operand1) > 0) {
            lv_label_set_text(data->result_lbl, data->operand1);
        } else {
            lv_label_set_text(data->result_lbl, "0");
        }
    }

    /* Update Operator Badges */
    for (int i = 0; i < 4; i++) {
        if (!data->op_badges[i] || !data->op_labels[i]) continue;
        if (i == data->active_op) {
            lv_obj_set_style_bg_color(data->op_badges[i], theme_get()->accent, 0);
            lv_obj_set_style_bg_opa(data->op_badges[i], LV_OPA_COVER, 0);
            lv_obj_set_style_text_color(data->op_labels[i], lv_color_hex(0x000000), 0);
        } else {
            lv_obj_set_style_bg_color(data->op_badges[i], theme_get()->card_color, 0);
            lv_obj_set_style_bg_opa(data->op_badges[i], LV_OPA_COVER, 0);
            lv_obj_set_style_text_color(data->op_labels[i], theme_get()->text_primary, 0);
        }
    }

    /* Update Softkeys */
    bool has_input = (strlen(data->operand1) > 0 ||
                      strlen(data->operand2) > 0 ||
                      data->active_op >= 0 ||
                      data->has_result ||
                      data->has_error);

    if (has_input) {
        softkey_set_actions("=", on_calc_equals_action, "Clear", on_calc_clear_action);
        softkey_set_rsk_long_action(on_calc_clear_all_action);
    } else {
        softkey_set_actions("=", on_calc_equals_action, "Back", on_calc_back_action);
        softkey_set_rsk_long_action(NULL);
    }
}

static void on_calc_clear_all_action(void)
{
    if (!s_calc_active_scr) return;
    calc_screen_data_t *data = (calc_screen_data_t *)lv_obj_get_user_data(s_calc_active_scr);
    if (!data) return;

    data->operand1[0] = '\0';
    data->operand2[0] = '\0';
    data->saved_expr[0] = '\0';
    data->active_op = -1;
    data->has_result = false;
    data->has_error = false;

    update_calc_ui(data);
}

static void on_calc_clear_action(void)
{
    if (!s_calc_active_scr) return;
    calc_screen_data_t *data = (calc_screen_data_t *)lv_obj_get_user_data(s_calc_active_scr);
    if (!data) return;

    if (data->has_result || data->has_error) {
        on_calc_clear_all_action();
        return;
    }

    if (strlen(data->operand2) > 0) {
        size_t len = strlen(data->operand2);
        data->operand2[len - 1] = '\0';
    } else if (data->active_op >= 0) {
        data->active_op = -1;
    } else if (strlen(data->operand1) > 0) {
        size_t len = strlen(data->operand1);
        data->operand1[len - 1] = '\0';
    }

    update_calc_ui(data);
}

static void on_calc_back_action(void)
{
    win_mgr_pop();
}

static void on_calc_equals_action(void)
{
    if (!s_calc_active_scr) return;
    calc_screen_data_t *data = (calc_screen_data_t *)lv_obj_get_user_data(s_calc_active_scr);
    if (!data) return;

    if (data->has_error) {
        on_calc_clear_all_action();
        return;
    }

    if (data->active_op < 0 || strlen(data->operand2) == 0) {
        return;
    }

    long long a = 0;
    long long b = 0;
    const char *pa = data->operand1;
    int sign_a = 1;
    if (*pa == '-') { sign_a = -1; pa++; }
    while (*pa >= '0' && *pa <= '9') { a = a * 10 + (*pa - '0'); pa++; }
    a *= sign_a;

    const char *pb = data->operand2;
    int sign_b = 1;
    if (*pb == '-') { sign_b = -1; pb++; }
    while (*pb >= '0' && *pb <= '9') { b = b * 10 + (*pb - '0'); pb++; }
    b *= sign_b;

    long long res = 0;

    switch (data->active_op) {
    case 0: res = a + b; break;
    case 1: res = a - b; break;
    case 2: res = a * b; break;
    case 3:
        if (b == 0) {
            data->has_error = true;
            snprintf(data->saved_expr, sizeof(data->saved_expr), "%s / 0", data->operand1);
            data->operand1[0] = '\0';
            data->operand2[0] = '\0';
            data->active_op = -1;
            update_calc_ui(data);
            return;
        }
        res = a / b;
        break;
    default:
        return;
    }

    /* Format expression line */
    snprintf(data->saved_expr, sizeof(data->saved_expr), "%s %s %s",
             data->operand1, s_op_symbols[data->active_op], data->operand2);

    /* Format result */
    char res_buf[32];
    snprintf(res_buf, sizeof(res_buf), "%lld", res);

    strncpy(data->operand1, res_buf, sizeof(data->operand1) - 1);
    data->operand1[sizeof(data->operand1) - 1] = '\0';
    data->operand2[0] = '\0';
    data->active_op = -1;
    data->has_result = true;
    data->has_error = false;

    update_calc_ui(data);
}

static void set_active_operator(calc_screen_data_t *data, int op)
{
    if (!data) return;
    if (data->has_error) {
        on_calc_clear_all_action();
        return;
    }

    if (strlen(data->operand1) == 0) {
        strcpy(data->operand1, "0");
    }

    if (data->active_op >= 0 && strlen(data->operand2) > 0) {
        /* Evaluate chained math first */
        on_calc_equals_action();
    }

    data->active_op = op;
    data->has_result = false;
    update_calc_ui(data);
}

void app_calc_handle_key(veebha_key_t key)
{
    if (!s_calc_active_scr) return;
    calc_screen_data_t *data = (calc_screen_data_t *)lv_obj_get_user_data(s_calc_active_scr);
    if (!data) return;

    if (key == VEEBHA_KEY_OK) {
        on_calc_equals_action();
        return;
    }

    if (key >= VEEBHA_KEY_NUM_0 && key <= VEEBHA_KEY_NUM_9) {
        char digit = '0' + (key - VEEBHA_KEY_NUM_0);

        if (data->has_result || data->has_error) {
            data->operand1[0] = '\0';
            data->operand2[0] = '\0';
            data->saved_expr[0] = '\0';
            data->active_op = -1;
            data->has_result = false;
            data->has_error = false;
        }

        if (data->active_op < 0) {
            size_t len = strlen(data->operand1);
            if (len < 24) {
                if (len == 1 && data->operand1[0] == '0') {
                    data->operand1[0] = digit;
                } else {
                    data->operand1[len] = digit;
                    data->operand1[len + 1] = '\0';
                }
            }
        } else {
            size_t len = strlen(data->operand2);
            if (len < 24) {
                if (len == 1 && data->operand2[0] == '0') {
                    data->operand2[0] = digit;
                } else {
                    data->operand2[len] = digit;
                    data->operand2[len + 1] = '\0';
                }
            }
        }
        update_calc_ui(data);
    } else if (key == VEEBHA_KEY_STAR) {
        /* Decimal point '.' */
        if (data->has_result || data->has_error) {
            data->operand1[0] = '\0';
            data->operand2[0] = '\0';
            data->saved_expr[0] = '\0';
            data->active_op = -1;
            data->has_result = false;
            data->has_error = false;
        }

        if (data->active_op < 0) {
            if (strchr(data->operand1, '.') == NULL) {
                if (strlen(data->operand1) == 0) {
                    strcpy(data->operand1, "0.");
                } else {
                    strcat(data->operand1, ".");
                }
            }
        } else {
            if (strchr(data->operand2, '.') == NULL) {
                if (strlen(data->operand2) == 0) {
                    strcpy(data->operand2, "0.");
                } else {
                    strcat(data->operand2, ".");
                }
            }
        }
        update_calc_ui(data);
    } else if (key == VEEBHA_KEY_HASH) {
        /* Cycle operators '+' -> '-' -> '*' -> '/' */
        int next_op = (data->active_op < 0) ? 0 : (data->active_op + 1) % 4;
        set_active_operator(data, next_op);
    } else if (key == VEEBHA_KEY_UP) {
        /* Direct operator shortcut: '+' */
        set_active_operator(data, 0);
    } else if (key == VEEBHA_KEY_DOWN) {
        /* Direct operator shortcut: '-' */
        set_active_operator(data, 1);
    } else if (key == VEEBHA_KEY_LEFT) {
        /* Direct operator shortcut: '*' */
        set_active_operator(data, 2);
    } else if (key == VEEBHA_KEY_RIGHT) {
        /* Direct operator shortcut: '/' */
        set_active_operator(data, 3);
    }
}

static void on_calc_delete_cb(lv_event_t *e)
{
    lv_obj_t *scr = lv_event_get_target(e);
    if (scr == s_calc_active_scr) {
        s_calc_active_scr = NULL;
    }
    calc_screen_data_t *data = (calc_screen_data_t *)lv_obj_get_user_data(scr);
    if (data) {
        free(data);
        lv_obj_set_user_data(scr, NULL);
    }
}

static void on_calc_key_event_cb(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_ENTER || key == '\n' || key == '\r') {
        on_calc_equals_action();
    }
}

lv_obj_t * app_calc_create(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, 176, 220);
    lv_obj_set_style_bg_color(screen, theme_get()->bg_color, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    calc_screen_data_t *data = (calc_screen_data_t *)calloc(1, sizeof(calc_screen_data_t));
    if (!data) {
        lv_obj_del(screen);
        return NULL;
    }

    data->view_type = VEEBHA_VIEW_TYPE_CALC;
    strncpy(data->title, "Calculator", sizeof(data->title) - 1);
    data->active_op = -1;

    lv_obj_set_user_data(screen, data);
    lv_obj_add_event_cb(screen, on_calc_delete_cb, LV_EVENT_DELETE, NULL);
    s_calc_active_scr = screen;

    /* 1. Zone A: Fixed 18px Top Status Bar */
    status_bar_create(screen, NULL);

    /* 2. Header Strip: CALCULATOR (18px) */
    lv_obj_t *hdr = lv_obj_create(screen);
    lv_obj_set_size(hdr, lv_pct(100), 18);
    lv_obj_set_style_bg_color(hdr, theme_get()->card_color, 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(hdr, theme_is_light_mode() ? lv_color_hex(0xE2E8F0) : lv_color_hex(0x282C35), 0);
    lv_obj_set_style_border_width(hdr, 1, 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *hdr_lbl = lv_label_create(hdr);
    lv_label_set_text(hdr_lbl, "CALCULATOR");
    lv_obj_set_style_text_color(hdr_lbl, theme_get()->accent, 0);
    lv_obj_set_style_text_font(hdr_lbl, &lv_font_montserrat_12, 0);

    /* 3. Zone B: Elastic Viewport (184px Viewport) */
    lv_obj_t *viewport = lv_obj_create(screen);
    lv_obj_set_size(viewport, lv_pct(100), 0);
    lv_obj_set_flex_grow(viewport, 1);
    lv_obj_set_style_bg_color(viewport, theme_get()->bg_color, 0);
    lv_obj_set_style_bg_opa(viewport, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(viewport, 0, 0);
    lv_obj_set_style_radius(viewport, 0, 0);
    lv_obj_set_style_pad_all(viewport, 6, 0);
    lv_obj_set_flex_flow(viewport, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(viewport, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(viewport, LV_OBJ_FLAG_SCROLLABLE);

    /* 3a. Display Card (48px) */
    lv_obj_t *card = lv_obj_create(viewport);
    lv_obj_set_size(card, lv_pct(100), 48);
    lv_obj_set_style_bg_color(card, theme_get()->card_color, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, theme_is_light_mode() ? lv_color_hex(0xE2E8F0) : lv_color_hex(0x30363D), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 6, 0);
    lv_obj_set_style_pad_all(card, 4, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    data->expr_lbl = lv_label_create(card);
    lv_label_set_text(data->expr_lbl, "");
    lv_obj_set_style_text_font(data->expr_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(data->expr_lbl, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_align(data->expr_lbl, LV_TEXT_ALIGN_RIGHT, 0);

    data->result_lbl = lv_label_create(card);
    lv_label_set_text(data->result_lbl, "0");
    lv_obj_set_style_text_font(data->result_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(data->result_lbl, theme_get()->accent, 0);
    lv_obj_set_style_text_align(data->result_lbl, LV_TEXT_ALIGN_RIGHT, 0);

    /* 3b. Operator Selector Bar: 3x3 D-Pad Diamond Grid (60x60px centered) */
    static int32_t col_dsc[] = { 18, 18, 18, LV_GRID_TEMPLATE_LAST };
    static int32_t row_dsc[] = { 18, 18, 18, LV_GRID_TEMPLATE_LAST };

    lv_obj_t *op_bar = lv_obj_create(viewport);
    lv_obj_set_size(op_bar, 62, 62);
    lv_obj_set_style_bg_opa(op_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(op_bar, 0, 0);
    lv_obj_set_style_radius(op_bar, 0, 0);
    lv_obj_set_style_pad_all(op_bar, 0, 0);
    lv_obj_set_style_pad_row(op_bar, 2, 0);
    lv_obj_set_style_pad_column(op_bar, 2, 0);
    lv_obj_set_grid_dsc_array(op_bar, col_dsc, row_dsc);
    lv_obj_remove_flag(op_bar, LV_OBJ_FLAG_SCROLLABLE);

    /* Grid positions for [0: +, 1: -, 2: ×, 3: ÷] */
    static const uint8_t grid_pos[4][2] = {
        { 1, 0 }, /* Index 0: '+' -> Col 1, Row 0 (Top Center: D-pad Up) */
        { 1, 2 }, /* Index 1: '-' -> Col 1, Row 2 (Bottom Center: D-pad Down) */
        { 0, 1 }, /* Index 2: '×' -> Col 0, Row 1 (Mid Left: D-pad Left) */
        { 2, 1 }, /* Index 3: '÷' -> Col 2, Row 1 (Mid Right: D-pad Right) */
    };

    for (int i = 0; i < 4; i++) {
        lv_obj_t *badge = lv_obj_create(op_bar);
        data->op_badges[i] = badge;
        lv_obj_set_grid_cell(badge, LV_GRID_ALIGN_STRETCH, grid_pos[i][0], 1,
                                   LV_GRID_ALIGN_STRETCH, grid_pos[i][1], 1);
        lv_obj_set_style_bg_color(badge, theme_get()->card_color, 0);
        lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(badge, 1, 0);
        lv_obj_set_style_border_color(badge, theme_is_light_mode() ? lv_color_hex(0xE2E8F0) : lv_color_hex(0x30363D), 0);
        lv_obj_set_style_radius(badge, 4, 0);
        lv_obj_set_style_pad_all(badge, 0, 0);
        lv_obj_remove_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *lbl = lv_label_create(badge);
        data->op_labels[i] = lbl;
        lv_label_set_text(lbl, s_op_symbols[i]);
        lv_obj_center(lbl);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(lbl, theme_get()->text_primary, 0);
    }

    /* 3c. Quick Guide */
    lv_obj_t *guide_lbl = lv_label_create(viewport);
    lv_label_set_text(guide_lbl, "D-pad: Up +, Down -, Left *, Right /");
    lv_obj_set_style_text_font(guide_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(guide_lbl, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_align(guide_lbl, LV_TEXT_ALIGN_CENTER, 0);

    /* 4. Zone C: Bottom 20px Softkey Bar */
    data->softkey_bar = softkey_bar_create(screen, "=", "Back");
    softkey_set_actions("=", on_calc_equals_action, "Back", on_calc_back_action);

    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, on_calc_key_event_cb, LV_EVENT_KEY, NULL);

    return screen;
}

void app_calc_open(void)
{
    lv_obj_t *calc_scr = app_calc_create();
    if (calc_scr) {
        win_mgr_push(calc_scr, "=", on_calc_equals_action, "Back", on_calc_back_action);
        lv_group_t *g = win_mgr_get_group();
        if (g) {
            lv_group_focus_obj(calc_scr);
        }
    }
}
