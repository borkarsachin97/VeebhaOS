/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Core OS Kernel Abstractions & Thread-Safety Primitives
 *
 * SPDX-License-Identifier: MIT
 */

#include "os_kernel.h"
#include "boards/board_config.h"
#include "sdk/include/veebha_log.h"
#include <stdio.h>

#if defined(CONFIG_BOARD_SIMULATOR) && CONFIG_BOARD_SIMULATOR
#include <pthread.h>
static pthread_mutex_t s_lvgl_mutex;
#else
static SemaphoreHandle_t s_lvgl_mutex = NULL;
#endif

#define TAG "OS_KERNEL"

static bool s_kernel_initialized = false;

void os_kernel_init(void)
{
    if (s_kernel_initialized) return;

#if defined(CONFIG_BOARD_SIMULATOR) && CONFIG_BOARD_SIMULATOR
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&s_lvgl_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
#else
    s_lvgl_mutex = xSemaphoreCreateRecursiveMutex();
#endif

    s_kernel_initialized = true;
    OS_LOGI(TAG, "Kernel primitives initialized (Recursive LVGL Mutex initialized)");
}

void os_lvgl_lock(void)
{
    if (s_kernel_initialized) {
#if defined(CONFIG_BOARD_SIMULATOR) && CONFIG_BOARD_SIMULATOR
        pthread_mutex_lock(&s_lvgl_mutex);
#else
        if (s_lvgl_mutex) {
            xSemaphoreTakeRecursive(s_lvgl_mutex, portMAX_DELAY);
        }
#endif
    }
}

void os_lvgl_unlock(void)
{
    if (s_kernel_initialized) {
#if defined(CONFIG_BOARD_SIMULATOR) && CONFIG_BOARD_SIMULATOR
        pthread_mutex_unlock(&s_lvgl_mutex);
#else
        if (s_lvgl_mutex) {
            xSemaphoreGiveRecursive(s_lvgl_mutex);
        }
#endif
    }
}

bool os_is_scheduler_running(void)
{
    return (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);
}

