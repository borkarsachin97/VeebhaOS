/* SPDX-License-Identifier: MIT */
#ifndef LAZY_LOAD_H
#define LAZY_LOAD_H

#include "lvgl.h"
#include "veebha_templates.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Context passed to the lazy‑load timer. */
typedef struct {
    tpl_list_view_t desc;   /* copy of the descriptor to use */
    lv_obj_t *placeholder; /* the placeholder screen */
} lazy_load_ctx_t;

/** Create a minimal placeholder screen showing only a title.
 *  The screen is pushed immediately; the real list will appear when the timer fires.
 */
lv_obj_t *placeholder_create(const char *title);

/** Timer callback that builds the full list screen and pushes it.
 *  After creation it frees the context.
 */
void lazy_load_cb(lv_timer_t *timer);

#ifdef __cplusplus
}
#endif

#endif /* LAZY_LOAD_H */
