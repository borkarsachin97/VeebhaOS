/*
 * VeebhaOS - Space Shooter VAPP Game
 * High-performance 2D Arcade Space Combat
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "vapp_space_shooter.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_templates.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_log.h"
#include "sdk/include/veebha_i18n.h"
#include "boards/board_config.h"
#include "third_party/lvgl/lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "VAPP_SHOOTER"

#define MAX_STARS       24
#define MAX_LASERS      6
#define MAX_ENEMIES     10
#define MAX_PARTICLES   16

#define PLAYFIELD_W     176
#define PLAYFIELD_H     204
#define PLAYER_W        14
#define PLAYER_H        10
#define ENEMY_W         12
#define ENEMY_H         8

typedef struct {
    int16_t x;
    int16_t y;
    int8_t  speed;
    uint8_t brightness;
} star_t;

typedef struct {
    int16_t x;
    int16_t y;
    bool    active;
} laser_t;

typedef struct {
    int16_t x;
    int16_t y;
    int8_t  dx;
    bool    alive;
    uint8_t row;
} enemy_t;

typedef struct {
    int16_t x;
    int16_t y;
    int8_t  dx;
    int8_t  dy;
    int8_t  life;
    uint32_t color;
} particle_t;

typedef struct {
    lv_obj_t   *screen;
    lv_obj_t   *playfield;
    lv_obj_t   *score_lbl;
    lv_obj_t   *wave_lbl;
    lv_obj_t   *lives_lbl;
    lv_timer_t *timer;

    int16_t     player_x;
    int16_t     player_y;
    int8_t      player_vx;
    uint8_t     lives;
    uint16_t    score;
    uint8_t     wave;
    bool        game_over;

    star_t      stars[MAX_STARS];
    laser_t     lasers[MAX_LASERS];
    enemy_t     enemies[MAX_ENEMIES];
    particle_t  particles[MAX_PARTICLES];

    uint16_t    rendered_score;
    uint8_t     rendered_lives;
    uint8_t     rendered_wave;
} shooter_state_t;

static void on_shooter_restart_action(void);
static void on_shooter_exit_action(void);
static void space_shooter_init_wave(shooter_state_t *st);

static void draw_fill_rect(lv_layer_t *layer, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = lv_color_hex(color);
    dsc.bg_opa = LV_OPA_COVER;
    dsc.radius = 0;
    dsc.border_width = 0;

    lv_area_t a;
    a.x1 = x;
    a.y1 = y;
    a.x2 = x + w - 1;
    a.y2 = y + h - 1;
    lv_draw_rect(layer, &dsc, &a);
}

static void space_shooter_spawn_particles(shooter_state_t *st, int16_t x, int16_t y, uint32_t color)
{
    int count = 0;
    for (int i = 0; i < MAX_PARTICLES && count < 4; i++) {
        if (st->particles[i].life <= 0) {
            st->particles[i].x = x;
            st->particles[i].y = y;
            st->particles[i].dx = (count == 0) ? -2 : (count == 1) ? 2 : (count == 2) ? -1 : 1;
            st->particles[i].dy = (count == 0) ? -1 : (count == 1) ? -2 : (count == 2) ? 2 : 1;
            st->particles[i].life = 8;
            st->particles[i].color = color;
            count++;
        }
    }
}

static void space_shooter_draw_cb(lv_event_t *e)
{
    shooter_state_t *st = (shooter_state_t *)lv_event_get_user_data(e);
    if (!st) return;

    lv_layer_t *layer = lv_event_get_layer(e);
    if (!layer) return;

    lv_obj_t *obj = lv_event_get_target(e);
    lv_area_t ca;
    lv_obj_get_coords(obj, &ca);

    /* 1. Draw Starfield */
    for (int i = 0; i < MAX_STARS; i++) {
        int32_t sx = ca.x1 + st->stars[i].x;
        int32_t sy = ca.y1 + st->stars[i].y;
        uint32_t scolor = (st->stars[i].brightness == 1) ? 0x4B5563 :
                          (st->stars[i].brightness == 2) ? 0x9CA3AF : 0xFFFFFF;
        draw_fill_rect(layer, sx, sy, 1, 1, scolor);
    }

    /* 2. Draw Lasers */
    for (int i = 0; i < MAX_LASERS; i++) {
        if (st->lasers[i].active) {
            int32_t lx = ca.x1 + st->lasers[i].x;
            int32_t ly = ca.y1 + st->lasers[i].y;
            draw_fill_rect(layer, lx, ly, 2, 6, 0x00E5FF);
        }
    }

    /* 3. Draw Enemies */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (st->enemies[i].alive) {
            int32_t ex = ca.x1 + st->enemies[i].x;
            int32_t ey = ca.y1 + st->enemies[i].y;
            uint32_t ecolor = (st->enemies[i].row == 0) ? 0xFF1744 : 0xFF9100;

            /* Alien sprite composed of 3 rectangles */
            draw_fill_rect(layer, ex + 2, ey, ENEMY_W - 4, ENEMY_H, ecolor);
            draw_fill_rect(layer, ex, ey + 2, ENEMY_W, ENEMY_H - 4, ecolor);
            /* Eye highlights */
            draw_fill_rect(layer, ex + 3, ey + 3, 2, 2, 0xFFFFFF);
            draw_fill_rect(layer, ex + 7, ey + 3, 2, 2, 0xFFFFFF);
        }
    }

    /* 4. Draw Particles */
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (st->particles[i].life > 0) {
            int32_t px = ca.x1 + st->particles[i].x;
            int32_t py = ca.y1 + st->particles[i].y;
            draw_fill_rect(layer, px, py, 2, 2, st->particles[i].color);
        }
    }

    /* 5. Draw Player Starfighter (if not game over or blinking) */
    if (!st->game_over || (st->game_over && st->lives > 0)) {
        int32_t px = ca.x1 + st->player_x;
        int32_t py = ca.y1 + st->player_y;

        /* Wings */
        draw_fill_rect(layer, px, py + 4, PLAYER_W, 3, 0x2979FF);
        /* Main Fuselage */
        draw_fill_rect(layer, px + 5, py, 4, PLAYER_H, 0x00E5FF);
        /* Cockpit */
        draw_fill_rect(layer, px + 6, py + 2, 2, 3, 0xFFD600);
        /* Thrusters */
        draw_fill_rect(layer, px + 5, py + PLAYER_H, 4, 2, 0xFF3D00);
    }

    /* 6. Game Over Banner Overlay */
    if (st->game_over) {
        int32_t mx = ca.x1 + 18;
        int32_t my = ca.y1 + 70;
        draw_fill_rect(layer, mx, my, 140, 50, 0x000000);
        /* Outer border */
        draw_fill_rect(layer, mx, my, 140, 1, 0xFF1744);
        draw_fill_rect(layer, mx, my + 49, 140, 1, 0xFF1744);
        draw_fill_rect(layer, mx, my, 1, 50, 0xFF1744);
        draw_fill_rect(layer, mx + 139, my, 1, 50, 0xFF1744);
    }
}

