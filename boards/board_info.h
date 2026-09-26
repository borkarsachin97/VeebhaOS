/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Universal Board Information Contract
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BOARDS_BOARD_INFO_H
#define BOARDS_BOARD_INFO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "veebha_version.h"

typedef struct {
    const char *brand;
    const char *model;
    const char *codename;
    const char *cpu_name;
    const char *arch;
    uint32_t    cpu_clock_mhz;
    uint32_t    ram_size_kb;
    uint32_t    flash_size_kb;
    uint16_t    display_width;
    uint16_t    display_height;
    const char *display_controller;
    const char *color_format;
    const char *gpu_blitter;
    const char *modem_type;
    const char *connectivity_chip;
    const char *os_version;
    const char *os_developer;
    const char *build_author;
    const char *build_timestamp;
} board_info_t;

const board_info_t * board_get_info(void);
const board_info_t * board_info_get(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_BOARD_INFO_H */
