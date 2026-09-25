/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Simulator Display Driver Presentation Interface
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BOARDS_SIMULATOR_DRIVERS_HAL_DISPLAY_SIM_H
#define BOARDS_SIMULATOR_DRIVERS_HAL_DISPLAY_SIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * Query if a new frame has been rendered by LVGL and is waiting for presentation.
 */
bool hal_display_sim_has_new_frame(void);

/**
 * Present the shadow framebuffer to the SDL2 window on Thread 0.
 */
void hal_display_sim_present(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_SIMULATOR_DRIVERS_HAL_DISPLAY_SIM_H */
