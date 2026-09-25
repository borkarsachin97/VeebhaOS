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

#include "veebha_log.h"
#include <stdio.h>
#include <string.h>

#define OS_LOG_BUFFER_SIZE 256

static os_log_level_t s_min_level = OS_LOG_LEVEL_DEBUG;
static os_log_sink_fn s_custom_sink = NULL;

static void default_log_sink(os_log_level_t level, const char *tag, const char *msg)
{
#if defined(CONFIG_SIMULATOR)
    const char *lvl_str = "D";
    switch (level) {
    case OS_LOG_LEVEL_DEBUG: lvl_str = "DEBUG"; break;
    case OS_LOG_LEVEL_INFO:  lvl_str = "INFO";  break;
    case OS_LOG_LEVEL_WARN:  lvl_str = "WARN";  break;
    case OS_LOG_LEVEL_ERROR: lvl_str = "ERROR"; break;
    default: lvl_str = "LOG"; break;
    }

    size_t len = strlen(msg);
    const char *nl = (len > 0 && msg[len - 1] == '\n') ? "" : "\n";

    if (tag && tag[0]) {
        printf("[%s] %s%s", tag, msg, nl);
    } else {
        printf("[%s] %s%s", lvl_str, msg, nl);
    }
#else
    /* Standalone / Real Hardware: do not write to anything and do not waste CPU cycles.
     * Future CDC console will hook into os_log_set_sink(). */
    (void)level;
    (void)tag;
    (void)msg;
#endif
}

void os_log_init(void)
{
    s_min_level = OS_LOG_LEVEL_DEBUG;
    s_custom_sink = NULL;
}

void os_log_set_level(os_log_level_t level)
{
    s_min_level = level;
}

os_log_level_t os_log_get_level(void)
{
    return s_min_level;
}

void os_log_set_sink(os_log_sink_fn sink)
{
    s_custom_sink = sink;
}

void os_log_vwrite(os_log_level_t level, const char *tag, const char *fmt, va_list args)
{
#if !defined(CONFIG_SIMULATOR)
    /* If no custom sink is installed, do not even format string to avoid wasting CPU cycles */
    if (!s_custom_sink) {
        return;
    }
#endif

    if (level < s_min_level || level >= OS_LOG_LEVEL_NONE) {
        return;
    }

    /* Fixed stack line buffer to guarantee zero dynamic heap allocation */
    char line_buf[OS_LOG_BUFFER_SIZE];
    vsnprintf(line_buf, sizeof(line_buf), fmt, args);

    if (s_custom_sink) {
        s_custom_sink(level, tag ? tag : "OS", line_buf);
    } else {
        default_log_sink(level, tag, line_buf);
    }
}

void os_log_write(os_log_level_t level, const char *tag, const char *fmt, ...)
{
    if (level < s_min_level || level >= OS_LOG_LEVEL_NONE) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    os_log_vwrite(level, tag, fmt, args);
    va_end(args);
}
