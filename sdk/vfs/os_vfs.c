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

#include "os_vfs.h"
#include "sdk/include/veebha_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "OS_VFS"

/* Storage Categories Root */
static const vfs_dirent_t s_storage_categories[] = {
    { .name = "Internal",         .size_bytes = 0, .type = VFS_NODE_DIR, .mime = VFS_MIME_UNKNOWN },
    { .name = "SD Card",          .size_bytes = 0, .type = VFS_NODE_DIR, .mime = VFS_MIME_UNKNOWN },
    { .name = "External SD Card", .size_bytes = 0, .type = VFS_NODE_DIR, .mime = VFS_MIME_UNKNOWN },
    { .name = "Other",            .size_bytes = 0, .type = VFS_NODE_DIR, .mime = VFS_MIME_UNKNOWN },
};

/* Mock Directory Table Entries */
static const vfs_dirent_t s_internal_entries[] = {
    { .name = "Wallpapers", .size_bytes = 0, .type = VFS_NODE_DIR,  .mime = VFS_MIME_UNKNOWN },
    { .name = "Documents",  .size_bytes = 0, .type = VFS_NODE_DIR,  .mime = VFS_MIME_UNKNOWN },
    { .name = "Apps",       .size_bytes = 0, .type = VFS_NODE_DIR,  .mime = VFS_MIME_UNKNOWN },
    { .name = "System",     .size_bytes = 0, .type = VFS_NODE_DIR,  .mime = VFS_MIME_UNKNOWN },
};

static const vfs_dirent_t s_sdcard_entries[] = {
    { .name = "Music",      .size_bytes = 0, .type = VFS_NODE_DIR,  .mime = VFS_MIME_UNKNOWN },
    { .name = "Recordings", .size_bytes = 0, .type = VFS_NODE_DIR,  .mime = VFS_MIME_UNKNOWN },
    { .name = "Photos",     .size_bytes = 0, .type = VFS_NODE_DIR,  .mime = VFS_MIME_UNKNOWN },
    { .name = "Documents",  .size_bytes = 0, .type = VFS_NODE_DIR,  .mime = VFS_MIME_UNKNOWN },
    { .name = "Downloads",  .size_bytes = 0, .type = VFS_NODE_DIR,  .mime = VFS_MIME_UNKNOWN },
    { .name = "Backup",     .size_bytes = 0, .type = VFS_NODE_DIR,  .mime = VFS_MIME_UNKNOWN },
};

static const vfs_dirent_t s_extsd_entries[] = {
    { .name = "DCIM",          .size_bytes = 0,       .type = VFS_NODE_DIR,  .mime = VFS_MIME_UNKNOWN },
    { .name = "Music",         .size_bytes = 0,       .type = VFS_NODE_DIR,  .mime = VFS_MIME_UNKNOWN },
    { .name = "Documents",     .size_bytes = 0,       .type = VFS_NODE_DIR,  .mime = VFS_MIME_UNKNOWN },
    { .name = "Android",       .size_bytes = 0,       .type = VFS_NODE_DIR,  .mime = VFS_MIME_UNKNOWN },
    { .name = "card_info.txt", .size_bytes = 256,     .type = VFS_NODE_FILE, .mime = VFS_MIME_TEXT },
    { .name = "ext_demo.mp3",  .size_bytes = 4194304, .type = VFS_NODE_FILE, .mime = VFS_MIME_AUDIO },
};

static const vfs_dirent_t s_extsd_dcim[] = {
    { .name = "IMG_0001.raw",  .size_bytes = 78848,   .type = VFS_NODE_FILE, .mime = VFS_MIME_IMAGE },
    { .name = "IMG_0002.jpg",  .size_bytes = 65420,   .type = VFS_NODE_FILE, .mime = VFS_MIME_IMAGE },
};

static const vfs_dirent_t s_extsd_music[] = {
    { .name = "Track_SD01.mp3", .size_bytes = 4194304, .type = VFS_NODE_FILE, .mime = VFS_MIME_AUDIO },
    { .name = "Track_SD02.mp3", .size_bytes = 5242880, .type = VFS_NODE_FILE, .mime = VFS_MIME_AUDIO },
};

