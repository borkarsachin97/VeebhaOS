/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */

#ifndef SDK_INCLUDE_VEEBHA_TEMPLATES_H
#define SDK_INCLUDE_VEEBHA_TEMPLATES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdint.h>

/**
 * List Template Item Descriptor
 */
typedef struct {
    const void *icon;       /* Optional image/icon descriptor or symbol string */
    const char *title;      /* Primary label */
    const char *subtext;    /* Secondary metadata (e.g. details, status, counter) */
} tpl_list_item_t;

/**
 * List Template View Descriptor (Declarative Contract)
 */
typedef struct {
    const char             *title;      /* Screen title rendered in Status Bar / Header */
    const tpl_list_item_t  *items;      /* Array of row items */
    uint16_t                count;      /* Number of items */
    void (*on_select)(uint16_t index);  /* Invoked on click / D-pad Center / LSK */
    void (*on_back)(void);              /* Invoked on RSK (defaults to win_mgr_pop if NULL) */
    const char             *lsk_label;  /* Left Softkey text (defaults to "Select") */
    const char             *rsk_label;  /* Right Softkey text (defaults to "Back") */
} tpl_list_view_t;

/**
 * Instantiate a zero-coordinate List Screen according to the Template 1 contract.
 *
 * @param desc Pointer to the declarative view descriptor.
 * @return Root screen lv_obj_t pointer ready for win_mgr_push().
 */
lv_obj_t * tpl_list_create(const tpl_list_view_t *desc);

/**
 * Default action callbacks for List Template softkeys
 */
void tpl_list_default_lsk(void);
void tpl_list_default_rsk(void);

/* ============================================================================
 * Template 2: Grid View Declarations
 * ============================================================================ */

/**
 * Grid Template Item Descriptor
 */
typedef struct {
    const void *icon;       /* Icon symbol (e.g. LV_SYMBOL_CALL) or image pointer */
    const char *title;      /* Item label underneath icon */
    uint32_t    badge;      /* Optional notification badge count (0 = none) */
} tpl_grid_item_t;

/**
 * Grid Template View Descriptor (Declarative Contract)
 */
typedef struct {
    const char             *title;      /* Screen title in Status Bar / Header */
    const tpl_grid_item_t  *items;      /* Array of grid items */
    uint16_t                count;      /* Total number of items (e.g. 9 for 3x3) */
    uint8_t                 columns;    /* Number of columns (default 3 if 0) */
    void (*on_select)(uint16_t index);  /* Invoked on click / D-pad Center / LSK */
    void (*on_back)(void);              /* Invoked on RSK (defaults to win_mgr_pop if NULL) */
    const char             *lsk_label;  /* Left Softkey text (defaults to "OK") */
    const char             *rsk_label;  /* Right Softkey text (defaults to "Back") */
} tpl_grid_view_t;

/**
 * Instantiate a zero-coordinate Grid Screen according to the Template 2 contract.
 *
 * @param desc Pointer to declarative grid view descriptor.
 * @return Root screen lv_obj_t pointer ready for win_mgr_push().
 */
lv_obj_t * tpl_grid_create(const tpl_grid_view_t *desc);

void tpl_grid_default_lsk(void);
void tpl_grid_default_rsk(void);

/* ============================================================================
 * Template 3: Modal Dialog Declarations
 * ============================================================================ */

/**
 * Modal Dialog Descriptor
 */
typedef struct {
    const char *title;                  /* Dialog title (e.g. "Notice", "Confirm") */
    const char *message;                /* Dialog body message (auto-wrapping) */
    const void *icon;                   /* Optional icon symbol or image */
    void (*on_confirm)(void);           /* Callback on LSK / Enter */
    void (*on_cancel)(void);            /* Callback on RSK / Esc */
    const char *lsk_label;              /* Left softkey label (defaults to "OK") */
    const char *rsk_label;              /* Right softkey label (defaults to "Cancel") */
} tpl_dialog_desc_t;

/**
 * Display a modal dialog on top of the active view on lv_layer_top().
 *
 * @param desc Pointer to dialog descriptor.
 * @return Root dialog overlay lv_obj_t pointer, or NULL on error.
 */
lv_obj_t * tpl_dialog_show(const tpl_dialog_desc_t *desc);

/**
 * Close and destroy the active modal dialog, restoring softkeys and focus.
 */
void tpl_dialog_close(void);

/**
 * Check if a modal dialog is currently displayed.
 */
bool tpl_dialog_is_active(void);

/* ============================================================================
 * Template 4: Text Editor Declarations
 * ============================================================================ */

/**
 * Text Editor Descriptor
 */
typedef struct {
    const char *title;                  /* Screen title in header */
    char       *buffer;                 /* Target string buffer */
    uint16_t    max_len;                /* Maximum buffer capacity */
    void (*on_save)(const char *text);  /* Callback on LSK ("Done") */
    void (*on_cancel)(void);            /* Callback on RSK ("Cancel") when empty */
    const char *lsk_label;              /* Left softkey label (defaults to "Done") */
    const char *rsk_label;              /* Right softkey label (defaults to "Clear") */
} tpl_editor_desc_t;

/**
 * Instantiate a zero-coordinate Text Editor according to the Template 4 contract.
 *
 * @param desc Pointer to editor descriptor.
 * @return Root screen lv_obj_t pointer ready for win_mgr_push().
 */
lv_obj_t * tpl_editor_create(const tpl_editor_desc_t *desc);

void tpl_editor_default_lsk(void);
void tpl_editor_default_rsk(void);

#ifdef __cplusplus
}
#endif

#endif /* SDK_INCLUDE_VEEBHA_TEMPLATES_H */
