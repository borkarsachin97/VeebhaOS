/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "vapp_tetris.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_templates.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_log.h"
#include "sdk/include/veebha_i18n.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "VAPP_TETRIS"

#define TETRIS_INITIAL_SPEED_MS 750
#define TETRIS_MIN_SPEED_MS     250

typedef struct {
    lv_obj_t   *screen;
    lv_obj_t   *arena;
    lv_obj_t   *next_box;
    lv_obj_t   *score_lbl;
    lv_obj_t   *lines_lbl;
    lv_obj_t   *level_lbl;
    lv_obj_t   *over_box;
    lv_obj_t   *over_lbl;
    lv_timer_t *timer;
    uint8_t     grid[16][10];
    int8_t      cur_type;
    int8_t      cur_rot;
    int8_t      cur_x;
    int8_t      cur_y;
    uint8_t     next_type;
    uint32_t    score;
    uint32_t    rendered_score;
    uint16_t    lines;
    uint16_t    rendered_lines;
    uint16_t    level;
    uint16_t    rendered_level;
    bool        game_over;
} tetris_state_t;

static tetris_state_t *s_active_tetris = NULL;

static const uint32_t TETRO_COLORS[8] = {
    0x101726, /* 0: Empty cell background */
    0x00E5FF, /* 1: I - Cyan */
    0xFFD600, /* 2: O - Yellow */
    0xD500F9, /* 3: T - Purple */
    0x00E676, /* 4: S - Emerald Green */
    0xFF1744, /* 5: Z - Bright Red */
    0x2979FF, /* 6: J - Blue */
    0xFF9100  /* 7: L - Orange */
};

static const int8_t TETRO_SHAPES[7][4][4][2] = {
    /* I */ { {{0,1},{1,1},{2,1},{3,1}}, {{2,0},{2,1},{2,2},{2,3}}, {{0,2},{1,2},{2,2},{3,2}}, {{1,0},{1,1},{1,2},{1,3}} },
    /* O */ { {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}} },
    /* T */ { {{1,0},{0,1},{1,1},{2,1}}, {{1,0},{1,1},{2,1},{1,2}}, {{0,1},{1,1},{2,1},{1,2}}, {{1,0},{0,1},{1,1},{1,2}} },
    /* S */ { {{1,0},{2,0},{0,1},{1,1}}, {{1,0},{1,1},{2,1},{2,2}}, {{1,1},{2,1},{0,2},{1,2}}, {{0,0},{0,1},{1,1},{1,2}} },
    /* Z */ { {{0,0},{1,0},{1,1},{2,1}}, {{2,0},{1,1},{2,1},{1,2}}, {{0,1},{1,1},{1,2},{2,2}}, {{1,0},{0,1},{1,1},{0,2}} },
    /* J */ { {{0,0},{0,1},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{1,2}}, {{0,1},{1,1},{2,1},{2,2}}, {{1,0},{1,1},{0,2},{1,2}} },
    /* L */ { {{2,0},{0,1},{1,1},{2,1}}, {{1,0},{1,1},{1,2},{2,2}}, {{0,1},{1,1},{2,1},{0,2}}, {{0,0},{1,0},{1,1},{1,2}} }
};

static bool tetris_collides(uint8_t grid[16][10], int type, int rot, int px, int py)
{
    for (int i = 0; i < 4; i++) {
        int x = px + TETRO_SHAPES[type][rot][i][0];
        int y = py + TETRO_SHAPES[type][rot][i][1];
        if (x < 0 || x >= 10 || y < 0 || y >= 16) return true;
        if (grid[y][x]) return true;
    }
    return false;
}

static void tetris_spawn_piece(tetris_state_t *st)
{
    st->cur_type = st->next_type;
    st->next_type = (uint8_t)(rand() % 7);
    st->cur_rot = 0;
    st->cur_x = 3;
    st->cur_y = 0;
    if (tetris_collides(st->grid, st->cur_type, st->cur_rot, st->cur_x, st->cur_y)) {
        st->game_over = true;
    }
}

