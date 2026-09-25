/*
 * VeebhaOS - QEMU RDA8809 Keypad Driver Header
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BOARDS_QEMU_XCPU_DRIVERS_HAL_KEYPAD_QEMU_H
#define BOARDS_QEMU_XCPU_DRIVERS_HAL_KEYPAD_QEMU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "third_party/lvgl/lvgl.h"

void hal_keypad_qemu_init(void);
void hal_keypad_task(void *pvParameters);
lv_indev_t * hal_keypad_qemu_get_indev(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_QEMU_XCPU_DRIVERS_HAL_KEYPAD_QEMU_H */
