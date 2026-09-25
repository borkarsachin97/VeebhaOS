/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Hardware Abstraction Layer: Display Driver Contract
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef HAL_DISPLAY_H
#define HAL_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef void (*hal_display_flush_cb_t)(void *disp_drv, const void *area, const void *color_p);

/**
 * Initialize the hardware display controller and configure frame buffers.
 *
 * @return true on success, false on failure.
 */
bool hal_display_init(void);

/**
 * Flush a rectangle of 16-bit RGB565 pixel data to the display panel.
 *
 * @param x1 Left coordinate (inclusive).
 * @param y1 Top coordinate (inclusive).
 * @param x2 Right coordinate (inclusive).
 * @param y2 Bottom coordinate (inclusive).
 * @param color_p Pointer to 16-bit RGB565 pixel array.
 */
void hal_display_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t *color_p);

/**
 * Set display backlight brightness level.
 *
 * @param brightness_pct Brightness percentage (0-100).
 */
void hal_display_set_backlight(uint8_t brightness_pct);

/**
 * Capture current framebuffer rendering and save as BMP image to file.
 *
 * @param filename File path to save the screenshot BMP image.
 * @return true on success, false on failure.
 */
bool hal_display_save_screenshot(const char *filename);

/**
 * Deinitialize display hardware and release buffers.
 */
void hal_display_deinit(void);


#ifdef __cplusplus
}
#endif

#endif /* HAL_DISPLAY_H */