static void tetris_render(tetris_state_t *st)
{
    if (!st) return;

    /* 1. Request single-pass blit redraw for arena and next-piece box */
    if (st->arena) {
        lv_obj_invalidate(st->arena);
    }
    if (st->next_box) {
        lv_obj_invalidate(st->next_box);
    }

    /* 2. Update HUD text only on changes */
    if (st->score_lbl && st->rendered_score != st->score) {
        st->rendered_score = st->score;
        char buf[32];
        snprintf(buf, sizeof(buf), "%u", (unsigned int)st->score);
        lv_label_set_text(st->score_lbl, buf);
    }
    if (st->lines_lbl && st->rendered_lines != st->lines) {
        st->rendered_lines = st->lines;
        char buf[32];
        snprintf(buf, sizeof(buf), "%u", (unsigned int)st->lines);
        lv_label_set_text(st->lines_lbl, buf);
    }
    if (st->level_lbl && st->rendered_level != st->level) {
        st->rendered_level = st->level;
        char buf[32];
        snprintf(buf, sizeof(buf), "%u", (unsigned int)st->level);
        lv_label_set_text(st->level_lbl, buf);
    }

    /* 3. Game Over overlay */
    if (st->over_box) {
        if (st->game_over) {
            lv_obj_clear_flag(st->over_box, LV_OBJ_FLAG_HIDDEN);
            if (st->over_lbl) {
                char obuf[64];
                snprintf(obuf, sizeof(obuf), "GAME OVER\n\nScore: %u\n\n[LSK] Retry", (unsigned int)st->score);
                lv_label_set_text(st->over_lbl, obuf);
            }
        } else {
            lv_obj_add_flag(st->over_box, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void tetris_arena_draw_cb(lv_event_t *e)
{
    tetris_state_t *st = (tetris_state_t *)lv_event_get_user_data(e);
    if (!st) return;

    lv_layer_t *layer = lv_event_get_layer(e);
    if (!layer) return;

    lv_obj_t *obj = lv_event_get_target(e);
    lv_area_t ca;
    lv_obj_get_coords(obj, &ca);

    /* 1. Build composite display grid */
    uint8_t display_grid[16][10];
    memcpy(display_grid, st->grid, sizeof(display_grid));
    if (!st->game_over && st->cur_type >= 0 && st->cur_type < 7) {
        for (int i = 0; i < 4; i++) {
            int x = st->cur_x + TETRO_SHAPES[st->cur_type][st->cur_rot][i][0];
            int y = st->cur_y + TETRO_SHAPES[st->cur_type][st->cur_rot][i][1];
            if (x >= 0 && x < 10 && y >= 0 && y < 16) {
                display_grid[y][x] = (uint8_t)(st->cur_type + 1);
            }
        }
    }

    /* 2. Direct blit 16x10 cells */
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_opa = LV_OPA_COVER;
    dsc.radius = 1;
    dsc.border_width = 0;

    for (int r = 0; r < 16; r++) {
        for (int c = 0; c < 10; c++) {
            uint8_t val = display_grid[r][c];
            if (val > 7) val = 1;

            lv_area_t cell_area;
            cell_area.x1 = ca.x1 + 1 + c * 10;
            cell_area.y1 = ca.y1 + 1 + r * 10;
            cell_area.x2 = cell_area.x1 + 9;
            cell_area.y2 = cell_area.y1 + 9;

            if (val == 0) {
                dsc.bg_color = lv_color_hex(0x0E1424);
                dsc.radius = 0;
            } else {
                dsc.bg_color = lv_color_hex(TETRO_COLORS[val]);
                dsc.radius = 1;
            }
            lv_draw_rect(layer, &dsc, &cell_area);
        }
    }
}

static void tetris_next_draw_cb(lv_event_t *e)
{
    tetris_state_t *st = (tetris_state_t *)lv_event_get_user_data(e);
    if (!st) return;

    lv_layer_t *layer = lv_event_get_layer(e);
    if (!layer) return;

    lv_obj_t *obj = lv_event_get_target(e);
    lv_area_t ca;
    lv_obj_get_coords(obj, &ca);

    uint8_t next_grid[4][4];
    memset(next_grid, 0, sizeof(next_grid));
    if (st->next_type < 7) {
        for (int i = 0; i < 4; i++) {
            int nx = TETRO_SHAPES[st->next_type][0][i][0];
            int ny = TETRO_SHAPES[st->next_type][0][i][1];
            if (nx >= 0 && nx < 4 && ny >= 0 && ny < 4) {
                next_grid[ny][nx] = (uint8_t)(st->next_type + 1);
            }
        }
    }

    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_opa = LV_OPA_COVER;
    dsc.radius = 0;
    dsc.border_width = 0;

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            uint8_t nval = next_grid[r][c];
            uint32_t col = (nval == 0) ? 0x0E1424 : TETRO_COLORS[nval];
            dsc.bg_color = lv_color_hex(col);

            lv_area_t cell_area;
            cell_area.x1 = ca.x1 + 2 + c * 7;
            cell_area.y1 = ca.y1 + 2 + r * 7;
            cell_area.x2 = cell_area.x1 + 5;
            cell_area.y2 = cell_area.y1 + 5;
            lv_draw_rect(layer, &dsc, &cell_area);
        }
    }
}

static void tetris_update_speed(tetris_state_t *st)
{
    if (!st || !st->timer) return;
    uint32_t speed = (st->level <= 15) ? (TETRIS_INITIAL_SPEED_MS - (st->level - 1) * 35) : TETRIS_MIN_SPEED_MS;
    if (speed < TETRIS_MIN_SPEED_MS) speed = TETRIS_MIN_SPEED_MS;
    lv_timer_set_period(st->timer, speed);
}

static void tetris_clear_lines(tetris_state_t *st)
{
    int cleared = 0;
    for (int y = 15; y >= 0; y--) {
        bool full = true;
        for (int x = 0; x < 10; x++) {
            if (!st->grid[y][x]) { full = false; break; }
        }
        if (full) {
            cleared++;
            for (int k = y; k > 0; k--) {
                for (int x = 0; x < 10; x++) {
                    st->grid[k][x] = st->grid[k - 1][x];
                }
            }
            for (int x = 0; x < 10; x++) st->grid[0][x] = 0;
            y++;
        }
    }
    if (cleared > 0) {
        st->lines += cleared;
        st->score += (cleared == 1) ? 100 : (cleared == 2) ? 300 : (cleared == 3) ? 500 : 800;
        st->level = 1 + (st->lines / 10);
        tetris_update_speed(st);
    }
}

static void tetris_lock_piece(tetris_state_t *st)
{
    for (int i = 0; i < 4; i++) {
        int x = st->cur_x + TETRO_SHAPES[st->cur_type][st->cur_rot][i][0];
        int y = st->cur_y + TETRO_SHAPES[st->cur_type][st->cur_rot][i][1];
        if (x >= 0 && x < 10 && y >= 0 && y < 16) {
            st->grid[y][x] = (uint8_t)(st->cur_type + 1);
        }
    }
    st->score += 10;
    tetris_clear_lines(st);
    tetris_spawn_piece(st);
}

static void tetris_timer_cb(lv_timer_t *t)
{
    tetris_state_t *st = (tetris_state_t *)lv_timer_get_user_data(t);
    if (!st || st->game_over) return;

    if (!tetris_collides(st->grid, st->cur_type, st->cur_rot, st->cur_x, st->cur_y + 1)) {
        st->cur_y++;
    } else {
        tetris_lock_piece(st);
    }
    tetris_render(st);
}

static void tetris_reset_game(tetris_state_t *st)
{
    memset(st->grid, 0, sizeof(st->grid));
    st->rendered_score = 0xFFFFFFFF;
    st->rendered_lines = 0xFFFF;
    st->rendered_level = 0xFFFF;
    st->score = 0;
    st->lines = 0;
    st->level = 1;
    st->game_over = false;
    st->next_type = (uint8_t)(rand() % 7);
    tetris_update_speed(st);
    tetris_spawn_piece(st);
    tetris_render(st);
}

static void tetris_key_cb(lv_event_t *e)
{
    tetris_state_t *st = (tetris_state_t *)lv_event_get_user_data(e);
    if (!st) return;

    uint32_t key = lv_event_get_key(e);
    if (st->game_over) {
        if (key == LV_KEY_ENTER || key == '5' || key == '1' || key == ' ' || key == LV_KEY_UP || key == LV_KEY_DOWN) {
            tetris_reset_game(st);
        }
    } else {
        if (key == LV_KEY_LEFT || key == LV_KEY_PREV || key == '4' || key == 'a' || key == 'A') {
            if (!tetris_collides(st->grid, st->cur_type, st->cur_rot, st->cur_x - 1, st->cur_y)) {
                st->cur_x--;
            }
        } else if (key == LV_KEY_RIGHT || key == LV_KEY_NEXT || key == '6' || key == 'd' || key == 'D') {
            if (!tetris_collides(st->grid, st->cur_type, st->cur_rot, st->cur_x + 1, st->cur_y)) {
                st->cur_x++;
            }
        } else if (key == LV_KEY_UP || key == '2' || key == '5' || key == 'w' || key == 'W' || key == LV_KEY_ENTER) {
            int next_rot = (st->cur_rot + 1) % 4;
            if (!tetris_collides(st->grid, st->cur_type, next_rot, st->cur_x, st->cur_y)) {
                st->cur_rot = (int8_t)next_rot;
            }
        } else if (key == LV_KEY_DOWN || key == '8' || key == 's' || key == 'S') {
            if (!tetris_collides(st->grid, st->cur_type, st->cur_rot, st->cur_x, st->cur_y + 1)) {
                st->cur_y++;
                st->score += 1;
            }
        } else if (key == '0' || key == '#' || key == ' ') {
            while (!tetris_collides(st->grid, st->cur_type, st->cur_rot, st->cur_x, st->cur_y + 1)) {
                st->cur_y++;
                st->score += 2;
            }
            tetris_lock_piece(st);
        }
    }
    tetris_render(st);
}

static void tetris_screen_delete_cb(lv_event_t *e)
{
    (void)e;
    tetris_state_t *st = (tetris_state_t *)lv_event_get_user_data(e);
    if (!st) return;

    if (s_active_tetris == st) {
        s_active_tetris = NULL;
    }

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

static void on_tetris_restart_action(void)
{
    if (s_active_tetris) {
        tetris_reset_game(s_active_tetris);
    }
}

static void on_tetris_exit_action(void)
{
    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_set_editing(g, false);
    }
    win_mgr_pop();
}

void vapp_tetris_launch(const vapp_package_t *pkg)
{
    tetris_state_t *st = (tetris_state_t *)calloc(1, sizeof(tetris_state_t));
    if (!st) return;

    s_active_tetris = st;
    st->level = 1;
    st->rendered_score = 0xFFFFFFFF;
    st->rendered_lines = 0xFFFF;
    st->rendered_level = 0xFFFF;
    st->next_type = (uint8_t)(rand() % 7);
    tetris_spawn_piece(st);

    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, 176, 220);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x060912), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)calloc(1, sizeof(win_mgr_screen_hdr_t));
    if (hdr) {
        hdr->view_type = VEEBHA_VIEW_TYPE_GENERIC;
        hdr->fullscreen_mode = OS_FULLSCREEN_FULL;
        strncpy(hdr->title, pkg ? pkg->header.name : "Tetris", sizeof(hdr->title) - 1);
        lv_obj_set_user_data(screen, hdr);
    }
    st->screen = screen;
    lv_obj_add_event_cb(screen, tetris_screen_delete_cb, LV_EVENT_DELETE, st);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, tetris_key_cb, LV_EVENT_KEY, st);

    /* Main Row Viewport (176x220 px fullscreen) */
    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_size(content, lv_pct(100), 0);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_color(content, lv_color_hex(0x080C18), 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_hor(content, 3, 0);
    lv_obj_set_style_pad_ver(content, 2, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    /* 1. Left Arena Container (102x162 px) */
    lv_obj_t *arena = lv_button_create(content);
    st->arena = arena;
    if (hdr) hdr->first_item = arena;
    lv_obj_set_size(arena, 102, 162);
    lv_obj_set_style_bg_color(arena, lv_color_hex(0x0A0F1D), 0);
    lv_obj_set_style_border_color(arena, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_border_width(arena, 1, 0);
    lv_obj_set_style_pad_all(arena, 0, 0);
    lv_obj_remove_flag(arena, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(arena, tetris_arena_draw_cb, LV_EVENT_DRAW_POST, st);
    lv_obj_add_event_cb(arena, tetris_key_cb, LV_EVENT_KEY, st);

    /* Game Over Modal Overlay */
    lv_obj_t *over_box = lv_obj_create(arena);
    st->over_box = over_box;
    lv_obj_set_size(over_box, 90, 80);
    lv_obj_set_pos(over_box, 5, 40);
    lv_obj_set_style_bg_color(over_box, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(over_box, LV_OPA_80, 0);
    lv_obj_set_style_border_color(over_box, lv_color_hex(0xFF1744), 0);
    lv_obj_set_style_border_width(over_box, 1, 0);
    lv_obj_set_style_pad_all(over_box, 4, 0);
    lv_obj_set_flex_flow(over_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(over_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(over_box, LV_OBJ_FLAG_HIDDEN);

    st->over_lbl = lv_label_create(over_box);
    lv_obj_set_style_text_color(st->over_lbl, lv_color_hex(0xFF1744), 0);
    lv_obj_set_style_text_font(st->over_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_align(st->over_lbl, LV_TEXT_ALIGN_CENTER, 0);

    /* 2. Right Side Panel (64x162 px) */
    lv_obj_t *side = lv_obj_create(content);
    lv_obj_set_size(side, 64, 162);
    lv_obj_set_style_bg_color(side, lv_color_hex(0x0D1424), 0);
    lv_obj_set_style_border_color(side, lv_color_hex(0x1B2A4A), 0);
    lv_obj_set_style_border_width(side, 1, 0);
    lv_obj_set_style_pad_all(side, 2, 0);
    lv_obj_set_flex_flow(side, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(side, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(side, LV_OBJ_FLAG_SCROLLABLE);

    /* NEXT Box */
    lv_obj_t *nlbl = lv_label_create(side);
    lv_label_set_text(nlbl, "NEXT");
    lv_obj_set_style_text_color(nlbl, lv_color_hex(0x8A99AD), 0);
    lv_obj_set_style_text_font(nlbl, &lv_font_montserrat_10, 0);

    lv_obj_t *next_box = lv_obj_create(side);
    st->next_box = next_box;
    lv_obj_set_size(next_box, 32, 32);
    lv_obj_set_style_bg_color(next_box, lv_color_hex(0x080D18), 0);
    lv_obj_set_style_border_color(next_box, lv_color_hex(0x223254), 0);
    lv_obj_set_style_border_width(next_box, 1, 0);
    lv_obj_set_style_pad_all(next_box, 0, 0);
    lv_obj_remove_flag(next_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(next_box, tetris_next_draw_cb, LV_EVENT_DRAW_POST, st);

    /* SCORE */
    lv_obj_t *slbl = lv_label_create(side);
    lv_label_set_text(slbl, "SCORE");
    lv_obj_set_style_text_color(slbl, lv_color_hex(0x8A99AD), 0);
    lv_obj_set_style_text_font(slbl, &lv_font_montserrat_10, 0);

    st->score_lbl = lv_label_create(side);
    lv_obj_set_style_text_color(st->score_lbl, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(st->score_lbl, &lv_font_montserrat_10, 0);
    lv_label_set_text(st->score_lbl, "0");

    /* LINES */
    lv_obj_t *llbl = lv_label_create(side);
    lv_label_set_text(llbl, "LINES");
    lv_obj_set_style_text_color(llbl, lv_color_hex(0x8A99AD), 0);
    lv_obj_set_style_text_font(llbl, &lv_font_montserrat_10, 0);

    st->lines_lbl = lv_label_create(side);
    lv_obj_set_style_text_color(st->lines_lbl, lv_color_hex(0x00FF88), 0);
    lv_obj_set_style_text_font(st->lines_lbl, &lv_font_montserrat_10, 0);
    lv_label_set_text(st->lines_lbl, "0");

    /* LEVEL */
    lv_obj_t *lvlbl = lv_label_create(side);
    lv_label_set_text(lvlbl, "LVL");
    lv_obj_set_style_text_color(lvlbl, lv_color_hex(0x8A99AD), 0);
    lv_obj_set_style_text_font(lvlbl, &lv_font_montserrat_10, 0);

    st->level_lbl = lv_label_create(side);
    lv_obj_set_style_text_color(st->level_lbl, lv_color_hex(0xFFD600), 0);
    lv_obj_set_style_text_font(st->level_lbl, &lv_font_montserrat_10, 0);
    lv_label_set_text(st->level_lbl, "1");

    tetris_render(st);

    /* Falling timer: starts at 750 ms (slower fall speed) */
    st->timer = lv_timer_create(tetris_timer_cb, TETRIS_INITIAL_SPEED_MS, st);

    lv_obj_t *sk = softkey_bar_create(screen, "Restart", "Exit");
    if (hdr) hdr->softkey_bar = sk;

    win_mgr_push(screen, "Restart", on_tetris_restart_action, "Exit", on_tetris_exit_action);

    /* Add arena to group and enable editing mode for direct D-pad & numpad control */
    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_add_obj(g, arena);
        lv_group_focus_obj(arena);
        lv_group_set_editing(g, true);
    }
}