static const vfs_dirent_t s_extsd_docs[] = {
    { .name = "SD_Readme.txt",  .size_bytes = 1024,    .type = VFS_NODE_FILE, .mime = VFS_MIME_TEXT },
};

static const vfs_dirent_t s_extsd_android[] = {
    { .name = "data.bin",       .size_bytes = 2048,    .type = VFS_NODE_FILE, .mime = VFS_MIME_UNKNOWN },
};

static const vfs_dirent_t s_other_entries[] = {
    { .name = "NVRAM",      .size_bytes = 0, .type = VFS_NODE_DIR,  .mime = VFS_MIME_UNKNOWN },
    { .name = "Partitions", .size_bytes = 0, .type = VFS_NODE_DIR,  .mime = VFS_MIME_UNKNOWN },
    { .name = "OTG_Drive",  .size_bytes = 0, .type = VFS_NODE_DIR,  .mime = VFS_MIME_UNKNOWN },
};

static const vfs_dirent_t s_music_entries[] = {
    { .name = "01_Blinding_Lights.mp3", .size_bytes = 5033165, .type = VFS_NODE_FILE, .mime = VFS_MIME_AUDIO },
    { .name = "02_Midnight_City.mp3",   .size_bytes = 5452595, .type = VFS_NODE_FILE, .mime = VFS_MIME_AUDIO },
    { .name = "03_Resonance.mp3",       .size_bytes = 4089446, .type = VFS_NODE_FILE, .mime = VFS_MIME_AUDIO },
};

static const vfs_dirent_t s_recordings_entries[] = {
    { .name = "Voice_001.wav",          .size_bytes = 348160,  .type = VFS_NODE_FILE, .mime = VFS_MIME_AUDIO },
};

static const vfs_dirent_t s_photos_entries[] = {
    { .name = "Wallpaper_Cyber.raw",    .size_bytes = 78848,   .type = VFS_NODE_FILE, .mime = VFS_MIME_IMAGE },
    { .name = "Sunset_Beach.jpg",       .size_bytes = 65420,   .type = VFS_NODE_FILE, .mime = VFS_MIME_IMAGE },
    { .name = "Night_City.raw",         .size_bytes = 82150,   .type = VFS_NODE_FILE, .mime = VFS_MIME_IMAGE },
};

static const vfs_dirent_t s_documents_entries[] = {
    { .name = "ReadMe.txt",             .size_bytes = 1228,    .type = VFS_NODE_FILE, .mime = VFS_MIME_TEXT  },
    { .name = "License.txt",            .size_bytes = 1084,    .type = VFS_NODE_FILE, .mime = VFS_MIME_TEXT  },
};

static const vfs_dirent_t s_wallpapers_entries[] = {
    { .name = "default.bmp",            .size_bytes = 77494,   .type = VFS_NODE_FILE, .mime = VFS_MIME_IMAGE },
    { .name = "Abstract.bmp",           .size_bytes = 77494,   .type = VFS_NODE_FILE, .mime = VFS_MIME_IMAGE },
    { .name = "Neon.bmp",               .size_bytes = 77494,   .type = VFS_NODE_FILE, .mime = VFS_MIME_IMAGE },
    { .name = "Nature.bmp",             .size_bytes = 77494,   .type = VFS_NODE_FILE, .mime = VFS_MIME_IMAGE },
};

static const vfs_dirent_t s_vapps_entries[] = {
    { .name = "brick_breaker.vapp",  .size_bytes = 16384, .type = VFS_NODE_FILE, .mime = VFS_MIME_VAPP },
    { .name = "unit_converter.vapp", .size_bytes = 12288, .type = VFS_NODE_FILE, .mime = VFS_MIME_VAPP },
    { .name = "morse_flasher.vapp",  .size_bytes = 8192,  .type = VFS_NODE_FILE, .mime = VFS_MIME_VAPP },
    { .name = "chip_synth.vapp",     .size_bytes = 14336, .type = VFS_NODE_FILE, .mime = VFS_MIME_VAPP },
    { .name = "sys_monitor.vapp",    .size_bytes = 10240, .type = VFS_NODE_FILE, .mime = VFS_MIME_VAPP },
};

