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

#include "app_game.h"
#include "sdk/include/veebha_status_bar.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_templates.h"
#include "sdk/include/veebha_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "APP_GAME"

#define SNAKE_GRID_SIZE 16
#define SNAKE_CELL_PX   10
#define SNAKE_MAX_LEN   64

typedef enum {
    DIR_UP = 0,
    DIR_RIGHT,
    DIR_DOWN,
    DIR_LEFT
} snake_dir_t;

typedef struct {
    int8_t x;
    int8_t y;
} point_t;

typedef struct {
    lv_obj_t          *softkey_bar;
    lv_obj_t          *first_item;
    veebha_view_type_t view_type;
    char               title[WIN_MGR_LABEL_MAX];
    os_fullscreen_mode_t fullscreen_mode;
    bool               show_battery_hud;

    lv_obj_t          *arena;
    lv_obj_t          *score_lbl;
    lv_obj_t          *snake_objs[SNAKE_MAX_LEN];
    lv_obj_t          *food_obj;

    point_t            snake[SNAKE_MAX_LEN];
    uint16_t           length;
    snake_dir_t        dir;
    snake_dir_t        next_dir;
    point_t            food;
    uint16_t           score;
    bool               is_paused;
    bool               is_game_over;

    lv_timer_t        *game_timer;
} game_screen_data_t;

static void spawn_food(game_screen_data_t *data)
{
    if (!data) return;
    bool valid = false;
    while (!valid) {
        data->food.x = rand() % SNAKE_GRID_SIZE;
        data->food.y = rand() % SNAKE_GRID_SIZE;
        valid = true;
        for (uint16_t i = 0; i < data->length; i++) {
            if (data->snake[i].x == data->food.x && data->snake[i].y == data->food.y) {
                valid = false;
                break;
            }
        }
    }
    if (data->food_obj) {
        lv_obj_set_pos(data->food_obj, data->food.x * SNAKE_CELL_PX, data->food.y * SNAKE_CELL_PX);
    }
}

static void on_game_exit_action(void);
static void reset_snake_game(game_screen_data_t *data);

static void on_game_over_dialog_restart(void)
{
    tpl_dialog_close();
    lv_obj_t *top = lv_scr_act();
    game_screen_data_t *data = (game_screen_data_t *)lv_obj_get_user_data(top);
    if (data) {
        reset_snake_game(data);
    }
}

static void on_game_over_dialog_exit(void)
{
    tpl_dialog_close();
    on_game_exit_action();
}

static void trigger_game_over(game_screen_data_t *data)
{
    if (!data) return;
    data->is_game_over = true;
    data->is_paused = true;

    static char msg_buf[64];
    snprintf(msg_buf, sizeof(msg_buf), "Game Over!\nFinal Score: %u", data->score);

    static tpl_dialog_desc_t dlg = {
        .title = "Snake",
        .icon = LV_SYMBOL_PLAY,
        .message = msg_buf,
        .lsk_label = "Retry",
        .rsk_label = "Exit",
        .on_confirm = on_game_over_dialog_restart,
        .on_cancel = on_game_over_dialog_exit
    };
    tpl_dialog_show(&dlg);
}

static void on_game_tick(lv_timer_t *tmr)
{
    game_screen_data_t *data = (game_screen_data_t *)lv_timer_get_user_data(tmr);
    if (!data || data->is_paused || data->is_game_over) return;

    data->dir = data->next_dir;
    point_t head = data->snake[0];

    switch (data->dir) {
    case DIR_UP:    head.y--; break;
    case DIR_DOWN:  head.y++; break;
    case DIR_LEFT:  head.x--; break;
    case DIR_RIGHT: head.x++; break;
    }

    /* Wall collision */
    if (head.x < 0 || head.x >= SNAKE_GRID_SIZE || head.y < 0 || head.y >= SNAKE_GRID_SIZE) {
        trigger_game_over(data);
        return;
    }

    /* Self collision */
    for (uint16_t i = 0; i < data->length; i++) {
        if (data->snake[i].x == head.x && data->snake[i].y == head.y) {
            trigger_game_over(data);
            return;
        }
    }

    /* Check food eating */
    bool ate_food = (head.x == data->food.x && head.y == data->food.y);

    if (ate_food) {
        if (data->length < SNAKE_MAX_LEN) {
            data->length++;
            lv_obj_clear_flag(data->snake_objs[data->length - 1], LV_OBJ_FLAG_HIDDEN);
        }
        data->score += 10;
        char sbuf[32];
        snprintf(sbuf, sizeof(sbuf), "SNAKE | Score: %02u", data->score);
        lv_label_set_text(data->score_lbl, sbuf);
        spawn_food(data);
    }

    /* Shift body segments */
    for (int i = (int)data->length - 1; i > 0; i--) {
        data->snake[i] = data->snake[i - 1];
    }
    data->snake[0] = head;

    /* Update object positions */
    for (uint16_t i = 0; i < data->length; i++) {
        lv_obj_set_pos(data->snake_objs[i], data->snake[i].x * SNAKE_CELL_PX, data->snake[i].y * SNAKE_CELL_PX);
    }
}

