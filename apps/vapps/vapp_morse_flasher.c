/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "vapp_morse_flasher.h"
#include "sdk/include/veebha_win_mgr.h"
#include "sdk/include/veebha_softkeys.h"
#include "sdk/include/veebha_templates.h"
#include "sdk/include/veebha_theme.h"
#include "sdk/include/veebha_log.h"
#include "sdk/text/font_fallback.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "VAPP_MORSE"

typedef struct {
    lv_obj_t   *screen;
    lv_obj_t   *flash_box;
    lv_timer_t *timer;
    bool        state;
    uint8_t     step;
} morse_state_t;

static void morse_timer_cb(lv_timer_t *t)
{
    morse_state_t *st = (morse_state_t *)lv_timer_get_user_data(t);
    if (!st || !st->flash_box) return;

    st->step = (st->step + 1) % 6;
    st->state = (st->step % 2 == 0);
    lv_obj_set_style_bg_color(st->flash_box, st->state ? lv_color_hex(0xFFD600) : lv_color_hex(0x111111), 0);
}

static void on_morse_delete_cb(lv_event_t *e)
{
    morse_state_t *st = (morse_state_t *)lv_event_get_user_data(e);
    if (st) {
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
}

void vapp_morse_flasher_launch(const vapp_package_t *pkg)
{
    morse_state_t *st = (morse_state_t *)calloc(1, sizeof(morse_state_t));
    if (!st) return;

    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, 176, 220);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);

    win_mgr_screen_hdr_t *hdr = (win_mgr_screen_hdr_t *)calloc(1, sizeof(win_mgr_screen_hdr_t));
    if (hdr) {
        hdr->view_type = VEEBHA_VIEW_TYPE_GENERIC;
        strncpy(hdr->title, pkg ? pkg->header.name : "Morse", sizeof(hdr->title) - 1);
        lv_obj_set_user_data(screen, hdr);
    }
    st->screen = screen;
    lv_obj_add_event_cb(screen, on_morse_delete_cb, LV_EVENT_DELETE, st);

    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_set_size(content, lv_pct(100), 0);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(content, lv_color_hex(0x141414), 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 8, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *box = lv_obj_create(content);
    st->flash_box = box;
    lv_obj_set_size(box, 70, 70);
    lv_obj_set_style_bg_color(box, lv_color_hex(0x111111), 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(box, 35, 0);
    lv_obj_set_style_border_color(box, lv_color_hex(0xFFD600), 0);
    lv_obj_set_style_border_width(box, 2, 0);

    lv_obj_t *lbl = lv_label_create(content);
    lv_label_set_text(lbl, "MORSE SOS\n... --- ...\nOptical Strobe");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl, veebha_font_get_default(), 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(lbl, 8, 0);

    st->timer = lv_timer_create(morse_timer_cb, 300, st);

    lv_obj_t *sk = softkey_bar_create(screen, "Flash", "Back");
    if (hdr) hdr->softkey_bar = sk;

    win_mgr_push(screen, "Flash", NULL, "Back", tpl_list_default_rsk);
}