static void space_shooter_fire_laser(shooter_state_t *st)
{
    if (st->game_over) return;
    for (int i = 0; i < MAX_LASERS; i++) {
        if (!st->lasers[i].active) {
            st->lasers[i].active = true;
            st->lasers[i].x = st->player_x + (PLAYER_W / 2) - 1;
            st->lasers[i].y = st->player_y - 6;
            break;
        }
    }
}

static void space_shooter_init_wave(shooter_state_t *st)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        int row = i / 5;
        int col = i % 5;
        st->enemies[i].x = 18 + col * 28;
        st->enemies[i].y = 10 + row * 18;
        st->enemies[i].dx = 1 + (st->wave / 3);
        st->enemies[i].alive = true;
        st->enemies[i].row = (uint8_t)row;
    }
}

static void space_shooter_reset_game(shooter_state_t *st)
{
    st->player_x = (PLAYFIELD_W / 2) - (PLAYER_W / 2);
    st->player_y = PLAYFIELD_H - 18;
    st->player_vx = 0;
    st->lives = 3;
    st->score = 0;
    st->wave = 1;
    st->game_over = false;
    st->rendered_score = 0xFFFF;
    st->rendered_lives = 0xFF;
    st->rendered_wave = 0xFF;

    for (int i = 0; i < MAX_LASERS; i++) st->lasers[i].active = false;
    for (int i = 0; i < MAX_PARTICLES; i++) st->particles[i].life = 0;

    /* Initialize stars */
    for (int i = 0; i < MAX_STARS; i++) {
        st->stars[i].x = rand() % PLAYFIELD_W;
        st->stars[i].y = rand() % PLAYFIELD_H;
        st->stars[i].speed = 1 + (rand() % 3);
        st->stars[i].brightness = (uint8_t)(1 + (rand() % 3));
    }

    space_shooter_init_wave(st);
}

