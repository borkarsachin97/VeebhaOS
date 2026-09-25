/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */

#ifndef SDK_INCLUDE_VEEBHA_LOG_H
#define SDK_INCLUDE_VEEBHA_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdbool.h>

/**
 * Severity levels for system and application logging.
 */
typedef enum {
    OS_LOG_LEVEL_DEBUG = 0,
    OS_LOG_LEVEL_INFO,
    OS_LOG_LEVEL_WARN,
    OS_LOG_LEVEL_ERROR,
    OS_LOG_LEVEL_NONE
} os_log_level_t;

/**
 * Pluggable log sink function prototype.
 *
 * @param level Severity level of the log entry.
 * @param tag Component or module identifier.
 * @param msg Formatted log message string (null-terminated).
 */
typedef void (*os_log_sink_fn)(os_log_level_t level, const char *tag, const char *msg);

/**
 * Initialize or reset the logging subsystem.
 */
void os_log_init(void);

/**
 * Set minimum logging severity filter.
 *
 * @param level Minimum level to process. Messages below this level are dropped.
 */
void os_log_set_level(os_log_level_t level);

/**
 * Get current minimum logging severity.
 */
os_log_level_t os_log_get_level(void);

/**
 * Register a custom log sink callback.
 * If sink is NULL, the subsystem defaults to stdout formatted output.
 *
 * @param sink Custom sink callback function or NULL for default stdout sink.
 */
void os_log_set_sink(os_log_sink_fn sink);

/**
 * Write a formatted log entry.
 * Guarantees zero dynamic heap allocation by using a fixed stack line buffer.
 *
 * @param level Severity level.
 * @param tag Component identifier tag.
 * @param fmt Format string (printf-style).
 */
void os_log_write(os_log_level_t level, const char *tag, const char *fmt, ...);

/**
 * Write a formatted log entry using a va_list.
 *
 * @param level Severity level.
 * @param tag Component identifier tag.
 * @param fmt Format string.
 * @param args Variadic argument list.
 */
void os_log_vwrite(os_log_level_t level, const char *tag, const char *fmt, va_list args);

/* Convenience logging macros */
#define OS_LOGD(tag, ...) os_log_write(OS_LOG_LEVEL_DEBUG, tag, __VA_ARGS__)
#define OS_LOGI(tag, ...) os_log_write(OS_LOG_LEVEL_INFO,  tag, __VA_ARGS__)
#define OS_LOGW(tag, ...) os_log_write(OS_LOG_LEVEL_WARN,  tag, __VA_ARGS__)
#define OS_LOGE(tag, ...) os_log_write(OS_LOG_LEVEL_ERROR, tag, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* SDK_INCLUDE_VEEBHA_LOG_H */
