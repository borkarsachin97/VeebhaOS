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

#ifndef APPS_FILES_APP_FILES_H
#define APPS_FILES_APP_FILES_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the File Manager subsystem and self-register with app_registry.
 */
void app_files_init(void);

/**
 * Open the File Manager at the root directory (/sdcard/).
 */
void app_files_open(void);

/**
 * Open the File Manager at a specific directory path.
 *
 * @param path Directory path to open (e.g. "/sdcard" or "/sdcard/Music").
 */
void app_files_open_path(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* APPS_FILES_APP_FILES_H */