static void reset_snake_game(game_screen_data_t *data)
{
    if (!data) return;
    data->length = 3;
    data->dir = DIR_RIGHT;
    data->next_dir = DIR_RIGHT;
    data->score = 0;
    data->is_paused = false;
    data->is_game_over = false;

    data->snake[0] = (point_t){ 5, 8 };
    data->snake[1] = (point_t){ 4, 8 };
    data->snake[2] = (point_t){ 3, 8 };

    for (int i = 0; i < SNAKE_MAX_LEN; i++) {
        if (i < 3) {
            lv_obj_clear_flag(data->snake_objs[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(data->snake_objs[i], data->snake[i].x * SNAKE_CELL_PX, data->snake[i].y * SNAKE_CELL_PX);
        } else {
            lv_obj_add_flag(data->snake_objs[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    char sbuf[32];
    snprintf(sbuf, sizeof(sbuf), "SNAKE | Score: %02u", data->score);
    lv_label_set_text(data->score_lbl, sbuf);

    spawn_food(data);
}

static void on_game_pause_action(void)
{
    lv_obj_t *top = lv_scr_act();
    game_screen_data_t *data = (game_screen_data_t *)lv_obj_get_user_data(top);
    if (!data || data->is_game_over) return;

    data->is_paused = !data->is_paused;
    softkey_set_actions(data->is_paused ? "Resume" : "Pause", on_game_pause_action, "Exit", on_game_exit_action);
}

static void on_game_exit_action(void)
{
    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_set_editing(g, false);
    }
    win_mgr_pop();
}

static lv_obj_t *s_game_active_scr = NULL;

bool app_game_is_active(void)
{
    return s_game_active_scr != NULL && lv_obj_is_valid(s_game_active_scr);
}

static void on_game_key_cb(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *scr = lv_obj_get_screen(target);
    game_screen_data_t *data = (game_screen_data_t *)lv_obj_get_user_data(scr);
    if (!data || data->is_paused || data->is_game_over) return;

    if ((key == LV_KEY_UP || key == LV_KEY_PREV || key == '2' || key == 'w' || key == 'W') && data->dir != DIR_DOWN) {
        data->next_dir = DIR_UP;
    } else if ((key == LV_KEY_DOWN || key == LV_KEY_NEXT || key == '8' || key == 's' || key == 'S') && data->dir != DIR_UP) {
        data->next_dir = DIR_DOWN;
    } else if ((key == LV_KEY_LEFT || key == '4' || key == 'a' || key == 'A') && data->dir != DIR_RIGHT) {
        data->next_dir = DIR_LEFT;
    } else if ((key == LV_KEY_RIGHT || key == '6' || key == 'd' || key == 'D') && data->dir != DIR_LEFT) {
        data->next_dir = DIR_RIGHT;
    }
}

static void on_game_delete_cb(lv_event_t *e)
{
    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_set_editing(g, false);
    }
    lv_obj_t *scr = lv_event_get_target(e);
    if (scr == s_game_active_scr) {
        s_game_active_scr = NULL;
    }
    game_screen_data_t *data = (game_screen_data_t *)lv_obj_get_user_data(scr);
    if (data) {
        if (data->game_timer) {
            lv_timer_delete(data->game_timer);
            data->game_timer = NULL;
        }
        free(data);
        lv_obj_set_user_data(scr, NULL);
    }
}

lv_obj_t * app_game_create(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, 176, 220);
    lv_obj_set_style_bg_color(screen, theme_get()->bg_color, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    game_screen_data_t *data = (game_screen_data_t *)calloc(1, sizeof(game_screen_data_t));
    if (!data) {
        lv_obj_del(screen);
        return NULL;
    }

    data->view_type = VEEBHA_VIEW_TYPE_GENERIC;
    strncpy(data->title, "Snake", sizeof(data->title) - 1);
    data->fullscreen_mode = OS_FULLSCREEN_PARTIAL;
    data->show_battery_hud = false;

    lv_obj_set_user_data(screen, data);
    lv_obj_add_event_cb(screen, on_game_delete_cb, LV_EVENT_DELETE, NULL);

    /* 1. Zone A: Fixed 18px Top Status Bar */
    status_bar_create(screen, NULL);

    /* 2. Header Strip (18px) */
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

    data->score_lbl = lv_label_create(hdr);
    lv_label_set_text(data->score_lbl, "SNAKE | Score: 00");
    lv_obj_set_style_text_color(data->score_lbl, theme_get()->accent, 0);
    lv_obj_set_style_text_font(data->score_lbl, &lv_font_montserrat_10, 0);

    /* 3. Viewport (Arena Container) */
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_size(content, lv_pct(100), 0);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 2, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    /* 160x160 Arena Box */
    lv_obj_t *arena = lv_button_create(content);
    data->arena = arena;
    data->first_item = arena;
    lv_obj_set_size(arena, 162, 162);
    lv_obj_set_style_bg_color(arena, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_bg_opa(arena, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(arena, theme_get()->accent, 0);
    lv_obj_set_style_border_width(arena, 1, 0);
    lv_obj_set_style_radius(arena, 2, 0);
    lv_obj_set_style_pad_all(arena, 0, 0);
    lv_obj_remove_flag(arena, LV_OBJ_FLAG_SCROLLABLE);

    /* Create Food Object */
    data->food_obj = lv_obj_create(arena);
    lv_obj_set_size(data->food_obj, 9, 9);
    lv_obj_set_style_bg_color(data->food_obj, lv_color_hex(0xFFC107), 0); /* Amber */
    lv_obj_set_style_bg_opa(data->food_obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(data->food_obj, 4, 0);
    lv_obj_set_style_border_width(data->food_obj, 0, 0);
    lv_obj_remove_flag(data->food_obj, LV_OBJ_FLAG_SCROLLABLE);

    /* Create Snake Segment Objects */
    for (int i = 0; i < SNAKE_MAX_LEN; i++) {
        lv_obj_t *seg = lv_obj_create(arena);
        data->snake_objs[i] = seg;
        lv_obj_set_size(seg, 9, 9);
        lv_obj_set_style_bg_color(seg, (i == 0) ? lv_color_hex(0xFFFFFF) : theme_get()->accent, 0);
        lv_obj_set_style_bg_opa(seg, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(seg, 2, 0);
        lv_obj_set_style_border_width(seg, 0, 0);
        lv_obj_remove_flag(seg, LV_OBJ_FLAG_SCROLLABLE);
    }

    s_game_active_scr = screen;
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, on_game_key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(arena, on_game_key_cb, LV_EVENT_KEY, NULL);

    lv_group_t *grp = win_mgr_get_group();
    if (grp) {
        lv_group_add_obj(grp, arena);
        lv_group_add_obj(grp, screen);
    }
    data->first_item = arena;

    /* 4. Bottom Softkey Bar */
    data->softkey_bar = softkey_bar_create(screen, "Pause", "Exit");
    softkey_set_actions("Pause", on_game_pause_action, "Exit", on_game_exit_action);

    reset_snake_game(data);

    data->game_timer = lv_timer_create(on_game_tick, 140, data);
    return screen;
}

void app_game_init(void)
{
    OS_LOGI(TAG, "Retro Snake Game module initialized");
}

void app_game_open(void)
{
    lv_obj_t *scr = app_game_create();
    if (scr) {
        win_mgr_push(scr, "Pause", on_game_pause_action, "Exit", on_game_exit_action);
        win_mgr_set_fullscreen_mode(scr, OS_FULLSCREEN_PARTIAL, false);
        lv_group_t *g = win_mgr_get_group();
        if (g) {
            game_screen_data_t *d = (game_screen_data_t *)lv_obj_get_user_data(scr);
            if (d && d->arena) {
                lv_group_focus_obj(d->arena);
            } else {
                lv_group_focus_obj(scr);
            }
            lv_group_set_editing(g, true);
        }
    }
}
