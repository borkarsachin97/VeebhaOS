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

#include "app_calendar.h"
#include "sdk/include/app_registry.h"
#include "sdk/include/veebha_status_bar.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_log.h"
#include "sdk/include/veebha_i18n.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "APP_CALENDAR"

#define CALENDAR_CELL_COUNT 42

typedef struct {
    lv_obj_t          *softkey_bar;
    lv_obj_t          *first_item;
    veebha_view_type_t view_type;
    char               title[WIN_MGR_LABEL_MAX];
    os_fullscreen_mode_t fullscreen_mode;
    bool               show_battery_hud;

    int                display_year;
    int                display_month; /* 1..12 */
    int                today_year;
    int                today_month;
    int                today_day;

    lv_obj_t          *month_hdr_btn;
    lv_obj_t          *month_lbl;
    lv_obj_t          *day_cells[CALENDAR_CELL_COUNT];
    lv_obj_t          *day_labels[CALENDAR_CELL_COUNT];
} calendar_screen_data_t;

static const char *s_month_names[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

static const char *s_weekday_names[] = {
    "Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"
};

static bool is_leap_year(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int get_days_in_month(int year, int month)
{
    static const int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month == 2 && is_leap_year(year)) return 29;
    if (month >= 1 && month <= 12) return days[month - 1];
    return 30;
}

/* Returns Monday=0, Tuesday=1, ..., Sunday=6 */
static int get_month_start_dow(int year, int month)
{
    static const int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    int y = year;
    if (month < 3) y -= 1;
    int dow_sun = (y + y / 4 - y / 100 + y / 400 + t[month - 1] + 1) % 7;
    return (dow_sun + 6) % 7;
}

static void render_calendar_month(calendar_screen_data_t *data, int focus_day)
{
    if (!data) return;

    /* Update Month Header Label */
    char hdr_buf[32];
    const char *mname = (data->display_month >= 1 && data->display_month <= 12) ?
                        s_month_names[data->display_month - 1] : "Unknown";
    snprintf(hdr_buf, sizeof(hdr_buf), "< %s %d >", mname, data->display_year);
    if (data->month_lbl) {
        lv_label_set_text(data->month_lbl, hdr_buf);
    }

    int start_dow = get_month_start_dow(data->display_year, data->display_month);
    int days_in_month = get_days_in_month(data->display_year, data->display_month);

    lv_obj_t *target_focus_obj = NULL;

    for (int i = 0; i < CALENDAR_CELL_COUNT; i++) {
        lv_obj_t *cell = data->day_cells[i];
        lv_obj_t *lbl = data->day_labels[i];
        if (!cell || !lbl) continue;

        if (i < start_dow || i >= (start_dow + days_in_month)) {
            lv_label_set_text(lbl, "");
            lv_obj_add_flag(cell, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(cell, LV_OBJ_FLAG_CLICKABLE);
        } else {
            int day = i - start_dow + 1;
            char day_buf[16];
            snprintf(day_buf, sizeof(day_buf), "%d", day);
            lv_label_set_text(lbl, day_buf);
            lv_obj_clear_flag(cell, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(cell, LV_OBJ_FLAG_CLICKABLE);

            bool is_today = (data->display_year == data->today_year &&
                             data->display_month == data->today_month &&
                             day == data->today_day);

            if (is_today) {
                /* Current day accent highlight */
                lv_obj_set_style_bg_color(cell, theme_get()->accent, 0);
                lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
                lv_obj_set_style_text_color(lbl, lv_color_hex(0x000000), 0);
            } else {
                lv_obj_set_style_bg_color(cell, theme_get()->card_color, 0);
                lv_obj_set_style_bg_opa(cell, LV_OPA_40, 0);
                lv_obj_set_style_text_color(lbl, theme_get()->text_primary, 0);
            }

            if (day == focus_day) {
                target_focus_obj = cell;
            }
        }
    }

    if (win_mgr_get_active_view_type() == VEEBHA_VIEW_TYPE_CALENDAR) {
        if (target_focus_obj) {
            lv_group_focus_obj(target_focus_obj);
        } else if (focus_day <= 0) {
            /* Default to focusing day 1 */
            if (start_dow < CALENDAR_CELL_COUNT && data->day_cells[start_dow]) {
                lv_group_focus_obj(data->day_cells[start_dow]);
            }
        }
    }
}

static void on_calendar_delete_cb(lv_event_t *e)
{
    lv_obj_t *scr = lv_event_get_target(e);
    calendar_screen_data_t *data = (calendar_screen_data_t *)lv_obj_get_user_data(scr);
    if (data) {
        free(data);
        lv_obj_set_user_data(scr, NULL);
    }
}

static void on_month_header_key_cb(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *scr = lv_obj_get_screen(btn);
    calendar_screen_data_t *data = (calendar_screen_data_t *)lv_obj_get_user_data(scr);
    if (!data) return;

    if (key == LV_KEY_LEFT) {
        data->display_month--;
        if (data->display_month < 1) {
            data->display_month = 12;
            data->display_year--;
        }
        render_calendar_month(data, -1);
        lv_group_focus_obj(data->month_hdr_btn);
    } else if (key == LV_KEY_RIGHT) {
        data->display_month++;
        if (data->display_month > 12) {
            data->display_month = 1;
            data->display_year++;
        }
        render_calendar_month(data, -1);
        lv_group_focus_obj(data->month_hdr_btn);
    } else if (key == LV_KEY_DOWN) {
        int start_dow = get_month_start_dow(data->display_year, data->display_month);
        if (data->day_cells[start_dow]) {
            lv_group_focus_obj(data->day_cells[start_dow]);
        }
    }
}

static void on_day_cell_key_cb(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    uint16_t idx = (uint16_t)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *scr = lv_obj_get_screen(btn);
    calendar_screen_data_t *data = (calendar_screen_data_t *)lv_obj_get_user_data(scr);
    if (!data) return;

    int start_dow = get_month_start_dow(data->display_year, data->display_month);
    int days_in_month = get_days_in_month(data->display_year, data->display_month);
    int current_day = (int)idx - start_dow + 1;

    if (key == LV_KEY_LEFT) {
        if (current_day <= 1) {
            /* Flip to previous month, focus last day */
            data->display_month--;
            if (data->display_month < 1) {
                data->display_month = 12;
                data->display_year--;
            }
            int prev_days = get_days_in_month(data->display_year, data->display_month);
            render_calendar_month(data, prev_days);
        } else {
            int target_idx = idx - 1;
            if (target_idx >= 0 && data->day_cells[target_idx]) {
                lv_group_focus_obj(data->day_cells[target_idx]);
            }
        }
    } else if (key == LV_KEY_RIGHT) {
        if (current_day >= days_in_month) {
            /* Flip to next month, focus day 1 */
            data->display_month++;
            if (data->display_month > 12) {
                data->display_month = 1;
                data->display_year++;
            }
            render_calendar_month(data, 1);
        } else {
            int target_idx = idx + 1;
            if (target_idx < CALENDAR_CELL_COUNT && data->day_cells[target_idx]) {
                lv_group_focus_obj(data->day_cells[target_idx]);
            }
        }
    } else if (key == LV_KEY_UP) {
        if (current_day <= 7) {
            /* Move up into month header switcher */
            if (data->month_hdr_btn) {
                lv_group_focus_obj(data->month_hdr_btn);
            }
        } else {
            int target_idx = idx - 7;
            if (target_idx >= 0 && data->day_cells[target_idx]) {
                lv_group_focus_obj(data->day_cells[target_idx]);
            }
        }
    } else if (key == LV_KEY_DOWN) {
        if (current_day + 7 <= days_in_month) {
            int target_idx = idx + 7;
            if (target_idx < CALENDAR_CELL_COUNT && data->day_cells[target_idx]) {
                lv_group_focus_obj(data->day_cells[target_idx]);
            }
        }
    }
}

static void on_calendar_today_action(void)
{
    lv_obj_t *top_scr = lv_scr_act();
    calendar_screen_data_t *data = (calendar_screen_data_t *)lv_obj_get_user_data(top_scr);
    if (!data) return;

    OS_LOGI(TAG, "Navigating to today: %d-%02d-%02d",
            data->today_year, data->today_month, data->today_day);

    data->display_year = data->today_year;
    data->display_month = data->today_month;
    render_calendar_month(data, data->today_day);
}

static void on_calendar_back_action(void)
{
    win_mgr_pop();
}

lv_obj_t * app_calendar_create(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, 176, 220);
    lv_obj_set_style_bg_color(screen, theme_get()->bg_color, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    calendar_screen_data_t *data = (calendar_screen_data_t *)calloc(1, sizeof(calendar_screen_data_t));
    if (!data) {
        lv_obj_del(screen);
        return NULL;
    }

    data->view_type = VEEBHA_VIEW_TYPE_CALENDAR;
    strncpy(data->title, "Calendar", sizeof(data->title) - 1);

    uint16_t cur_year = 2026;
    uint8_t cur_month = 9, cur_day = 23;
    status_bar_get_rtc_date(&cur_year, &cur_month, &cur_day);

    data->today_year = (int)cur_year;
    data->today_month = (int)cur_month;
    data->today_day = (int)cur_day;
    data->display_year = (int)cur_year;
    data->display_month = (int)cur_month;

    lv_obj_set_user_data(screen, data);
    lv_obj_add_event_cb(screen, on_calendar_delete_cb, LV_EVENT_DELETE, NULL);

    /* 1. Zone A: Fixed 18px Top Status Bar */
    status_bar_create(screen, NULL);

    /* 2. Sub-Header: Month Switcher (18px) */
    lv_obj_t *month_hdr = lv_btn_create(screen);
    data->month_hdr_btn = month_hdr;
    lv_obj_set_size(month_hdr, lv_pct(100), 18);
    lv_obj_set_style_bg_color(month_hdr, theme_get()->card_color, 0);
    lv_obj_set_style_bg_opa(month_hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(month_hdr, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(month_hdr, theme_is_light_mode() ? lv_color_hex(0xE2E8F0) : lv_color_hex(0x282C35), 0);
    lv_obj_set_style_border_width(month_hdr, 1, 0);
    lv_obj_set_style_radius(month_hdr, 0, 0);
    lv_obj_set_style_pad_all(month_hdr, 0, 0);
    lv_obj_set_flex_flow(month_hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(month_hdr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Highlight border when header is focused */
    lv_obj_set_style_border_color(month_hdr, theme_get()->accent, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(month_hdr, 1, LV_STATE_FOCUSED);

    lv_obj_t *m_lbl = lv_label_create(month_hdr);
    data->month_lbl = m_lbl;
    lv_obj_set_style_text_color(m_lbl, theme_get()->accent, 0);
    lv_obj_set_style_text_font(m_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_align(m_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(m_lbl, "< September 2026 >");

    lv_group_t *grp = win_mgr_get_group();
    if (grp) {
        lv_group_add_obj(grp, month_hdr);
    }
    lv_obj_add_event_cb(month_hdr, on_month_header_key_cb, LV_EVENT_KEY, NULL);

    /* 3. Zone B: Elastic Viewport (184px Viewport) */
    lv_obj_t *viewport = lv_obj_create(screen);
    lv_obj_set_size(viewport, lv_pct(100), 0);
    lv_obj_set_flex_grow(viewport, 1);
    lv_obj_set_style_bg_color(viewport, theme_get()->bg_color, 0);
    lv_obj_set_style_bg_opa(viewport, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(viewport, 0, 0);
    lv_obj_set_style_radius(viewport, 0, 0);
    lv_obj_set_style_pad_all(viewport, 2, 0);
    lv_obj_set_flex_flow(viewport, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(viewport, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(viewport, LV_SCROLLBAR_MODE_OFF);

    /* 3a. Weekday Row (14px) */
    lv_obj_t *wk_row = lv_obj_create(viewport);
    lv_obj_set_size(wk_row, lv_pct(100), 14);
    lv_obj_set_style_bg_opa(wk_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wk_row, 0, 0);
    lv_obj_set_style_radius(wk_row, 0, 0);
    lv_obj_set_style_pad_all(wk_row, 0, 0);
    lv_obj_set_flex_flow(wk_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(wk_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(wk_row, LV_SCROLLBAR_MODE_OFF);

    for (int d = 0; d < 7; d++) {
        lv_obj_t *lbl = lv_label_create(wk_row);
        lv_label_set_text(lbl, s_weekday_names[d]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x8B949E), 0); /* Muted gray */
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    }

    /* 3b. 7x6 Day Grid */
    lv_obj_t *grid = lv_obj_create(viewport);
    lv_obj_set_size(grid, lv_pct(100), 0);
    lv_obj_set_flex_grow(grid, 1);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_radius(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 1, 0);
    lv_obj_set_style_pad_row(grid, 2, 0);
    lv_obj_set_style_pad_column(grid, 2, 0);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_OFF);

    static const int32_t col_dsc[] = {
        LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
        LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST
    };
    static const int32_t row_dsc[] = {
        LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
        LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST
    };
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);

    for (int r = 0; r < 6; r++) {
        for (int c = 0; c < 7; c++) {
            int idx = r * 7 + c;
            lv_obj_t *cell_btn = lv_btn_create(grid);
            data->day_cells[idx] = cell_btn;
            lv_obj_set_grid_cell(cell_btn, LV_GRID_ALIGN_STRETCH, c, 1,
                                 LV_GRID_ALIGN_STRETCH, r, 1);

            lv_obj_set_style_radius(cell_btn, 3, 0);
            lv_obj_set_style_pad_all(cell_btn, 0, 0);
            lv_obj_set_style_border_width(cell_btn, 0, 0);

            /* Focused state: Cyan high-contrast border */
            lv_obj_set_style_border_color(cell_btn, theme_get()->accent, LV_STATE_FOCUSED);
            lv_obj_set_style_border_width(cell_btn, 2, LV_STATE_FOCUSED);

            lv_obj_t *lbl = lv_label_create(cell_btn);
            data->day_labels[idx] = lbl;
            lv_obj_center(lbl);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_10, 0);

            if (grp) {
                lv_group_add_obj(grp, cell_btn);
            }
            lv_obj_add_event_cb(cell_btn, on_day_cell_key_cb, LV_EVENT_KEY, (void *)(uintptr_t)idx);
        }
    }

    /* 4. Zone C: Bottom 20px Softkey Bar */
    data->softkey_bar = softkey_bar_create(screen, "Today", "Back");
    softkey_set_actions("Today", on_calendar_today_action, "Back", on_calendar_back_action);

    /* Render current month and focus today */
    render_calendar_month(data, data->today_day);

    int start_dow = get_month_start_dow(data->today_year, data->today_month);
    int day_idx = start_dow + data->today_day - 1;
    if (day_idx >= 0 && day_idx < CALENDAR_CELL_COUNT) {
        data->first_item = data->day_cells[day_idx];
    }

    return screen;
}

void app_calendar_init(void)
{
    OS_LOGI(TAG, "Calendar application module initialized");
}

void app_calendar_open(void)
{
    lv_obj_t *cal_scr = app_calendar_create();
    if (cal_scr) {
        win_mgr_push(cal_scr, "Today", on_calendar_today_action, "Back", on_calendar_back_action);
    }
}
