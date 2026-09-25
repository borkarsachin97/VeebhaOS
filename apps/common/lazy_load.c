/* SPDX-License-Identifier: MIT */
#include "lazy_load.h"
#include "sdk/include/veebha_templates.h"
#include "sdk/include/veebha_win_mgr.h"
#include "boards/board_config.h"
#include "lvgl.h"
#include <stdlib.h>

/* Simple placeholder screen showing only a title label */
lv_obj_t *placeholder_create(const char *title)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_size(scr, CONFIG_DISP_HOR_RES, CONFIG_DISP_VER_RES);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* Title label centered */
    lv_obj_t *lbl = lv_label_create(scr);
    lv_label_set_text(lbl, title ? title : "");
    lv_obj_center(lbl);
    return scr;
}

void lazy_load_cb(lv_timer_t *timer)
{
    lazy_load_ctx_t *ctx = (lazy_load_ctx_t *)timer->user_data;
    if (!ctx) {
        return;
    }
    /* Build the real list screen */
    lv_obj_t *real = tpl_list_create(&ctx->desc);
    if (real) {
        /* Push the real screen */
        win_mgr_push(real,
                     ctx->desc.lsk_label, tpl_list_default_lsk,
                     ctx->desc.rsk_label, tpl_list_default_rsk);
    }

    /* Delete placeholder screen now that the real one is pushed */
    if (ctx->placeholder && lv_obj_is_valid(ctx->placeholder)) {
        lv_obj_del(ctx->placeholder);
        ctx->placeholder = NULL;
    }

    /* Clean up context; LVGL auto-deletes one-shot timers after callback */
    free(ctx);
}
