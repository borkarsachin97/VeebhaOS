/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Generic LCD Panel Driver Interface / HAL for RDA8809
 */

#ifndef _LCD_PANEL_H_
#define _LCD_PANEL_H_

#include "cs_types.h"

typedef struct lcd_panel_s lcd_panel_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    bool     invert_colors;
    uint8_t  orientation;
} lcd_panel_config_t;

struct lcd_panel_s {
    const char *name;
    uint16_t width;
    uint16_t height;

    /**
     * @brief Initialize panel hardware registers and power step-up circuits.
     */
    bool     (*init)(const lcd_panel_config_t *config);

    /**
     * @brief Query panel controller ID register (e.g. 0x00).
     */
    uint16_t (*read_id)(void);

    /**
     * @brief Set display RAM address window and prepare memory write.
     */
    void     (*set_window)(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

    /**
     * @brief Put panel into sleep mode (power-down step-up circuits and display OFF).
     */
    void     (*sleep)(void);

    /**
     * @brief Wake panel from sleep mode (restore charge pumps and display ON).
     */
    void     (*wakeup)(void);
};

#endif // _LCD_PANEL_H_