static const vfs_dirent_t s_system_entries[] = {
    { .name = "os_kernel.bin",       .size_bytes = 524288, .type = VFS_NODE_FILE, .mime = VFS_MIME_UNKNOWN },
    { .name = "boot_config.sys",     .size_bytes = 512,    .type = VFS_NODE_FILE, .mime = VFS_MIME_TEXT },
    { .name = "build_info.txt",      .size_bytes = 1024,   .type = VFS_NODE_FILE, .mime = VFS_MIME_TEXT },
};

static const vfs_dirent_t s_downloads_entries[] = {
    { .name = "Track_Demo.mp3",      .size_bytes = 3145728, .type = VFS_NODE_FILE, .mime = VFS_MIME_AUDIO },
    { .name = "ReleaseNotes.txt",    .size_bytes = 2048,    .type = VFS_NODE_FILE, .mime = VFS_MIME_TEXT },
};

static const vfs_dirent_t s_backup_entries[] = {
    { .name = "contacts_backup.vcf", .size_bytes = 4096,    .type = VFS_NODE_FILE, .mime = VFS_MIME_TEXT },
};

static const vfs_dirent_t s_nvram_entries[] = {
    { .name = "sys_config.nv",       .size_bytes = 256,     .type = VFS_NODE_FILE, .mime = VFS_MIME_TEXT },
    { .name = "bt_bonded.nv",        .size_bytes = 512,     .type = VFS_NODE_FILE, .mime = VFS_MIME_TEXT },
    { .name = "call_log.nv",         .size_bytes = 1024,    .type = VFS_NODE_FILE, .mime = VFS_MIME_TEXT },
    { .name = "user_profile.nv",     .size_bytes = 128,     .type = VFS_NODE_FILE, .mime = VFS_MIME_TEXT },
};

static const vfs_dirent_t s_partitions_entries[] = {
    { .name = "bootloader.bin",      .size_bytes = 65536,   .type = VFS_NODE_FILE, .mime = VFS_MIME_UNKNOWN },
    { .name = "kernel_image.bin",    .size_bytes = 524288,  .type = VFS_NODE_FILE, .mime = VFS_MIME_UNKNOWN },
    { .name = "rootfs.bin",          .size_bytes = 2097152, .type = VFS_NODE_FILE, .mime = VFS_MIME_UNKNOWN },
};

static const vfs_dirent_t s_otg_entries[] = {
    { .name = "usb_otg_status.txt",  .size_bytes = 64,      .type = VFS_NODE_FILE, .mime = VFS_MIME_TEXT },
};

#include "sdk/include/veebha_hardware.h"

#define VFS_MAX_DYNAMIC_ENTRIES 32

#pragma pack(push, 1)
typedef struct {
    uint8_t  name[11];
    uint8_t  attr;
    uint8_t  nt_res;
    uint8_t  crt_time_tenth;
    uint16_t crt_time;
    uint16_t crt_date;
    uint16_t lst_acc_date;
    uint16_t fst_clus_hi;
    uint16_t wrt_time;
    uint16_t wrt_date;
    uint16_t fst_clus_lo;
    uint32_t file_size;
} fat_raw_entry_t;

typedef struct {
    uint8_t  order;
    uint16_t name1[5];
    uint8_t  attr;       /* 0x0F */
    uint8_t  type;
    uint8_t  checksum;
    uint16_t name2[6];
    uint16_t first_cluster; /* 0 */
    uint16_t name3[2];
} fat_raw_lfn_t;
#pragma pack(pop)

static inline uint16_t read_le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static inline uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

typedef struct {
    bool     valid;
    bool     is_fat32;
    uint32_t lba_start;
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint32_t fat_size;
    uint16_t root_entry_count;
    uint32_t root_cluster;       /* FAT32 */
    uint32_t root_dir_sector;    /* FAT16 */
    uint32_t root_dir_sectors;   /* FAT16 */
    uint32_t first_data_sector;
} fat_volume_t;

static fat_volume_t s_fat_vol;
static bool s_fat_mounted = false;

