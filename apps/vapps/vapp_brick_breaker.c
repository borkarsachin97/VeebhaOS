/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "vapp_brick_breaker.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_templates.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_log.h"
#include "sdk/include/veebha_i18n.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "VAPP_BRICK"

typedef struct {
    lv_obj_t   *screen;
    lv_obj_t   *playfield;
    lv_obj_t   *paddle_obj;
    lv_obj_t   *ball_obj;
    lv_obj_t   *brick_objs[18]; /* 3 rows x 6 cols */
    lv_obj_t   *score_lbl;
    lv_obj_t   *lives_lbl;
    lv_timer_t *timer;
    int16_t     paddle_x;
    int16_t     ball_x;
    int16_t     ball_y;
    int16_t     ball_dx;
    int16_t     ball_dy;
    uint16_t    score;
    uint16_t    rendered_score;
    uint8_t     lives;
    uint8_t     rendered_lives;
    bool        bricks[18];
    int8_t      rendered_bricks[18];
    int16_t     rendered_paddle_x;
} brick_game_state_t;

static void brick_game_render(brick_game_state_t *st)
{
    if (!st) return;
    if (st->paddle_obj && st->rendered_paddle_x != st->paddle_x) {
        st->rendered_paddle_x = st->paddle_x;
        lv_obj_set_pos(st->paddle_obj, st->paddle_x, 136);
    }
    if (st->ball_obj) {
        lv_obj_set_pos(st->ball_obj, st->ball_x, st->ball_y);
    }
    for (int i = 0; i < 18; i++) {
        if (st->brick_objs[i] && st->rendered_bricks[i] != st->bricks[i]) {
            st->rendered_bricks[i] = st->bricks[i];
            if (st->bricks[i]) {
                lv_obj_clear_flag(st->brick_objs[i], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(st->brick_objs[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
    if (st->score_lbl && st->rendered_score != st->score) {
        st->rendered_score = st->score;
        char buf[32];
        snprintf(buf, sizeof(buf), "SCORE: %04u", st->score);
        lv_label_set_text(st->score_lbl, buf);
    }
    if (st->lives_lbl && st->rendered_lives != st->lives) {
        st->rendered_lives = st->lives;
        char buf[32];
        snprintf(buf, sizeof(buf), "LIVES: %u", st->lives);
        lv_label_set_text(st->lives_lbl, buf);
    }
}

static void brick_game_reset_field(brick_game_state_t *st)
{
    st->paddle_x = 65;
    st->rendered_paddle_x = -1;
    st->ball_x = 80;
    st->ball_y = 70;
    st->ball_dx = 3;
    st->ball_dy = 2;
    st->rendered_score = 0xFFFF;
    st->rendered_lives = 0xFF;
    memset(st->rendered_bricks, 0xFF, sizeof(st->rendered_bricks));
    for (int i = 0; i < 18; i++) {
        st->bricks[i] = true;
    }
    brick_game_render(st);
}

static void brick_game_timer_cb(lv_timer_t *t)
{
    brick_game_state_t *st = (brick_game_state_t *)lv_timer_get_user_data(t);
    if (!st) return;

    st->ball_x += st->ball_dx;
    st->ball_y += st->ball_dy;

    /* Wall bounds */
    if (st->ball_x <= 2) {
        st->ball_x = 2;
        st->ball_dx = -st->ball_dx;
    } else if (st->ball_x >= 158) {
        st->ball_x = 158;
        st->ball_dx = -st->ball_dx;
    }
    if (st->ball_y <= 2) {
        st->ball_y = 2;
        st->ball_dy = -st->ball_dy;
    }

    /* Brick collisions: 3 rows (y: 10, 22, 34) x 6 cols (x: 4 + c*27) */
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 6; c++) {
            int idx = r * 6 + c;
            if (!st->bricks[idx]) continue;
            int bx = 4 + c * 27;
            int by = 10 + r * 12;
            if (st->ball_x + 6 >= bx && st->ball_x <= bx + 24 &&
                st->ball_y + 6 >= by && st->ball_y <= by + 8) {
                st->bricks[idx] = false;
                st->ball_dy = -st->ball_dy;
                st->score += (3 - r) * 10;
                break;
            }
        }
    }

    /* Paddle collision */
    if (st->ball_y >= 130 && st->ball_y <= 138) {
        if (st->ball_x + 6 >= st->paddle_x - 2 && st->ball_x <= st->paddle_x + 38) {
            st->ball_dy = -abs(st->ball_dy);
            st->score += 5;
        }
    } else if (st->ball_y >= 144) {
        /* Ball dropped */
        if (st->lives > 1) {
            st->lives--;
            st->ball_x = st->paddle_x + 15;
            st->ball_y = 80;
            st->ball_dy = 2;
        } else {
            /* Game over / reset */
            st->lives = 3;
            st->score = 0;
            brick_game_reset_field(st);
        }
    }

    brick_game_render(st);
}

static void brick_game_key_cb(lv_event_t *e)
{
    brick_game_state_t *st = (brick_game_state_t *)lv_event_get_user_data(e);
    if (!st) return;

    uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_LEFT || key == LV_KEY_PREV || key == '4' || key == 'a' || key == 'A') {
        st->paddle_x = (st->paddle_x > 8) ? (st->paddle_x - 8) : 4;
    } else if (key == LV_KEY_RIGHT || key == LV_KEY_NEXT || key == '6' || key == 'd' || key == 'D') {
        st->paddle_x = (st->paddle_x < 124) ? (st->paddle_x + 8) : 126;
    } else if (key == LV_KEY_ENTER || key == '5' || key == '1' || key == ' ') {
        st->score += 5;
    }
    brick_game_render(st);
}

static void brick_game_screen_delete_cb(lv_event_t *e)
{
    (void)e;
    brick_game_state_t *st = (brick_game_state_t *)lv_event_get_user_data(e);
    if (!st) return;

    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_set_editing(g, false);
    }
    if (st->timer) {
        lv_timer_delete(st->timer);
        st->timer = NULL;
    }
    win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)lv_obj_get_user_data(st->screen);
    if (hdr) {
        free(hdr);
        lv_obj_set_user_data(st->screen, NULL);
    }
    free(st);
}

static void on_brick_restart_action(void)
{
    lv_obj_t *top = lv_scr_act();
    brick_game_state_t *st = (brick_game_state_t *)lv_obj_get_user_data(top);
    if (!st) return;
    st->lives = 3;
    st->score = 0;
    brick_game_reset_field(st);
}

static void on_brick_exit_action(void)
{
    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_set_editing(g, false);
    }
    win_mgr_pop();
}

