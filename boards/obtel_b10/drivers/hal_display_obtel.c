/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * OBTEL B10 Hardware Abstraction Layer: Display Driver (ILI9225G/9226)
 *
 * SPDX-License-Identifier: MIT
 */

#include "sdk/hal/hal_display.h"
#include "boards/obtel_b10/include/global_macros.h"
#include "lvgl.h"
#include <stdbool.h>
#include <string.h>

#define LCD_WIDTH   176
#define LCD_HEIGHT  220

#define LCD_BUFFER_ROWS (LCD_HEIGHT / 10) /* 22 rows */
#define LCD_BUFFER_SIZE (LCD_WIDTH * LCD_BUFFER_ROWS * sizeof(lv_color16_t))

static lv_display_t *s_disp = NULL;
static uint8_t s_buf1[LCD_BUFFER_SIZE];
static uint8_t s_buf2[LCD_BUFFER_SIZE];

static void ili9225_write_cmd(uint16_t cmd)
{
    REG32(RDA_BASE_LCD + 0x00) = cmd;
}

static void ili9225_write_data(uint16_t data)
{
    REG32(RDA_BASE_LCD + 0x04) = data;
}

static void ili9225_set_window(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    ili9225_write_cmd(0x36); ili9225_write_data((uint16_t)x2);
    ili9225_write_cmd(0x37); ili9225_write_data((uint16_t)x1);
    ili9225_write_cmd(0x38); ili9225_write_data((uint16_t)y2);
    ili9225_write_cmd(0x39); ili9225_write_data((uint16_t)y1);
    ili9225_write_cmd(0x20); ili9225_write_data((uint16_t)x1);
    ili9225_write_cmd(0x21); ili9225_write_data((uint16_t)y1);
    ili9225_write_cmd(0x22); /* Write RAM */
}

static void obtel_display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    if (px_map) {
        int32_t x1 = area->x1;
        int32_t y1 = area->y1;
        int32_t x2 = area->x2;
        int32_t y2 = area->y2;
        int32_t len = (x2 - x1 + 1) * (y2 - y1 + 1);

        ili9225_set_window(x1, y1, x2, y2);

        const uint16_t *pixels = (const uint16_t *)px_map;
        for (int32_t i = 0; i < len; i++) {
            ili9225_write_data(pixels[i]);
        }
    }

    lv_display_flush_ready(disp);
}

bool hal_display_init(void)
{
    /* Reset and initialize ILI9225G LCD panel */
    ili9225_write_cmd(0x01); ili9225_write_data(0x011C); /* Power control */
    ili9225_write_cmd(0x02); ili9225_write_data(0x0100); /* LCD AC driving control */
    ili9225_write_cmd(0x03); ili9225_write_data(0x1030); /* Entry mode */
    ili9225_write_cmd(0x07); ili9225_write_data(0x0012); /* Display control 1 */
    ili9225_write_cmd(0x07); ili9225_write_data(0x1017); /* Turn on display */

    s_disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    if (!s_disp) {
        return false;
    }

    lv_display_set_color_format(s_disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(s_disp, obtel_display_flush_cb);
    lv_display_set_buffers(s_disp, s_buf1, s_buf2, sizeof(s_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    return true;
}

void hal_display_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t *color_p)
{
    if (color_p) {
        int32_t len = (x2 - x1 + 1) * (y2 - y1 + 1);
        ili9225_set_window(x1, y1, x2, y2);
        for (int32_t i = 0; i < len; i++) {
            ili9225_write_data(color_p[i]);
        }
    }
}

void hal_display_set_backlight(uint8_t brightness_pct)
{
    uint32_t pwm_val = (brightness_pct * 255) / 100;
    REG32(RDA_BASE_PMU + 0x10) = pwm_val;
}

bool hal_display_save_screenshot(const char *filename)
{
    (void)filename;
    /* Headless screenshot capture not available on bare-metal hardware */
    return false;
}

void hal_display_deinit(void)
{
    hal_display_set_backlight(0);
}

void hal_display_set_rotation(hal_disp_rot_t rot)
{
    if (!s_disp) return;

    switch (rot) {
    case HAL_DISP_ROT_90:
        lv_display_set_rotation(s_disp, LV_DISPLAY_ROTATION_90);
        break;
    case HAL_DISP_ROT_180:
        lv_display_set_rotation(s_disp, LV_DISPLAY_ROTATION_180);
        break;
    case HAL_DISP_ROT_270:
        lv_display_set_rotation(s_disp, LV_DISPLAY_ROTATION_270);
        break;
    case HAL_DISP_ROT_0:
    default:
        lv_display_set_rotation(s_disp, LV_DISPLAY_ROTATION_0);
        break;
    }
}

uint16_t hal_display_get_hor_res(void)
{
    return LCD_WIDTH;
}

uint16_t hal_display_get_ver_res(void)
{
    return LCD_HEIGHT;
}

lv_display_t * hal_display_get_lv_display(void)
{
    return s_disp;
}