static void space_shooter_timer_cb(lv_timer_t *t)
{
    shooter_state_t *st = (shooter_state_t *)lv_timer_get_user_data(t);
    if (!st) return;

    /* 1. Update Starfield */
    for (int i = 0; i < MAX_STARS; i++) {
        st->stars[i].y += st->stars[i].speed;
        if (st->stars[i].y >= PLAYFIELD_H) {
            st->stars[i].y = 0;
            st->stars[i].x = rand() % PLAYFIELD_W;
        }
    }

    /* 2. Update Particles */
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (st->particles[i].life > 0) {
            st->particles[i].x += st->particles[i].dx;
            st->particles[i].y += st->particles[i].dy;
            st->particles[i].life--;
        }
    }

    if (!st->game_over) {
        /* 3. Update Player Position */
        st->player_x += st->player_vx;
        if (st->player_x < 2) st->player_x = 2;
        if (st->player_x > PLAYFIELD_W - PLAYER_W - 2) st->player_x = PLAYFIELD_W - PLAYER_W - 2;
        st->player_vx = 0;

        /* 4. Update Lasers */
        for (int i = 0; i < MAX_LASERS; i++) {
            if (st->lasers[i].active) {
                st->lasers[i].y -= 6;
                if (st->lasers[i].y < -6) {
                    st->lasers[i].active = false;
                }
            }
        }

        /* 5. Update Enemies */
        bool hit_wall = false;
        int active_count = 0;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (st->enemies[i].alive) {
                active_count++;
                st->enemies[i].x += st->enemies[i].dx;
                if (st->enemies[i].x <= 2 || st->enemies[i].x >= PLAYFIELD_W - ENEMY_W - 2) {
                    hit_wall = true;
                }
            }
        }

        if (hit_wall) {
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (st->enemies[i].alive) {
                    st->enemies[i].dx = -st->enemies[i].dx;
                    st->enemies[i].y += 6;
                    if (st->enemies[i].y >= st->player_y - ENEMY_H) {
                        /* Alien breached defenses */
                        st->lives--;
                        if (st->lives == 0) {
                            st->game_over = true;
                        } else {
                            space_shooter_init_wave(st);
                        }
                        break;
                    }
                }
            }
        }

        /* Next wave when all enemies defeated */
        if (active_count == 0) {
            st->wave++;
            st->score += 150 * st->wave;
            space_shooter_init_wave(st);
        }

        /* 6. Check Laser vs Enemy Collisions */
        for (int l = 0; l < MAX_LASERS; l++) {
            if (!st->lasers[l].active) continue;
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (!st->enemies[e].alive) continue;
                if (st->lasers[l].x + 2 >= st->enemies[e].x &&
                    st->lasers[l].x <= st->enemies[e].x + ENEMY_W &&
                    st->lasers[l].y <= st->enemies[e].y + ENEMY_H &&
                    st->lasers[l].y + 6 >= st->enemies[e].y) {
                    /* Hit! */
                    st->enemies[e].alive = false;
                    st->lasers[l].active = false;
                    st->score += (st->enemies[e].row == 0 ? 30 : 20) * st->wave;
                    space_shooter_spawn_particles(st, st->enemies[e].x + 6, st->enemies[e].y + 4, 0xFF9100);
                    break;
                }
            }
        }
    }

    /* 7. Update HUD Labels */
    if (st->score_lbl && st->rendered_score != st->score) {
        st->rendered_score = st->score;
        char buf[32];
        snprintf(buf, sizeof(buf), "PTS: %04u", (unsigned int)st->score);
        lv_label_set_text(st->score_lbl, buf);
    }
    if (st->wave_lbl && st->rendered_wave != st->wave) {
        st->rendered_wave = st->wave;
        char buf[32];
        snprintf(buf, sizeof(buf), "WAVE: %u", (unsigned int)st->wave);
        lv_label_set_text(st->wave_lbl, buf);
    }
    if (st->lives_lbl && st->rendered_lives != st->lives) {
        st->rendered_lives = st->lives;
        char buf[32];
        snprintf(buf, sizeof(buf), "LIFE: %u", (unsigned int)st->lives);
        lv_label_set_text(st->lives_lbl, buf);
    }

    /* Redraw Playfield */
    if (st->playfield) {
        lv_obj_invalidate(st->playfield);
    }
}

static void space_shooter_key_cb(lv_event_t *e)
{
    shooter_state_t *st = (shooter_state_t *)lv_event_get_user_data(e);
    if (!st) return;

    uint32_t key = lv_event_get_key(e);
    if (st->game_over) {
        if (key == LV_KEY_ENTER || key == '5' || key == '1' || key == ' ') {
            space_shooter_reset_game(st);
        }
        return;
    }

    switch (key) {
    case LV_KEY_LEFT:
    case '4':
        st->player_vx = -5;
        break;
    case LV_KEY_RIGHT:
    case '6':
        st->player_vx = 5;
        break;
    case LV_KEY_ENTER:
    case LV_KEY_UP:
    case '5':
    case '2':
    case ' ':
        space_shooter_fire_laser(st);
        break;
    default:
        break;
    }
}