static bool fat_mount(void)
{
    if (!veebha_hw_sdcard_present()) {
        s_fat_mounted = false;
        return false;
    }

    uint8_t sec[512];
    if (!veebha_hw_sdcard_read_blocks(0, sec, 1)) {
        s_fat_mounted = false;
        return false;
    }

    if (sec[510] != 0x55 || sec[511] != 0xAA) {
        s_fat_mounted = false;
        return false;
    }

    uint32_t lba_start = 0;
    uint16_t bps = read_le16(&sec[11]);

    if (bps == 512 && sec[13] > 0 && (sec[13] & (sec[13] - 1)) == 0 && sec[16] >= 1) {
        lba_start = 0;
    } else {
        uint32_t part_lba = read_le32(&sec[0x1BE + 8]);
        uint32_t part_size = read_le32(&sec[0x1BE + 12]);
        if (part_lba > 0 && part_size > 0) {
            lba_start = part_lba;
            if (!veebha_hw_sdcard_read_blocks(lba_start, sec, 1)) {
                s_fat_mounted = false;
                return false;
            }
            if (sec[510] != 0x55 || sec[511] != 0xAA) {
                s_fat_mounted = false;
                return false;
            }
        }
    }

    bps = read_le16(&sec[11]);
    if (bps != 512) bps = 512;
    uint8_t spc = sec[13];
    if (spc == 0) spc = 8;
    uint16_t rsvd = read_le16(&sec[14]);
    uint8_t fats = sec[16];
    if (fats == 0) fats = 2;
    uint16_t root_cnt = read_le16(&sec[17]);
    uint16_t fat_sz16 = read_le16(&sec[22]);
    uint32_t fat_sz32 = read_le32(&sec[36]);
    uint32_t fat_size = fat_sz16 ? (uint32_t)fat_sz16 : fat_sz32;

    bool is_fat32 = (root_cnt == 0 || fat_sz16 == 0);
    uint32_t root_clus = is_fat32 ? read_le32(&sec[44]) : 0;
    if (is_fat32 && root_clus == 0) root_clus = 2;

    s_fat_vol.valid = true;
    s_fat_vol.is_fat32 = is_fat32;
    s_fat_vol.lba_start = lba_start;
    s_fat_vol.bytes_per_sector = bps;
    s_fat_vol.sectors_per_cluster = spc;
    s_fat_vol.reserved_sectors = rsvd;
    s_fat_vol.num_fats = fats;
    s_fat_vol.fat_size = fat_size;
    s_fat_vol.root_entry_count = root_cnt;
    s_fat_vol.root_cluster = root_clus;

    if (is_fat32) {
        s_fat_vol.first_data_sector = lba_start + rsvd + (fats * fat_size);
        s_fat_vol.root_dir_sector = s_fat_vol.first_data_sector + (root_clus - 2) * spc;
        s_fat_vol.root_dir_sectors = spc;
    } else {
        s_fat_vol.root_dir_sectors = ((root_cnt * 32) + (bps - 1)) / bps;
        s_fat_vol.root_dir_sector = lba_start + rsvd + (fats * fat_size);
        s_fat_vol.first_data_sector = s_fat_vol.root_dir_sector + s_fat_vol.root_dir_sectors;
    }

    s_fat_mounted = true;
    OS_LOGI(TAG, "FAT SD Card Mounted: %s (LBA=%u, Cluster=%u sectors, RootSec=%u)",
            is_fat32 ? "FAT32" : "FAT16", (unsigned int)lba_start, (unsigned int)spc, (unsigned int)s_fat_vol.root_dir_sector);
    return true;
}

static void format_fat_83_name(const uint8_t *raw_name, char *out_name, size_t out_size)
{
    char base[9];
    char ext[4];
    int b_len = 0, e_len = 0;

    for (int i = 0; i < 8; i++) {
        if (raw_name[i] != ' ') {
            base[b_len++] = (char)raw_name[i];
        }
    }
    base[b_len] = '\0';

    for (int i = 8; i < 11; i++) {
        if (raw_name[i] != ' ') {
            ext[e_len++] = (char)raw_name[i];
        }
    }
    ext[e_len] = '\0';

    if (e_len > 0) {
        snprintf(out_name, out_size, "%s.%s", base, ext);
    } else {
        snprintf(out_name, out_size, "%s", base);
    }
}

