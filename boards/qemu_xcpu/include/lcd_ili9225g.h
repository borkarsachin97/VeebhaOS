/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * ILI9225G LCD Panel Driver Header for RDA8809
 */

#ifndef _LCD_ILI9225G_H_
#define _LCD_ILI9225G_H_

#include "lcd_panel.h"

#define ILI9225G_LCD_WIDTH      176
#define ILI9225G_LCD_HEIGHT     220
#define ILI9225G_EXPECTED_ID    0x9225

/**
 * @brief Retrieve the singleton instance of the ILI9225G panel driver.
 */
const lcd_panel_t* lcd_ili9225g_get_panel(void);

#endif // _LCD_ILI9225G_H_