static void space_shooter_screen_delete_cb(lv_event_t *e)
{
    shooter_state_t *st = (shooter_state_t *)lv_event_get_user_data(e);
    if (!st) return;
    if (st->timer) {
        lv_timer_delete(st->timer);
        st->timer = NULL;
    }
    free(st);
}

static shooter_state_t *s_active_shooter = NULL;

static void on_shooter_restart_action(void)
{
    if (s_active_shooter) {
        space_shooter_reset_game(s_active_shooter);
    }
}

static void on_shooter_exit_action(void)
{
    win_mgr_pop();
}

void vapp_space_shooter_launch(const vapp_package_t *pkg)
{
    shooter_state_t *st = (shooter_state_t *)calloc(1, sizeof(shooter_state_t));
    if (!st) return;

    s_active_shooter = st;

    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, 176, 220);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x05070E), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)calloc(1, sizeof(win_mgr_screen_hdr_t));
    if (hdr) {
        hdr->view_type = VEEBHA_VIEW_TYPE_GENERIC;
        hdr->fullscreen_mode = OS_FULLSCREEN_FULL;
        strncpy(hdr->title, pkg ? pkg->header.name : "Space Shooter", sizeof(hdr->title) - 1);
        lv_obj_set_user_data(screen, hdr);
    }

    st->screen = screen;
    lv_obj_add_event_cb(screen, space_shooter_screen_delete_cb, LV_EVENT_DELETE, st);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, space_shooter_key_cb, LV_EVENT_KEY, st);

    /* 1. Sleek Top HUD Strip (16px) */
    lv_obj_t *hud = lv_obj_create(screen);
    lv_obj_set_size(hud, lv_pct(100), 16);
    lv_obj_set_style_bg_color(hud, lv_color_hex(0x0A0F1D), 0);
    lv_obj_set_style_border_side(hud, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(hud, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_border_width(hud, 1, 0);
    lv_obj_set_style_pad_hor(hud, 4, 0);
    lv_obj_set_style_pad_ver(hud, 0, 0);
    lv_obj_set_flex_flow(hud, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hud, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(hud, LV_OBJ_FLAG_SCROLLABLE);

    st->score_lbl = lv_label_create(hud);
    lv_obj_set_style_text_color(st->score_lbl, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(st->score_lbl, &lv_font_montserrat_10, 0);
    lv_label_set_text(st->score_lbl, "PTS: 0000");

    st->wave_lbl = lv_label_create(hud);
    lv_obj_set_style_text_color(st->wave_lbl, lv_color_hex(0xFFD600), 0);
    lv_obj_set_style_text_font(st->wave_lbl, &lv_font_montserrat_10, 0);
    lv_label_set_text(st->wave_lbl, "WAVE: 1");

    st->lives_lbl = lv_label_create(hud);
    lv_obj_set_style_text_color(st->lives_lbl, lv_color_hex(0xFF1744), 0);
    lv_obj_set_style_text_font(st->lives_lbl, &lv_font_montserrat_10, 0);
    lv_label_set_text(st->lives_lbl, "LIFE: 3");

    /* 2. Direct Blitter Canvas / Playfield (176x204 px) */
    lv_obj_t *playfield = lv_button_create(screen);
    st->playfield = playfield;
    if (hdr) hdr->first_item = playfield;
    lv_obj_set_size(playfield, PLAYFIELD_W, PLAYFIELD_H);
    lv_obj_set_style_bg_color(playfield, lv_color_hex(0x05070E), 0);
    lv_obj_set_style_border_width(playfield, 0, 0);
    lv_obj_set_style_pad_all(playfield, 0, 0);
    lv_obj_remove_flag(playfield, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(playfield, space_shooter_draw_cb, LV_EVENT_DRAW_POST, st);
    lv_obj_add_event_cb(playfield, space_shooter_key_cb, LV_EVENT_KEY, st);

    space_shooter_reset_game(st);

    /* 33ms game loop (~30 FPS) */
    st->timer = lv_timer_create(space_shooter_timer_cb, 33, st);

    lv_obj_t *sk = softkey_bar_create(screen, "Restart", "Exit");
    if (hdr) hdr->softkey_bar = sk;

    win_mgr_push(screen, "Restart", on_shooter_restart_action, "Exit", on_shooter_exit_action);

    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_add_obj(g, playfield);
        lv_group_focus_obj(playfield);
        lv_group_set_editing(g, true);
    }
}