static void extract_lfn_chars(const fat_raw_lfn_t *lfn, char *lfn_buf, size_t lfn_buf_size)
{
    uint8_t order = lfn->order;
    uint8_t seq = order & 0x1F;
    if (seq == 0 || seq > 8) return;

    size_t start_pos = (seq - 1) * 13;
    if (start_pos + 13 >= lfn_buf_size) return;

    size_t cur = start_pos;
    for (int i = 0; i < 5; i++) {
        uint16_t c = lfn->name1[i];
        if (c == 0x0000 || c == 0xFFFF) break;
        lfn_buf[cur++] = (c < 128) ? (char)c : '?';
    }
    if (lfn->name1[4] != 0x0000 && lfn->name1[4] != 0xFFFF) {
        for (int i = 0; i < 6; i++) {
            uint16_t c = lfn->name2[i];
            if (c == 0x0000 || c == 0xFFFF) break;
            lfn_buf[cur++] = (c < 128) ? (char)c : '?';
        }
    }
    if (lfn->name2[5] != 0x0000 && lfn->name2[5] != 0xFFFF) {
        for (int i = 0; i < 2; i++) {
            uint16_t c = lfn->name3[i];
            if (c == 0x0000 || c == 0xFFFF) break;
            lfn_buf[cur++] = (c < 128) ? (char)c : '?';
        }
    }
    lfn_buf[cur] = '\0';
}

static uint16_t fat_scan_directory(uint32_t start_sector, uint32_t num_sectors, vfs_dirent_t *out_entries, uint16_t max_entries)
{
    uint8_t sec_buf[512];
    char lfn_buf[64];
    lfn_buf[0] = '\0';
    uint16_t count = 0;

    for (uint32_t s = 0; s < num_sectors && count < max_entries; s++) {
        if (!veebha_hw_sdcard_read_blocks(start_sector + s, sec_buf, 1)) {
            break;
        }

        for (int i = 0; i < 16 && count < max_entries; i++) {
            const uint8_t *rec = &sec_buf[i * 32];
            uint8_t first_byte = rec[0];

            if (first_byte == 0x00) {
                return count;
            }
            if (first_byte == 0xE5) {
                lfn_buf[0] = '\0';
                continue;
            }

            uint8_t attr = rec[11];
            if (attr == 0x0F) {
                const fat_raw_lfn_t *lfn = (const fat_raw_lfn_t *)rec;
                extract_lfn_chars(lfn, lfn_buf, sizeof(lfn_buf));
                continue;
            }

            if (attr & 0x08) {
                lfn_buf[0] = '\0';
                continue;
            }

            char filename[32];
            if (lfn_buf[0] != '\0') {
                strncpy(filename, lfn_buf, sizeof(filename) - 1);
                filename[sizeof(filename) - 1] = '\0';
            } else {
                format_fat_83_name(rec, filename, sizeof(filename));
            }
            lfn_buf[0] = '\0';

            if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
                continue;
            }

            bool is_dir = (attr & 0x10) != 0;
            const fat_raw_entry_t *entry = (const fat_raw_entry_t *)rec;
            uint32_t size = is_dir ? 0 : read_le32((const uint8_t *)&entry->file_size);

            vfs_dirent_t *d = &out_entries[count++];
            strncpy(d->name, filename, sizeof(d->name) - 1);
            d->name[sizeof(d->name) - 1] = '\0';
            d->type = is_dir ? VFS_NODE_DIR : VFS_NODE_FILE;
            d->size_bytes = size;
            d->mime = is_dir ? VFS_MIME_UNKNOWN : vfs_detect_mime(filename);
        }
    }
    return count;
}

static uint32_t fat_find_subdir_cluster(uint32_t parent_start_sec, uint32_t parent_num_secs, const char *dirname)
{
    uint8_t sec_buf[512];
    char lfn_buf[64];
    lfn_buf[0] = '\0';

    for (uint32_t s = 0; s < parent_num_secs; s++) {
        if (!veebha_hw_sdcard_read_blocks(parent_start_sec + s, sec_buf, 1)) {
            break;
        }

        for (int i = 0; i < 16; i++) {
            const uint8_t *rec = &sec_buf[i * 32];
            uint8_t first_byte = rec[0];
            if (first_byte == 0x00) return 0;
            if (first_byte == 0xE5) { lfn_buf[0] = '\0'; continue; }

            uint8_t attr = rec[11];
            if (attr == 0x0F) {
                const fat_raw_lfn_t *lfn = (const fat_raw_lfn_t *)rec;
                extract_lfn_chars(lfn, lfn_buf, sizeof(lfn_buf));
                continue;
            }
            if (attr & 0x08) { lfn_buf[0] = '\0'; continue; }

            char filename[32];
            if (lfn_buf[0] != '\0') {
                strncpy(filename, lfn_buf, sizeof(filename) - 1);
                filename[sizeof(filename) - 1] = '\0';
            } else {
                format_fat_83_name(rec, filename, sizeof(filename));
            }
            lfn_buf[0] = '\0';

            if ((attr & 0x10) && strcasecmp(filename, dirname) == 0) {
                const fat_raw_entry_t *entry = (const fat_raw_entry_t *)rec;
                uint32_t clus = ((uint32_t)read_le16((const uint8_t *)&entry->fst_clus_hi) << 16) |
                                 (uint32_t)read_le16((const uint8_t *)&entry->fst_clus_lo);
                return clus;
            }
        }
    }
    return 0;
}

