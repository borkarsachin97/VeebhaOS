/*
 * VeebhaOS - QEMU RDA8809 Display Driver Header
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BOARDS_QEMU_XCPU_DRIVERS_HAL_DISPLAY_QEMU_H
#define BOARDS_QEMU_XCPU_DRIVERS_HAL_DISPLAY_QEMU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "third_party/lvgl/lvgl.h"

void hal_display_qemu_init(void);
lv_display_t * hal_display_qemu_get_disp(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_QEMU_XCPU_DRIVERS_HAL_DISPLAY_QEMU_H */