void vapp_brick_breaker_launch(const vapp_package_t *pkg)
{
    brick_game_state_t *st = (brick_game_state_t *)calloc(1, sizeof(brick_game_state_t));
    if (!st) return;

    st->paddle_x = 65;
    st->ball_x = 80;
    st->ball_y = 70;
    st->ball_dx = 3;
    st->ball_dy = 2;
    st->lives = 3;
    st->score = 0;
    st->rendered_paddle_x = -1;
    st->rendered_score = 0xFFFF;
    st->rendered_lives = 0xFF;
    memset(st->rendered_bricks, 0xFF, sizeof(st->rendered_bricks));
    for (int i = 0; i < 18; i++) st->bricks[i] = true;

    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, 176, 220);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x050B14), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)calloc(1, sizeof(win_mgr_screen_hdr_t));
    if (hdr) {
        hdr->view_type = VEEBHA_VIEW_TYPE_GENERIC;
        strncpy(hdr->title, pkg ? pkg->header.name : "Brick Breaker", sizeof(hdr->title) - 1);
        lv_obj_set_user_data(screen, hdr);
    }

    st->screen = screen;
    lv_obj_add_event_cb(screen, brick_game_screen_delete_cb, LV_EVENT_DELETE, st);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, brick_game_key_cb, LV_EVENT_KEY, st);

    /* Top HUD Strip (18px) */
    lv_obj_t *hud = lv_obj_create(screen);
    lv_obj_set_size(hud, lv_pct(100), 18);
    lv_obj_set_style_bg_color(hud, lv_color_hex(0x0A1224), 0);
    lv_obj_set_style_border_side(hud, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(hud, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_border_width(hud, 1, 0);
    lv_obj_set_style_pad_hor(hud, 6, 0);
    lv_obj_set_style_pad_ver(hud, 0, 0);
    lv_obj_set_flex_flow(hud, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hud, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    st->score_lbl = lv_label_create(hud);
    lv_obj_set_style_text_color(st->score_lbl, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(st->score_lbl, &lv_font_montserrat_10, 0);
    lv_label_set_text(st->score_lbl, "SCORE: 0000");

    st->lives_lbl = lv_label_create(hud);
    lv_obj_set_style_text_color(st->lives_lbl, lv_color_hex(0xFF1744), 0);
    lv_obj_set_style_text_font(st->lives_lbl, &lv_font_montserrat_10, 0);
    lv_label_set_text(st->lives_lbl, "LIVES: 3");

    /* Graphical Playfield Viewport (168x150 px) */
    lv_obj_t *playfield = lv_button_create(screen);
    st->playfield = playfield;
    if (hdr) hdr->first_item = playfield;
    lv_obj_set_size(playfield, 168, 150);
    lv_obj_set_style_bg_color(playfield, lv_color_hex(0x090E1A), 0);
    lv_obj_set_style_border_color(playfield, lv_color_hex(0x1B2A4A), 0);
    lv_obj_set_style_border_width(playfield, 1, 0);
    lv_obj_set_style_pad_all(playfield, 0, 0);
    lv_obj_remove_flag(playfield, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(playfield, brick_game_key_cb, LV_EVENT_KEY, st);

    /* 18 Bricks (3 rows x 6 cols) */
    uint32_t row_colors[3] = { 0xFF1744, 0xFF9100, 0x00E676 };
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 6; c++) {
            int idx = r * 6 + c;
            lv_obj_t *brk = lv_obj_create(playfield);
            st->brick_objs[idx] = brk;
            lv_obj_set_size(brk, 24, 8);
            lv_obj_set_pos(brk, 4 + c * 27, 10 + r * 12);
            lv_obj_set_style_bg_color(brk, lv_color_hex(row_colors[r]), 0);
            lv_obj_set_style_bg_opa(brk, LV_OPA_COVER, 0);
            lv_obj_set_style_radius(brk, 1, 0);
            lv_obj_set_style_border_width(brk, 0, 0);
            lv_obj_remove_flag(brk, LV_OBJ_FLAG_SCROLLABLE);
        }
    }

    /* Paddle */
    lv_obj_t *paddle = lv_obj_create(playfield);
    st->paddle_obj = paddle;
    lv_obj_set_size(paddle, 36, 6);
    lv_obj_set_pos(paddle, st->paddle_x, 136);
    lv_obj_set_style_bg_color(paddle, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_bg_opa(paddle, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(paddle, 3, 0);
    lv_obj_set_style_border_width(paddle, 0, 0);
    lv_obj_remove_flag(paddle, LV_OBJ_FLAG_SCROLLABLE);

    /* Ball */
    lv_obj_t *ball = lv_obj_create(playfield);
    st->ball_obj = ball;
    lv_obj_set_size(ball, 6, 6);
    lv_obj_set_pos(ball, st->ball_x, st->ball_y);
    lv_obj_set_style_bg_color(ball, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(ball, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ball, 3, 0);
    lv_obj_set_style_border_width(ball, 0, 0);
    lv_obj_remove_flag(ball, LV_OBJ_FLAG_SCROLLABLE);

    brick_game_render(st);

    st->timer = lv_timer_create(brick_game_timer_cb, 50, st);

    lv_obj_t *sk = softkey_bar_create(screen, "Restart", "Exit");
    if (hdr) hdr->softkey_bar = sk;

    win_mgr_push(screen, "Restart", on_brick_restart_action, "Exit", on_brick_exit_action);

    /* Add playfield to group and enable editing mode for direct D-pad & numpad control */
    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_add_obj(g, playfield);
        lv_group_focus_obj(playfield);
        lv_group_set_editing(g, true);
    }
}
