/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Core OS Kernel Abstractions & Thread-Safety Primitives
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef SDK_CORE_OS_KERNEL_H
#define SDK_CORE_OS_KERNEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdbool.h>

/**
 * Initialize kernel synchronization primitives (LVGL mutex, scheduler flags).
 */
void os_kernel_init(void);

/**
 * Acquire the global LVGL rendering mutex.
 * Must be called before modifying LVGL widgets, window management, or timer calls from non-UI tasks.
 */
void os_lvgl_lock(void);

/**
 * Release the global LVGL rendering mutex.
 */
void os_lvgl_unlock(void);

/**
 * Check if the FreeRTOS multitasking scheduler is currently executing.
 *
 * @return true if scheduler state is taskSCHEDULER_RUNNING, false otherwise.
 */
bool os_is_scheduler_running(void);

#ifdef __cplusplus
}
#endif

#endif /* SDK_CORE_OS_KERNEL_H */
