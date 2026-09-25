/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Professional RGB565 Framebuffer Management & 2D Graphics Engine Header for RDA8809
 */

#ifndef _FRAMEBUFFER_H_
#define _FRAMEBUFFER_H_

#include <stdarg.h>
#include "cs_types.h"
#include "lcd_panel.h"

// =============================================================================
//  DISPLAY RESOLUTION & RGB565 PALETTE
// =============================================================================
#define LCDD_DISP_X             176
#define LCDD_DISP_Y             220
#define FB_PIXEL_COUNT          (LCDD_DISP_X * LCDD_DISP_Y)
#define FB_BUFFER_SIZE_BYTES    (FB_PIXEL_COUNT * 2)

// RGB565 conversion macro
#define RGB565(r, g, b)         ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

// Common 16-bit RGB565 Color Constants
#define COLOR_BLACK             0x0000
#define COLOR_WHITE             0xFFFF
#define COLOR_RED               0xF800
#define COLOR_GREEN             0x07E0
#define COLOR_BLUE              0x001F
#define COLOR_CYAN              0x07FF
#define COLOR_MAGENTA           0xF81F
#define COLOR_YELLOW            0xFFE0
#define COLOR_ORANGE            0xFD20
#define COLOR_DARKGREY          0x4208
#define COLOR_LIGHTGREY         0xC618
#define COLOR_DARKBLUE          0x0810
#define COLOR_TRANSPARENT       0x0001

// =============================================================================
//  FONT SELECTOR ENUM
// =============================================================================
typedef enum {
    FB_FONT_CLEAN_8X8 = 0,      /* Clean Sans 8x8 (High contrast, open counters, ideal for dense UI & tables) */
    FB_FONT_BOLD_8X16 = 1       /* Bold Sans 8x16 (High legibility for headers, title banners & large digits) */
} fb_font_t;

// =============================================================================
//  FRAMEBUFFER STRUCTURE
// =============================================================================
typedef struct {
    uint16_t          *buffer;        /* Pointer to RGB565 pixel buffer */
    uint16_t           width;         /* Framebuffer width in pixels */
    uint16_t           height;        /* Framebuffer height in pixels */
    uint16_t           stride;        /* Stride/pitch in pixels */
    uint16_t           dirty_x1;      /* Bounding dirty region min X */
    uint16_t           dirty_y1;      /* Bounding dirty region min Y */
    uint16_t           dirty_x2;      /* Bounding dirty region max X */
    uint16_t           dirty_y2;      /* Bounding dirty region max Y */
    uint8_t            is_dirty;      /* Dirty region flag */
    uint8_t            auto_flush;    /* Auto-flush mode flag */
    const lcd_panel_t *panel;         /* Attached LCD panel driver */
} framebuffer_t;

// =============================================================================
//  PUBLIC FRAMEBUFFER API
// =============================================================================

framebuffer_t* fb_init(const lcd_panel_t *panel);
framebuffer_t* fb_get_primary(void);
void fb_mark_dirty(framebuffer_t *fb, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void fb_clear_dirty(framebuffer_t *fb);

// GOUDA Hardware & DMA Flushing
void fb_flush(framebuffer_t *fb);
void fb_flush_async(framebuffer_t *fb);
bool fb_wait_flush(uint32_t timeout_ms);
bool fb_is_flushing(void);
void fb_flush_full(framebuffer_t *fb);
void fb_flush_region(framebuffer_t *fb, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void fb_hw_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

// 2D Drawing Primitives
void fb_clear(framebuffer_t *fb, uint16_t color);
void fb_put_pixel(framebuffer_t *fb, int16_t x, int16_t y, uint16_t color);
uint16_t fb_get_pixel(framebuffer_t *fb, int16_t x, int16_t y);
void fb_fill_rect(framebuffer_t *fb, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void fb_draw_rect(framebuffer_t *fb, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void fb_draw_line(framebuffer_t *fb, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void fb_draw_circle(framebuffer_t *fb, int16_t xc, int16_t yc, int16_t r, uint16_t color);
void fb_fill_circle(framebuffer_t *fb, int16_t xc, int16_t yc, int16_t r, uint16_t color);

// Alpha Blending & Sprites
void fb_blend_rect(framebuffer_t *fb, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, uint8_t alpha);
void fb_blit_sprite(framebuffer_t *fb, int16_t dst_x, int16_t dst_y,
                    const uint16_t *sprite, uint16_t spr_w, uint16_t spr_h,
                    uint16_t transparent_col);

// High-Legibility Typography
void fb_draw_char_ex(framebuffer_t *fb, int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, fb_font_t font, uint8_t scale);
void fb_draw_string_ex(framebuffer_t *fb, int16_t x, int16_t y, const char *str, uint16_t color, uint16_t bg, fb_font_t font, uint8_t scale);
void fb_draw_printf_ex(framebuffer_t *fb, int16_t x, int16_t y, uint16_t color, uint16_t bg, fb_font_t font, uint8_t scale, const char *fmt, ...);

void fb_draw_char(framebuffer_t *fb, int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t scale);
void fb_draw_string(framebuffer_t *fb, int16_t x, int16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t scale);
void fb_draw_printf(framebuffer_t *fb, int16_t x, int16_t y, uint16_t color, uint16_t bg, uint8_t scale, const char *fmt, ...);

// Camera & Video Frame Blitting
void fb_blit_yuv422(framebuffer_t *fb, int16_t dst_x, int16_t dst_y,
                    uint16_t dst_w, uint16_t dst_h,
                    const uint8_t *yuv, uint16_t src_w, uint16_t src_h);
void fb_blit_raw565(framebuffer_t *fb, int16_t dst_x, int16_t dst_y,
                    uint16_t dst_w, uint16_t dst_h,
                    const uint16_t *raw, uint16_t src_w, uint16_t src_h);

// Standalone 3D Demo
void render_3d_cube_standalone(framebuffer_t *fb, uint8_t ax, uint8_t ay, uint8_t az);

#endif // _FRAMEBUFFER_H_
