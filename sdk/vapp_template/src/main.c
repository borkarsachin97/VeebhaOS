/*
 * VeebhaOS VAPP Standalone Application Template
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* VeebhaOS Application Life-Cycle Definitions */
typedef enum {
    KEY_UP = 0,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_OK,
    KEY_LSK,
    KEY_RSK,
    KEY_NUM_0,
    KEY_NUM_1,
    KEY_NUM_2,
    KEY_NUM_3,
    KEY_NUM_4,
    KEY_NUM_5,
    KEY_NUM_6,
    KEY_NUM_7,
    KEY_NUM_8,
    KEY_NUM_9,
    KEY_STAR,
    KEY_HASH
} app_key_t;

typedef struct {
    uint32_t score;
    uint16_t moves;
    int      board[4][4];
} app_state_t;

static app_state_t g_app;

void vapp_init(void)
{
    memset(&g_app, 0, sizeof(g_app));
    g_app.board[0][0] = 2;
    g_app.board[0][1] = 2;
    printf("[2048_VAPP] Initialized 2048 Retro application\n");
}

void vapp_on_key(app_key_t key)
{
    if (key == KEY_UP || key == KEY_NUM_2) {
        g_app.moves++;
        g_app.score += 4;
        printf("[2048_VAPP] Move UP -> Score: %u, Moves: %u\n", g_app.score, g_app.moves);
    } else if (key == KEY_DOWN || key == KEY_NUM_8) {
        g_app.moves++;
        g_app.score += 4;
        printf("[2048_VAPP] Move DOWN -> Score: %u, Moves: %u\n", g_app.score, g_app.moves);
    } else if (key == KEY_LEFT || key == KEY_NUM_4) {
        g_app.moves++;
        g_app.score += 4;
        printf("[2048_VAPP] Move LEFT -> Score: %u, Moves: %u\n", g_app.score, g_app.moves);
    } else if (key == KEY_RIGHT || key == KEY_NUM_6) {
        g_app.moves++;
        g_app.score += 4;
        printf("[2048_VAPP] Move RIGHT -> Score: %u, Moves: %u\n", g_app.score, g_app.moves);
    }
}

void vapp_render(void)
{
    /* Rendering logic using LVGL / VeebhaOS Templates */
}

void vapp_deinit(void)
{
    printf("[2048_VAPP] Teardown and memory reclaimed\n");
}

int main(void)
{
    vapp_init();
    vapp_on_key(KEY_UP);
    vapp_deinit();
    return 0;
}
