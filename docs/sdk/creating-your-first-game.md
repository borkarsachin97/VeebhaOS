# Creating Your First 2D Game on VeebhaOS

This tutorial walks through creating a high-performance, 60 FPS retro arcade game on VeebhaOS using pure integer math and the direct blitter engine.

---

## 1. Game Structure Overview

A VeebhaOS game consists of:
1. **Game State Structure**: Holds player coordinates, score, lives, and active entity pools.
2. **Screen Initialization (`_launch`)**: Configures `OS_FULLSCREEN_FULL` (176×220 full viewport).
3. **High-Precision Frame Timer**: Runs at 16–33 ms intervals to update physics and request blit redrawing.
4. **Keypad Input Handler**: Responds to D-Pad, numpad keys (`4`, `6`, `5`), and softkeys.
5. **Direct Blitter Draw Callback**: Renders game elements directly into the display layer.

---

## 2. Step 1: Define the Game State

Create your game state in pure integer types:

```c
typedef struct {
    lv_obj_t   *screen;
    lv_obj_t   *canvas;
    lv_timer_t *game_timer;

    /* Player coordinates (pure integer) */
    int16_t     player_x;
    int16_t     player_y;
    int16_t     player_speed;

    /* Game telemetry */
    uint32_t    score;
    uint8_t     lives;
    bool        game_over;
} my_game_state_t;
```

---

## 3. Step 2: Initialize Fullscreen Viewport

Initialize a 176×220 fullscreen object with `OS_FULLSCREEN_FULL`:

```c
void my_game_launch(const vapp_package_t *pkg) {
    my_game_state_t *st = (my_game_state_t *)calloc(1, sizeof(my_game_state_t));
    if (!st) return;

    st->player_x = 88;
    st->player_y = 190;
    st->player_speed = 6;
    st->lives = 3;

    /* 1. Create root screen */
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, 176, 220);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x050A14), 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    /* 2. Configure Fullscreen Header */
    win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)calloc(1, sizeof(win_mgr_screen_hdr_t));
    if (hdr) {
        hdr->view_type = VEEBHA_VIEW_TYPE_GENERIC;
        hdr->fullscreen_mode = OS_FULLSCREEN_FULL; /* Hides system status & softkey bars */
        strncpy(hdr->title, "My Game", sizeof(hdr->title) - 1);
        lv_obj_set_user_data(screen, hdr);
    }
    st->screen = screen;

    /* 3. Create interactive canvas */
    lv_obj_t *canvas = lv_button_create(screen);
    st->canvas = canvas;
    lv_obj_set_size(canvas, 176, 220);
    lv_obj_set_style_bg_opa(canvas, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(canvas, 0, 0);
    lv_obj_remove_flag(canvas, LV_OBJ_FLAG_SCROLLABLE);

    /* 4. Bind rendering and input callbacks */
    lv_obj_add_event_cb(canvas, my_game_draw_cb, LV_EVENT_DRAW_POST, st);
    lv_obj_add_event_cb(canvas, my_game_key_cb, LV_EVENT_KEY, st);

    /* 5. Start 60 FPS update timer (16 ms) */
    st->game_timer = lv_timer_create(my_game_timer_cb, 16, st);

    /* 6. Push to Window Manager */
    win_mgr_push(screen, "Restart", on_restart_action, "Exit", on_exit_action);

    /* 7. Focus canvas into keypad group */
    lv_group_t *g = win_mgr_get_group();
    if (g) {
        lv_group_add_obj(g, canvas);
        lv_group_focus_obj(canvas);
        lv_group_set_editing(g, true); /* Direct numeric & arrow key capture */
    }
}
```

---

## 4. Step 3: Handle Keypad Input

Handle both D-Pad arrows and standard feature-phone numeric keys:

```c
static void my_game_key_cb(lv_event_t *e) {
    my_game_state_t *st = (my_game_state_t *)lv_event_get_user_data(e);
    if (!st || st->game_over) return;

    uint32_t key = lv_event_get_key(e);

    /* Move Left: Left Arrow or '4' */
    if (key == LV_KEY_LEFT || key == '4') {
        st->player_x -= st->player_speed;
        if (st->player_x < 8) st->player_x = 8;
    }
    /* Move Right: Right Arrow or '6' */
    else if (key == LV_KEY_RIGHT || key == '6') {
        st->player_x += st->player_speed;
        if (st->player_x > 168) st->player_x = 168;
    }
    /* Fire Action: Center OK or '5' */
    else if (key == LV_KEY_ENTER || key == '5' || key == ' ') {
        spawn_player_bullet(st);
    }
}
```

---

## 5. Step 4: Fast Direct Blit Rendering

Implement rendering using `lv_draw_rect` inside `LV_EVENT_DRAW_POST`:

```c
static void my_game_draw_cb(lv_event_t *e) {
    my_game_state_t *st = (my_game_state_t *)lv_event_get_user_data(e);
    if (!st) return;

    lv_layer_t *layer = lv_event_get_layer(e);
    if (!layer) return;

    /* Setup blit descriptor */
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_opa = LV_OPA_COVER;
    dsc.radius = 2;

    /* Draw player ship */
    dsc.bg_color = lv_color_hex(0x00E5FF);
    lv_area_t player_area = {
        .x1 = st->player_x - 6,
        .y1 = st->player_y - 4,
        .x2 = st->player_x + 6,
        .y2 = st->player_y + 4
    };
    lv_draw_rect(layer, &dsc, &player_area);
}
```

---

## 6. Step 5: Update Timer

In the timer callback, advance physics and invalidate the canvas:

```c
static void my_game_timer_cb(lv_timer_t *t) {
    my_game_state_t *st = (my_game_state_t *)lv_timer_get_user_data(t);
    if (!st || st->game_over) return;

    update_bullets(st);
    update_enemies(st);

    /* Invalidate canvas to trigger hardware blit */
    lv_obj_invalidate(st->canvas);
}
```

---

## 7. Packaging Your Game

1. Create a `vapp.json` manifest:
   ```json
   {
     "name": "My Game",
     "author": "IndieDev",
     "version": "1.0.0",
     "type": "game",
     "icon": "PLAY",
     "description": "High-octane retro arcade game",
     "heap_budget_kb": 16,
     "app_id": 10
   }
   ```
2. Build and package into `.vapp`:
   ```bash
   python3 tools/vapp_pack.py --manifest vapp.json --bin build/my_game.bin --output vapps/my_game.vapp
   ```
3. Drop `my_game.vapp` into `/vapps/` and launch it from the **VeebhaOS Fun Zone**!