typedef struct {
    char         path[64];
    uint16_t     index;
    uint16_t     count;
    vfs_dirent_t dynamic_entries[VFS_MAX_DYNAMIC_ENTRIES];
    const vfs_dirent_t *entries;
} vfs_dir_handle_t;

void vfs_init(void)
{
    fat_mount();
    OS_LOGI(TAG, "Virtual File System initialized (Mounts: /internal, /sdcard, /extsd, /other, /vapps)");
}

static void normalize_path(const char *src, char *dst, size_t dst_size)
{
    if (!src || !dst || dst_size == 0) return;
    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';

    size_t len = strlen(dst);
    /* Remove trailing slash unless it's just "/" */
    while (len > 1 && dst[len - 1] == '/') {
        dst[len - 1] = '\0';
        len--;
    }
}

bool vfs_opendir(const char *path, void **out_handle)
{
    if (!path || !out_handle) return false;

    char norm[64];
    normalize_path(path, norm, sizeof(norm));

    const vfs_dirent_t *entries = NULL;
    uint16_t count = 0;
    bool is_dynamic = false;
    vfs_dirent_t dyn_buf[VFS_MAX_DYNAMIC_ENTRIES];

    /* Dynamic Hardware SD Card Handling for /extsd */
    if (strncasecmp(norm, "/extsd", 6) == 0 || strncasecmp(norm, "extsd", 5) == 0) {
        if (!s_fat_mounted) {
            fat_mount();
        }

        if (s_fat_mounted) {
            const char *sub = NULL;
            if (strncasecmp(norm, "/extsd", 6) == 0) {
                sub = norm + 6;
            } else {
                sub = norm + 5;
            }
            while (*sub == '/') sub++;

            uint32_t cur_sec = s_fat_vol.root_dir_sector;
            uint32_t cur_num_secs = s_fat_vol.root_dir_sectors;
            bool lookup_ok = true;

            if (*sub != '\0') {
                const char *p = sub;
                while (*p != '\0') {
                    while (*p == '/') p++;
                    if (*p == '\0') break;

                    char token[32];
                    size_t t_idx = 0;
                    while (*p != '\0' && *p != '/' && t_idx < sizeof(token) - 1) {
                        token[t_idx++] = *p++;
                    }
                    token[t_idx] = '\0';

                    uint32_t clus = fat_find_subdir_cluster(cur_sec, cur_num_secs, token);
                    if (clus >= 2) {
                        cur_sec = s_fat_vol.first_data_sector + (clus - 2) * s_fat_vol.sectors_per_cluster;
                        cur_num_secs = s_fat_vol.sectors_per_cluster;
                    } else {
                        lookup_ok = false;
                        break;
                    }
                }
            }

            if (lookup_ok) {
                uint16_t fat_cnt = fat_scan_directory(cur_sec, cur_num_secs, dyn_buf, VFS_MAX_DYNAMIC_ENTRIES);
                if (fat_cnt > 0) {
                    is_dynamic = true;
                    count = fat_cnt;
                    OS_LOGI(TAG, "Hardware SD card scanned %u real entries for '%s'", count, norm);
                }
            }
        }
    }

    if (!is_dynamic) {
        if (strcmp(norm, "/") == 0 || strcmp(norm, "") == 0) {
            entries = s_storage_categories;
            count = (uint16_t)(sizeof(s_storage_categories) / sizeof(s_storage_categories[0]));
        } else if (strcasecmp(norm, "/internal") == 0) {
            entries = s_internal_entries;
            count = (uint16_t)(sizeof(s_internal_entries) / sizeof(s_internal_entries[0]));
        } else if (strcasecmp(norm, "/internal/Wallpapers") == 0) {
            entries = s_wallpapers_entries;
            count = (uint16_t)(sizeof(s_wallpapers_entries) / sizeof(s_wallpapers_entries[0]));
        } else if (strcasecmp(norm, "/internal/Documents") == 0) {
            entries = s_documents_entries;
            count = (uint16_t)(sizeof(s_documents_entries) / sizeof(s_documents_entries[0]));
        } else if (strcasecmp(norm, "/internal/Apps") == 0 || strcasecmp(norm, "/vapps") == 0 || strcasecmp(norm, "vapps") == 0) {
            entries = s_vapps_entries;
            count = (uint16_t)(sizeof(s_vapps_entries) / sizeof(s_vapps_entries[0]));
        } else if (strcasecmp(norm, "/internal/System") == 0) {
            entries = s_system_entries;
            count = (uint16_t)(sizeof(s_system_entries) / sizeof(s_system_entries[0]));
        } else if (strcasecmp(norm, "/sdcard") == 0 || strcasecmp(norm, "sdcard") == 0 || strcasecmp(norm, "/sdcard/") == 0) {
            entries = s_sdcard_entries;
            count = (uint16_t)(sizeof(s_sdcard_entries) / sizeof(s_sdcard_entries[0]));
        } else if (strcasecmp(norm, "/sdcard/Music") == 0) {
            entries = s_music_entries;
            count = (uint16_t)(sizeof(s_music_entries) / sizeof(s_music_entries[0]));
        } else if (strcasecmp(norm, "/sdcard/Recordings") == 0) {
            entries = s_recordings_entries;
            count = (uint16_t)(sizeof(s_recordings_entries) / sizeof(s_recordings_entries[0]));
        } else if (strcasecmp(norm, "/sdcard/Photos") == 0) {
            entries = s_photos_entries;
            count = (uint16_t)(sizeof(s_photos_entries) / sizeof(s_photos_entries[0]));
        } else if (strcasecmp(norm, "/sdcard/Documents") == 0) {
            entries = s_documents_entries;
            count = (uint16_t)(sizeof(s_documents_entries) / sizeof(s_documents_entries[0]));
        } else if (strcasecmp(norm, "/sdcard/Wallpapers") == 0) {
            entries = s_wallpapers_entries;
            count = (uint16_t)(sizeof(s_wallpapers_entries) / sizeof(s_wallpapers_entries[0]));
        } else if (strcasecmp(norm, "/sdcard/Downloads") == 0) {
            entries = s_downloads_entries;
            count = (uint16_t)(sizeof(s_downloads_entries) / sizeof(s_downloads_entries[0]));
        } else if (strcasecmp(norm, "/sdcard/Backup") == 0) {
            entries = s_backup_entries;
            count = (uint16_t)(sizeof(s_backup_entries) / sizeof(s_backup_entries[0]));
        } else if (strcasecmp(norm, "/extsd") == 0 || strcasecmp(norm, "extsd") == 0 || strcasecmp(norm, "/extsd/") == 0) {
            entries = s_extsd_entries;
            count = (uint16_t)(sizeof(s_extsd_entries) / sizeof(s_extsd_entries[0]));
        } else if (strcasecmp(norm, "/extsd/DCIM") == 0) {
            entries = s_extsd_dcim;
            count = (uint16_t)(sizeof(s_extsd_dcim) / sizeof(s_extsd_dcim[0]));
        } else if (strcasecmp(norm, "/extsd/Music") == 0) {
            entries = s_extsd_music;
            count = (uint16_t)(sizeof(s_extsd_music) / sizeof(s_extsd_music[0]));
        } else if (strcasecmp(norm, "/extsd/Documents") == 0) {
            entries = s_extsd_docs;
            count = (uint16_t)(sizeof(s_extsd_docs) / sizeof(s_extsd_docs[0]));
        } else if (strcasecmp(norm, "/extsd/Android") == 0) {
            entries = s_extsd_android;
            count = (uint16_t)(sizeof(s_extsd_android) / sizeof(s_extsd_android[0]));
        } else if (strcasecmp(norm, "/other") == 0) {
            entries = s_other_entries;
            count = (uint16_t)(sizeof(s_other_entries) / sizeof(s_other_entries[0]));
        } else if (strcasecmp(norm, "/other/NVRAM") == 0) {
            entries = s_nvram_entries;
            count = (uint16_t)(sizeof(s_nvram_entries) / sizeof(s_nvram_entries[0]));
        } else if (strcasecmp(norm, "/other/Partitions") == 0) {
            entries = s_partitions_entries;
            count = (uint16_t)(sizeof(s_partitions_entries) / sizeof(s_partitions_entries[0]));
        } else if (strcasecmp(norm, "/other/OTG_Drive") == 0) {
            entries = s_otg_entries;
            count = (uint16_t)(sizeof(s_otg_entries) / sizeof(s_otg_entries[0]));
        } else {
            OS_LOGW(TAG, "Directory not found: %s", path);
            return false;
        }
    }

    vfs_dir_handle_t *h = (vfs_dir_handle_t *)calloc(1, sizeof(vfs_dir_handle_t));
    if (!h) return false;

    strncpy(h->path, norm, sizeof(h->path) - 1);
    h->index = 0;
    h->count = count;

    if (is_dynamic) {
        memcpy(h->dynamic_entries, dyn_buf, count * sizeof(vfs_dirent_t));
        h->entries = h->dynamic_entries;
    } else {
        h->entries = entries;
    }

    *out_handle = h;
    return true;
}

