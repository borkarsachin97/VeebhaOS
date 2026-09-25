/* SPDX-License-Identifier: MIT */
#ifndef TEMPLATE_SCREEN_H
#define TEMPLATE_SCREEN_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Initialise a hidden template list screen. Called once at OS start. */
void template_screen_init(void);

/** Return the (hidden) template screen object. */
lv_obj_t *template_screen_get(void);

#ifdef __cplusplus
}
#endif
#endif /* TEMPLATE_SCREEN_H */
