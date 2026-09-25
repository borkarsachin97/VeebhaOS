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

#ifndef SDK_VFS_OS_VFS_H
#define SDK_VFS_OS_VFS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);

typedef enum {
    VFS_NODE_FILE = 0,
    VFS_NODE_DIR
} vfs_node_type_t;

typedef enum {
    VFS_MIME_UNKNOWN = 0,
    VFS_MIME_AUDIO,      /* .mp3, .wav, .aac */
    VFS_MIME_IMAGE,      /* .raw, .bmp, .bin */
    VFS_MIME_TEXT,       /* .txt, .log */
    VFS_MIME_VAPP,       /* .vapp (VeebhaOS Executable App Package) */
} vfs_mime_t;

typedef struct {
    char            name[32];
    uint32_t        size_bytes;
    vfs_node_type_t type;
    vfs_mime_t      mime;
} vfs_dirent_t;

/**
 * Initialize Virtual File System storage structures.
 */
void vfs_init(void);

/**
 * Open a directory for traversal.
 *
 * @param path Directory path (e.g. "/sdcard" or "/sdcard/Music").
 * @param out_handle Pointer to receive the allocated directory iterator handle.
 * @return true if directory exists and was opened, false otherwise.
 */
bool vfs_opendir(const char *path, void **out_handle);

/**
 * Read the next directory entry from an open directory handle.
 *
 * @param handle Directory handle returned by vfs_opendir.
 * @param out_entry Output buffer populated with the directory entry info.
 * @return true if an entry was read, false if no more entries exist or on error.
 */
bool vfs_readdir(void *handle, vfs_dirent_t *out_entry);

/**
 * Close an open directory handle and release internal resources.
 *
 * @param handle Directory handle returned by vfs_opendir.
 */
void vfs_closedir(void *handle);

/**
 * Detect file MIME classification from file extension.
 *
 * @param filename Name or path of file.
 * @return Detected vfs_mime_t enum value.
 */
vfs_mime_t vfs_detect_mime(const char *filename);

/**
 * Format raw byte count into human-readable representation (e.g. "4.8 MB", "340 KB", "12 B").
 *
 * @param bytes Number of bytes.
 * @param buf Output string buffer.
 * @param buf_size Size of output buffer.
 */
void vfs_format_size(uint32_t bytes, char *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif /* SDK_VFS_OS_VFS_H */