bool vfs_readdir(void *handle, vfs_dirent_t *out_entry)
{
    if (!handle || !out_entry) return false;
    vfs_dir_handle_t *h = (vfs_dir_handle_t *)handle;

    if (h->index < h->count) {
        *out_entry = h->entries[h->index];
        h->index++;
        return true;
    }

    return false;
}

void vfs_closedir(void *handle)
{
    if (handle) {
        free(handle);
    }
}

vfs_mime_t vfs_detect_mime(const char *filename)
{
    if (!filename) return VFS_MIME_UNKNOWN;

    const char *dot = strrchr(filename, '.');
    if (!dot) return VFS_MIME_UNKNOWN;

    if (strcasecmp(dot, ".mp3") == 0 ||
        strcasecmp(dot, ".wav") == 0 ||
        strcasecmp(dot, ".aac") == 0 ||
        strcasecmp(dot, ".ogg") == 0 ||
        strcasecmp(dot, ".flac") == 0) {
        return VFS_MIME_AUDIO;
    }

    if (strcasecmp(dot, ".raw") == 0 ||
        strcasecmp(dot, ".bmp") == 0 ||
        strcasecmp(dot, ".bin") == 0 ||
        strcasecmp(dot, ".png") == 0 ||
        strcasecmp(dot, ".jpg") == 0 ||
        strcasecmp(dot, ".jpeg") == 0) {
        return VFS_MIME_IMAGE;
    }

    if (strcasecmp(dot, ".txt") == 0 ||
        strcasecmp(dot, ".log") == 0 ||
        strcasecmp(dot, ".c") == 0 ||
        strcasecmp(dot, ".h") == 0) {
        return VFS_MIME_TEXT;
    }

    if (strcasecmp(dot, ".vapp") == 0) {
        return VFS_MIME_VAPP;
    }

    return VFS_MIME_UNKNOWN;
}

void vfs_format_size(uint32_t bytes, char *buf, size_t buf_size)
{
    if (!buf || buf_size == 0) return;

    if (bytes < 1024) {
        snprintf(buf, buf_size, "%u B", (unsigned int)bytes);
    } else if (bytes < 1024 * 1024) {
        uint32_t kb = bytes / 1024;
        uint32_t dec = ((bytes % 1024) * 10) / 1024;
        if (dec == 0) {
            snprintf(buf, buf_size, "%u KB", (unsigned int)kb);
        } else {
            snprintf(buf, buf_size, "%u.%u KB", (unsigned int)kb, (unsigned int)dec);
        }
    } else {
        uint32_t mb = bytes / (1024 * 1024);
        uint32_t dec = (((bytes % (1024 * 1024)) * 10) / (1024 * 1024));
        snprintf(buf, buf_size, "%u.%u MB", (unsigned int)mb, (unsigned int)dec);
    }
}
