/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Hardware Abstraction Layer: Keypad Input Contract
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef HAL_KEYPAD_H
#define HAL_KEYPAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * Initialize keypad scanning hardware / matrix.
 */
void hal_keypad_init(void);

/**
 * Poll for raw hardware keypad events.
 *
 * @param out_key Pointer to receive pressed key code.
 * @param out_pressed Pointer to receive press state (true = pressed, false = released).
 * @return true if an event was available, false otherwise.
 */
bool hal_keypad_poll(uint32_t *out_key, bool *out_pressed);

/**
 * Ingest physical/matrix keys and feed into LVGL indev.
 */
void hal_keypad_poll_and_feed(void);

/**
 * Deinitialize keypad driver.
 */
void hal_keypad_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_KEYPAD_H */
