/* SPDX-License-Identifier: MIT */
#include "apps/screens/template_screen.h"
#include "sdk/include/veebha_templates.h"
#include "sdk/include/veebha_win_mgr.h"
#include "lvgl.h"

/* Hidden template screen created once at OS start */
static lv_obj_t *g_template_screen = NULL;

void template_screen_init(void)
{
    if (g_template_screen) {
        return; /* already initialised */
    }
    /* Create a minimal list descriptor – empty, keep_alive so it won't be deleted */
    static const tpl_list_item_t empty_items[1] = {{0}};
    tpl_list_view_t tmpl_desc = {
        .title      = "",
        .items      = (tpl_list_item_t *)empty_items,
        .count      = 0,
        .on_select  = NULL,
        .on_back    = NULL,
        .lsk_label  = NULL,
        .rsk_label  = NULL,
        .keep_alive = true,
    };
    g_template_screen = tpl_list_create(&tmpl_desc);
    if (g_template_screen) {
        /* Keep the screen hidden; repopulated and shown when needed */
        lv_obj_add_flag(g_template_screen, LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t *template_screen_get(void)
{
    return g_template_screen;
}
