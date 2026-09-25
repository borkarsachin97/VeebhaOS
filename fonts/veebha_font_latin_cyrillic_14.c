/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "fonts/veebha_font_latin_cyrillic_14.h"
#include "fonts/veebha_font_devanagari_16.h"

/* 1-bpp monochrome Latin & Cyrillic 14px bitmap array */
static const uint8_t glyph_bitmap_latin_cyrillic_14[] __attribute__((section(".rodata.fonts"))) = {
    0xFC, 0xC0, 0xB6, 0xD0, 0x09, 0x02, 0x40, 0x91, 0xFF, 0x12, 0x04, 0x8F, 0xF8, 0x98, 0x24, 0x09,
    0x00, 0x10, 0x21, 0xF4, 0x99, 0x1A, 0x1F, 0x0B, 0x13, 0x25, 0xF0, 0x81, 0x00, 0x70, 0x88, 0x88,
    0x89, 0x08, 0xB0, 0x8A, 0xE7, 0x51, 0x0D, 0x10, 0x91, 0x11, 0x11, 0x0E, 0x38, 0x11, 0x04, 0x00,
    0x80, 0x50, 0x22, 0x18, 0x46, 0x0A, 0x43, 0x0F, 0x30, 0xF0, 0x69, 0x49, 0x24, 0x89, 0x10, 0x89,
    0x12, 0x49, 0x29, 0x40, 0x11, 0x25, 0xF1, 0xCD, 0x62, 0x00, 0x08, 0x04, 0x02, 0x01, 0x0F, 0xF8,
    0x40, 0x20, 0x10, 0x08, 0x00, 0x58, 0xF0, 0xC0, 0x08, 0xC4, 0x23, 0x10, 0x8C, 0x42, 0x31, 0x00,
    0x38, 0x8A, 0x0C, 0x18, 0x30, 0x60, 0xC1, 0x44, 0x70, 0x65, 0x08, 0x42, 0x10, 0x84, 0x27, 0xC0,
    0x7A, 0x30, 0x41, 0x0C, 0x21, 0x08, 0x43, 0xF0, 0x7D, 0x0C, 0x08, 0x13, 0xC0, 0xC0, 0x81, 0x86,
    0xF0, 0x0C, 0x28, 0x91, 0x24, 0x50, 0xBF, 0x82, 0x04, 0x08, 0xFD, 0x02, 0x07, 0xC8, 0x40, 0x40,
    0x81, 0x84, 0xF0, 0x3C, 0xC7, 0x04, 0x0B, 0xD8, 0xE0, 0xC1, 0x46, 0x78, 0xFE, 0x08, 0x10, 0x40,
    0x82, 0x04, 0x10, 0x20, 0x80, 0x7D, 0x8E, 0x0E, 0x33, 0x98, 0xE0, 0xC1, 0xC6, 0xF8, 0x79, 0x8A,
    0x0C, 0x1C, 0x6F, 0x40, 0x83, 0x8C, 0xF0, 0xC6, 0x50, 0x16, 0x00, 0x83, 0x8E, 0x1C, 0x0E, 0x01,
    0xC0, 0x1C, 0x01, 0xFF, 0x80, 0x00, 0x1F, 0xF0, 0x80, 0x38, 0x03, 0x80, 0x70, 0x38, 0x71, 0xC1,
    0x00, 0x74, 0x42, 0x33, 0x10, 0x80, 0x21, 0x00, 0x0F, 0x83, 0x04, 0x40, 0x24, 0xE9, 0x99, 0x99,
    0x09, 0x90, 0x99, 0x9A, 0x4E, 0xC4, 0x00, 0x30, 0x40, 0xF8, 0x08, 0x0A, 0x05, 0x04, 0x42, 0x21,
    0x11, 0xFC, 0x82, 0x41, 0x40, 0x40, 0xFE, 0x83, 0x81, 0x83, 0xFE, 0x83, 0x81, 0x81, 0x83, 0xFE,
    0x3E, 0x61, 0xC0, 0x80, 0x80, 0x80, 0x80, 0xC0, 0x61, 0x3E, 0xFC, 0x41, 0xA0, 0x70, 0x18, 0x0C,
    0x06, 0x03, 0x03, 0x83, 0x7E, 0x00, 0xFF, 0x02, 0x04, 0x0F, 0xF0, 0x20, 0x40, 0x81, 0xFC, 0xFE,
    0x08, 0x20, 0xFE, 0x08, 0x20, 0x82, 0x00, 0x1F, 0x30, 0x50, 0x10, 0x08, 0x04, 0x1E, 0x02, 0x81,
    0x60, 0x8F, 0x80, 0x81, 0x81, 0x81, 0x81, 0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF, 0xC0, 0x24,
    0x92, 0x49, 0x24, 0x9C, 0x83, 0x0A, 0x24, 0x8E, 0x14, 0x24, 0x44, 0x85, 0x04, 0x82, 0x08, 0x20,
    0x82, 0x08, 0x20, 0x83, 0xF0, 0xC0, 0xF0, 0x3A, 0x16, 0x85, 0x92, 0x64, 0x98, 0xC6, 0x31, 0x80,
    0x60, 0x10, 0xC1, 0xC1, 0xA1, 0x91, 0x91, 0x89, 0x89, 0x85, 0x83, 0x83, 0x3E, 0x31, 0xB0, 0x70,
    0x18, 0x0C, 0x06, 0x03, 0x83, 0x63, 0x1F, 0x00, 0xFD, 0x0E, 0x0C, 0x18, 0x7F, 0xA0, 0x40, 0x81,
    0x00, 0x3E, 0x31, 0xB0, 0x70, 0x18, 0x0C, 0x06, 0x03, 0x83, 0x63, 0x1F, 0x00, 0x80, 0x20, 0xFC,
    0x86, 0x82, 0x82, 0x86, 0xFC, 0x86, 0x82, 0x82, 0x81, 0x7D, 0x86, 0x04, 0x06, 0x03, 0x80, 0x81,
    0x86, 0xF8, 0xFF, 0x84, 0x02, 0x01, 0x00, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x00, 0x81, 0x81,
    0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x42, 0x3C, 0x80, 0xA0, 0x90, 0x48, 0x22, 0x21, 0x10, 0x50,
    0x28, 0x14, 0x04, 0x00, 0x82, 0x0C, 0x28, 0x51, 0x44, 0x8A, 0x24, 0x51, 0x14, 0x50, 0xA2, 0x85,
    0x14, 0x28, 0xA0, 0x82, 0x00, 0xC1, 0xA0, 0x88, 0x82, 0x80, 0x80, 0xC0, 0x50, 0x44, 0x41, 0x60,
    0xC0, 0xC1, 0xA0, 0x88, 0x84, 0x41, 0x40, 0x40, 0x20, 0x10, 0x08, 0x04, 0x00, 0xFF, 0x01, 0x02,
    0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0xFF, 0xF2, 0x49, 0x24, 0x92, 0x70, 0x86, 0x10, 0x86, 0x10,
    0x86, 0x10, 0x86, 0x10, 0xE4, 0x92, 0x49, 0x24, 0xF0, 0x1C, 0x1B, 0x18, 0xD8, 0x30, 0xFE, 0xC8,
    0x80, 0x7A, 0x10, 0x5F, 0x86, 0x18, 0xDD, 0x81, 0x02, 0x05, 0xCC, 0x50, 0x60, 0xC1, 0x83, 0x8A,
    0xE0, 0x39, 0x18, 0x20, 0x82, 0x04, 0x4E, 0x02, 0x04, 0x09, 0xD4, 0x70, 0x60, 0xC1, 0x82, 0x8C,
    0xE8, 0x3C, 0x8E, 0x0F, 0xF8, 0x10, 0x10, 0x9E, 0x34, 0x4F, 0x44, 0x44, 0x44, 0x40, 0x3A, 0x8E,
    0x0C, 0x18, 0x30, 0x51, 0x9D, 0x02, 0x8C, 0xF0, 0x81, 0x02, 0x05, 0xEC, 0x70, 0x60, 0xC1, 0x83,
    0x06, 0x08, 0xDF, 0xE0, 0x24, 0x12, 0x49, 0x24, 0x93, 0x80, 0x82, 0x08, 0x21, 0x8A, 0x4E, 0x28,
    0x92, 0x28, 0x40, 0xFF, 0xE0, 0xB9, 0xD8, 0xC6, 0x10, 0xC2, 0x18, 0x43, 0x08, 0x61, 0x0C, 0x21,
    0xBD, 0x8E, 0x0C, 0x18, 0x30, 0x60, 0xC1, 0x38, 0x8A, 0x0C, 0x18, 0x30, 0x51, 0x1C, 0xB9, 0x8A,
    0x0C, 0x18, 0x30, 0x71, 0x5C, 0x81, 0x02, 0x00, 0x3A, 0x8E, 0x0C, 0x18, 0x30, 0x51, 0x9D, 0x02,
    0x04, 0x08, 0xBC, 0x88, 0x88, 0x88, 0x7A, 0x18, 0x38, 0x1C, 0x18, 0x5E, 0x42, 0x3E, 0x84, 0x21,
    0x08, 0x41, 0xC0, 0x83, 0x06, 0x0C, 0x18, 0x30, 0x71, 0xBD, 0x83, 0x05, 0x12, 0x24, 0x45, 0x0A,
    0x08, 0x88, 0xC4, 0x62, 0x2A, 0xA5, 0x52, 0xA8, 0x88, 0x44, 0x82, 0x88, 0xA0, 0x81, 0x05, 0x11,
    0x41, 0x83, 0x05, 0x12, 0x22, 0x85, 0x04, 0x08, 0x10, 0x43, 0x00, 0xFC, 0x10, 0x84, 0x21, 0x08,
    0x3F, 0x19, 0x08, 0x42, 0x13, 0x04, 0x21, 0x08, 0x41, 0x80, 0xFF, 0xFC, 0xC1, 0x08, 0x42, 0x10,
    0x64, 0x21, 0x08, 0x4C, 0x00, 0x78, 0xC7, 0x80, 0x08, 0x0A, 0x05, 0x04, 0x42, 0x21, 0x11, 0xFC,
    0x82, 0x41, 0x40, 0x40, 0xFE, 0x80, 0x80, 0x80, 0xFC, 0x83, 0x81, 0x81, 0x83, 0xFE, 0xFE, 0x83,
    0x81, 0x83, 0xFE, 0x83, 0x81, 0x81, 0x83, 0xFE, 0xFE, 0x08, 0x20, 0x82, 0x08, 0x20, 0x82, 0x00,
    0x3F, 0x88, 0x22, 0x08, 0x82, 0x20, 0x88, 0x22, 0x09, 0x02, 0x40, 0xBF, 0xF8, 0x06, 0x01, 0xFF,
    0x02, 0x04, 0x0F, 0xF0, 0x20, 0x40, 0x81, 0xFC, 0x41, 0x04, 0x42, 0x10, 0x44, 0x40, 0x49, 0x00,
    0xD6, 0x02, 0x72, 0x0C, 0x46, 0x10, 0x84, 0x41, 0x05, 0x82, 0x0C, 0x7D, 0x04, 0x08, 0x13, 0xC0,
    0xC0, 0x81, 0x86, 0xF0, 0x83, 0x83, 0x85, 0x89, 0x89, 0x91, 0x91, 0xA1, 0xC1, 0xC1, 0x24, 0x18,
    0x00, 0x83, 0x83, 0x85, 0x89, 0x89, 0x91, 0x91, 0xA1, 0xC1, 0xC1, 0x82, 0x84, 0x88, 0x90, 0xB0,
    0xC8, 0x8C, 0x84, 0x82, 0x83, 0x1F, 0x88, 0x44, 0x22, 0x11, 0x08, 0x84, 0x42, 0x41, 0x60, 0xE0,
    0x40, 0xC0, 0xF0, 0x3A, 0x16, 0x85, 0x92, 0x64, 0x98, 0xC6, 0x31, 0x80, 0x60, 0x10, 0x81, 0x81,
    0x81, 0x81, 0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x3E, 0x31, 0xB0, 0x70, 0x18, 0x0C, 0x06, 0x03,
    0x83, 0x63, 0x1F, 0x00, 0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFD, 0x0E,
    0x0C, 0x18, 0x7F, 0xA0, 0x40, 0x81, 0x00, 0x3E, 0x61, 0xC0, 0x80, 0x80, 0x80, 0x80, 0xC0, 0x61,
    0x3E, 0xFF, 0x84, 0x02, 0x01, 0x00, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x00, 0x81, 0x42, 0x42,
    0x64, 0x24, 0x24, 0x18, 0x18, 0x10, 0x70, 0x04, 0x03, 0xE1, 0x93, 0x42, 0x18, 0x43, 0x08, 0x61,
    0x0A, 0x22, 0x3F, 0x80, 0x80, 0xC1, 0xA0, 0x88, 0x82, 0x80, 0x80, 0xC0, 0x50, 0x44, 0x41, 0x60,
    0xC0, 0x81, 0x40, 0xA0, 0x50, 0x28, 0x14, 0x0A, 0x05, 0x02, 0x81, 0x7F, 0xC0, 0x20, 0x10, 0x83,
    0x06, 0x0C, 0x17, 0xE0, 0x40, 0x81, 0x02, 0x04, 0x82, 0x0C, 0x10, 0x60, 0x83, 0x04, 0x18, 0x20,
    0xC1, 0x06, 0x08, 0x30, 0x41, 0x82, 0x0F, 0xFF, 0xC0, 0x82, 0x0A, 0x08, 0x28, 0x20, 0xA0, 0x82,
    0x82, 0x0A, 0x08, 0x28, 0x20, 0xA0, 0x82, 0x82, 0x0B, 0xFF, 0xF0, 0x00, 0x40, 0x01, 0xE0, 0x10,
    0x08, 0x04, 0x03, 0xF1, 0x0C, 0x82, 0x41, 0x21, 0x9F, 0x80, 0x80, 0xC0, 0x60, 0x30, 0x1F, 0xCC,
    0x36, 0x0B, 0x05, 0x86, 0xFE, 0x40, 0x81, 0x02, 0x04, 0x0F, 0xD0, 0xE0, 0xC1, 0x87, 0xF8, 0x7C,
    0x82, 0x01, 0x01, 0x7F, 0x01, 0x01, 0x03, 0x86, 0x7C, 0x87, 0xC8, 0xC6, 0x98, 0x39, 0x01, 0xF0,
    0x19, 0x01, 0x90, 0x18, 0x83, 0x8C, 0x68, 0x7C, 0x7F, 0x06, 0x0C, 0x1F, 0xE6, 0x48, 0xB1, 0x43,
    0x04, 0x7A, 0x10, 0x5F, 0x86, 0x18, 0xDD, 0x3C, 0x82, 0x07, 0xCC, 0x50, 0x60, 0xC1, 0x82, 0x88,
    0xE0, 0xFA, 0x18, 0x7E, 0x86, 0x18, 0x7E, 0xFC, 0x21, 0x08, 0x42, 0x10, 0x3E, 0x22, 0x22, 0x22,
    0x22, 0x22, 0x42, 0xFF, 0x81, 0x81, 0x3C, 0x8E, 0x0F, 0xF8, 0x10, 0x10, 0x9E, 0xC4, 0x6C, 0x98,
    0xD6, 0x0F, 0x82, 0xE8, 0xC9, 0x91, 0x14, 0x21, 0x7A, 0x10, 0x4E, 0x04, 0x18, 0xDE, 0x87, 0x0E,
    0x2C, 0x99, 0x34, 0x70, 0xE1, 0x44, 0x70, 0x04, 0x38, 0x71, 0x64, 0xC9, 0xA3, 0x87, 0x08, 0x85,
    0x12, 0x45, 0x8D, 0x91, 0x21, 0x43, 0x3E, 0x44, 0x89, 0x12, 0x24, 0x50, 0xE1, 0xC3, 0xC3, 0xE7,
    0xA5, 0xA5, 0x99, 0x99, 0x81, 0x83, 0x06, 0x0F, 0xF8, 0x30, 0x60, 0xC1, 0x38, 0x8A, 0x0C, 0x18,
    0x30, 0x51, 0x1C, 0xFF, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xC1, 0xB9, 0x8A, 0x0C, 0x18, 0x30, 0x71,
    0x5C, 0x81, 0x02, 0x00, 0x39, 0x18, 0x20, 0x82, 0x04, 0x4E, 0xFE, 0x20, 0x40, 0x81, 0x02, 0x04,
    0x08, 0x83, 0x05, 0x12, 0x22, 0x85, 0x04, 0x08, 0x10, 0x43, 0x00, 0x04, 0x00, 0x80, 0x10, 0x3A,
    0xE4, 0xE5, 0x08, 0x61, 0x0C, 0x21, 0x84, 0x29, 0xC9, 0xD7, 0x02, 0x00, 0x40, 0x08, 0x00, 0x82,
    0x88, 0xA0, 0x81, 0x05, 0x11, 0x41, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0xFF, 0x01, 0x01,
    0x86, 0x18, 0x61, 0x7C, 0x10, 0x41, 0x84, 0x30, 0x86, 0x10, 0xC2, 0x18, 0x43, 0x08, 0x61, 0x0F,
    0xFF, 0x84, 0x28, 0x42, 0x84, 0x28, 0x42, 0x84, 0x28, 0x42, 0x84, 0x2F, 0xFF, 0x00, 0x10, 0x01,
    0xF0, 0x08, 0x04, 0x03, 0xE1, 0x08, 0x84, 0x42, 0x3E, 0x81, 0x81, 0x81, 0xF9, 0x85, 0x85, 0x85,
    0xF9, 0x82, 0x08, 0x3E, 0x86, 0x18, 0x7E, 0x72, 0x20, 0x5F, 0x04, 0x18, 0x9C, 0x8F, 0x22, 0x29,
    0x07, 0xC1, 0x90, 0x64, 0x18, 0x8A, 0x1C, 0x3E, 0x85, 0x0A, 0x13, 0xE4, 0x58, 0xE1, 0x28, 0x03,
    0xFC, 0x08, 0x10, 0x3F, 0xC0, 0x81, 0x02, 0x07, 0xF0, 0x28, 0x00, 0x01, 0xE4, 0x70, 0x7F, 0xC0,
    0x80, 0x84, 0xF0, 0x6F, 0xF6
};

/* Glyph descriptions table */
static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc_latin_cyrillic_14[] __attribute__((section(".rodata.fonts"))) = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0}, /* [0] 'U+0000' */
    {.bitmap_index = 0, .adv_w = 71, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0}, /* [1] ' ' */
    {.bitmap_index = 0, .adv_w = 90, .box_w = 1, .box_h = 10, .ofs_x = 2, .ofs_y = 0}, /* [2] '!' */
    {.bitmap_index = 2, .adv_w = 103, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 6}, /* [3] '"' */
    {.bitmap_index = 4, .adv_w = 188, .box_w = 10, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [4] '#' */
    {.bitmap_index = 17, .adv_w = 142, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -2}, /* [5] '$' */
    {.bitmap_index = 29, .adv_w = 213, .box_w = 12, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* [6] '%' */
    {.bitmap_index = 44, .adv_w = 192, .box_w = 10, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [7] '&' */
    {.bitmap_index = 57, .adv_w = 62, .box_w = 1, .box_h = 4, .ofs_x = 1, .ofs_y = 6}, /* [8] ''' */
    {.bitmap_index = 58, .adv_w = 88, .box_w = 3, .box_h = 12, .ofs_x = 1, .ofs_y = -1}, /* [9] '(' */
    {.bitmap_index = 63, .adv_w = 88, .box_w = 3, .box_h = 12, .ofs_x = 1, .ofs_y = -1}, /* [10] ')' */
    {.bitmap_index = 68, .adv_w = 112, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 4}, /* [11] '*' */
    {.bitmap_index = 74, .adv_w = 188, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0}, /* [12] '+' */
    {.bitmap_index = 85, .adv_w = 71, .box_w = 2, .box_h = 3, .ofs_x = 1, .ofs_y = -1}, /* [13] ',' */
    {.bitmap_index = 86, .adv_w = 81, .box_w = 4, .box_h = 1, .ofs_x = 1, .ofs_y = 3}, /* [14] '-' */
    {.bitmap_index = 87, .adv_w = 71, .box_w = 1, .box_h = 2, .ofs_x = 2, .ofs_y = 0}, /* [15] '.' */
    {.bitmap_index = 88, .adv_w = 96, .box_w = 5, .box_h = 12, .ofs_x = 0, .ofs_y = -2}, /* [16] '/' */
    {.bitmap_index = 96, .adv_w = 142, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [17] '0' */
    {.bitmap_index = 105, .adv_w = 142, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0}, /* [18] '1' */
    {.bitmap_index = 112, .adv_w = 142, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [19] '2' */
    {.bitmap_index = 120, .adv_w = 142, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [20] '3' */
    {.bitmap_index = 129, .adv_w = 142, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [21] '4' */
    {.bitmap_index = 138, .adv_w = 142, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [22] '5' */
    {.bitmap_index = 147, .adv_w = 142, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [23] '6' */
    {.bitmap_index = 156, .adv_w = 142, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [24] '7' */
    {.bitmap_index = 165, .adv_w = 142, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [25] '8' */
    {.bitmap_index = 174, .adv_w = 142, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [26] '9' */
    {.bitmap_index = 183, .adv_w = 76, .box_w = 1, .box_h = 7, .ofs_x = 2, .ofs_y = 0}, /* [27] ':' */
    {.bitmap_index = 184, .adv_w = 76, .box_w = 2, .box_h = 8, .ofs_x = 1, .ofs_y = -1}, /* [28] ';' */
    {.bitmap_index = 186, .adv_w = 188, .box_w = 9, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [29] '<' */
    {.bitmap_index = 195, .adv_w = 188, .box_w = 9, .box_h = 4, .ofs_x = 1, .ofs_y = 3}, /* [30] '=' */
    {.bitmap_index = 200, .adv_w = 188, .box_w = 9, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [31] '>' */
    {.bitmap_index = 209, .adv_w = 119, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [32] '?' */
    {.bitmap_index = 216, .adv_w = 224, .box_w = 12, .box_h = 12, .ofs_x = 1, .ofs_y = -2}, /* [33] '@' */
    {.bitmap_index = 234, .adv_w = 153, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* [34] 'A' */
    {.bitmap_index = 246, .adv_w = 154, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [35] 'B' */
    {.bitmap_index = 256, .adv_w = 156, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [36] 'C' */
    {.bitmap_index = 266, .adv_w = 172, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [37] 'D' */
    {.bitmap_index = 278, .adv_w = 142, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [38] 'E' */
    {.bitmap_index = 287, .adv_w = 129, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [39] 'F' */
    {.bitmap_index = 295, .adv_w = 174, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [40] 'G' */
    {.bitmap_index = 307, .adv_w = 168, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [41] 'H' */
    {.bitmap_index = 317, .adv_w = 66, .box_w = 1, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [42] 'I' */
    {.bitmap_index = 319, .adv_w = 66, .box_w = 3, .box_h = 13, .ofs_x = -1, .ofs_y = -3}, /* [43] 'J' */
    {.bitmap_index = 324, .adv_w = 147, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [44] 'K' */
    {.bitmap_index = 333, .adv_w = 125, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [45] 'L' */
    {.bitmap_index = 341, .adv_w = 193, .box_w = 10, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [46] 'M' */
    {.bitmap_index = 354, .adv_w = 168, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [47] 'N' */
    {.bitmap_index = 364, .adv_w = 176, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [48] 'O' */
    {.bitmap_index = 376, .adv_w = 135, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [49] 'P' */
    {.bitmap_index = 385, .adv_w = 176, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = -2}, /* [50] 'Q' */
    {.bitmap_index = 399, .adv_w = 156, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [51] 'R' */
    {.bitmap_index = 409, .adv_w = 142, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [52] 'S' */
    {.bitmap_index = 418, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* [53] 'T' */
    {.bitmap_index = 430, .adv_w = 164, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [54] 'U' */
    {.bitmap_index = 440, .adv_w = 153, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* [55] 'V' */
    {.bitmap_index = 452, .adv_w = 222, .box_w = 13, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* [56] 'W' */
    {.bitmap_index = 469, .adv_w = 154, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* [57] 'X' */
    {.bitmap_index = 481, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* [58] 'Y' */
    {.bitmap_index = 493, .adv_w = 154, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [59] 'Z' */
    {.bitmap_index = 503, .adv_w = 88, .box_w = 3, .box_h = 12, .ofs_x = 1, .ofs_y = -1}, /* [60] '[' */
    {.bitmap_index = 508, .adv_w = 96, .box_w = 5, .box_h = 12, .ofs_x = 0, .ofs_y = -2}, /* [61] 'U+005C' */
    {.bitmap_index = 516, .adv_w = 88, .box_w = 3, .box_h = 12, .ofs_x = 1, .ofs_y = -1}, /* [62] ']' */
    {.bitmap_index = 521, .adv_w = 188, .box_w = 9, .box_h = 4, .ofs_x = 1, .ofs_y = 6}, /* [63] '^' */
    {.bitmap_index = 526, .adv_w = 112, .box_w = 7, .box_h = 1, .ofs_x = 0, .ofs_y = -3}, /* [64] '_' */
    {.bitmap_index = 527, .adv_w = 112, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 9}, /* [65] '`' */
    {.bitmap_index = 529, .adv_w = 137, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [66] 'a' */
    {.bitmap_index = 535, .adv_w = 142, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0}, /* [67] 'b' */
    {.bitmap_index = 545, .adv_w = 123, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [68] 'c' */
    {.bitmap_index = 551, .adv_w = 142, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0}, /* [69] 'd' */
    {.bitmap_index = 561, .adv_w = 138, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [70] 'e' */
    {.bitmap_index = 568, .adv_w = 79, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = 0}, /* [71] 'f' */
    {.bitmap_index = 574, .adv_w = 142, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -3}, /* [72] 'g' */
    {.bitmap_index = 584, .adv_w = 142, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0}, /* [73] 'h' */
    {.bitmap_index = 594, .adv_w = 62, .box_w = 1, .box_h = 11, .ofs_x = 1, .ofs_y = 0}, /* [74] 'i' */
    {.bitmap_index = 596, .adv_w = 62, .box_w = 3, .box_h = 14, .ofs_x = -1, .ofs_y = -3}, /* [75] 'j' */
    {.bitmap_index = 602, .adv_w = 130, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0}, /* [76] 'k' */
    {.bitmap_index = 611, .adv_w = 62, .box_w = 1, .box_h = 11, .ofs_x = 1, .ofs_y = 0}, /* [77] 'l' */
    {.bitmap_index = 613, .adv_w = 218, .box_w = 11, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [78] 'm' */
    {.bitmap_index = 624, .adv_w = 142, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [79] 'n' */
    {.bitmap_index = 631, .adv_w = 137, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [80] 'o' */
    {.bitmap_index = 638, .adv_w = 142, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -3}, /* [81] 'p' */
    {.bitmap_index = 648, .adv_w = 142, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -3}, /* [82] 'q' */
    {.bitmap_index = 658, .adv_w = 92, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [83] 'r' */
    {.bitmap_index = 662, .adv_w = 117, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [84] 's' */
    {.bitmap_index = 668, .adv_w = 88, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* [85] 't' */
    {.bitmap_index = 675, .adv_w = 142, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [86] 'u' */
    {.bitmap_index = 682, .adv_w = 132, .box_w = 7, .box_h = 8, .ofs_x = -1, .ofs_y = 0}, /* [87] 'v' */
    {.bitmap_index = 689, .adv_w = 183, .box_w = 9, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [88] 'w' */
    {.bitmap_index = 698, .adv_w = 132, .box_w = 7, .box_h = 8, .ofs_x = -1, .ofs_y = 0}, /* [89] 'x' */
    {.bitmap_index = 705, .adv_w = 132, .box_w = 7, .box_h = 11, .ofs_x = -1, .ofs_y = -3}, /* [90] 'y' */
    {.bitmap_index = 715, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [91] 'z' */
    {.bitmap_index = 721, .adv_w = 142, .box_w = 5, .box_h = 13, .ofs_x = 2, .ofs_y = -2}, /* [92] '{' */
    {.bitmap_index = 730, .adv_w = 76, .box_w = 1, .box_h = 14, .ofs_x = 2, .ofs_y = -3}, /* [93] '|' */
    {.bitmap_index = 732, .adv_w = 142, .box_w = 5, .box_h = 13, .ofs_x = 3, .ofs_y = -2}, /* [94] '}' */
    {.bitmap_index = 741, .adv_w = 188, .box_w = 9, .box_h = 2, .ofs_x = 1, .ofs_y = 4}, /* [95] '~' */
    {.bitmap_index = 744, .adv_w = 153, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* [96] 'U+0410' */
    {.bitmap_index = 756, .adv_w = 154, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [97] 'U+0411' */
    {.bitmap_index = 766, .adv_w = 154, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [98] 'U+0412' */
    {.bitmap_index = 776, .adv_w = 136, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [99] 'U+0413' */
    {.bitmap_index = 784, .adv_w = 192, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = -2}, /* [100] 'U+0414' */
    {.bitmap_index = 799, .adv_w = 142, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [101] 'U+0415' */
    {.bitmap_index = 808, .adv_w = 241, .box_w = 15, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* [102] 'U+0416' */
    {.bitmap_index = 827, .adv_w = 144, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [103] 'U+0417' */
    {.bitmap_index = 836, .adv_w = 168, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [104] 'U+0418' */
    {.bitmap_index = 846, .adv_w = 168, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 0}, /* [105] 'U+0419' */
    {.bitmap_index = 859, .adv_w = 159, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [106] 'U+041A' */
    {.bitmap_index = 869, .adv_w = 168, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [107] 'U+041B' */
    {.bitmap_index = 881, .adv_w = 193, .box_w = 10, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [108] 'U+041C' */
    {.bitmap_index = 894, .adv_w = 168, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [109] 'U+041D' */
    {.bitmap_index = 904, .adv_w = 176, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [110] 'U+041E' */
    {.bitmap_index = 916, .adv_w = 168, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [111] 'U+041F' */
    {.bitmap_index = 926, .adv_w = 135, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [112] 'U+0420' */
    {.bitmap_index = 935, .adv_w = 156, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [113] 'U+0421' */
    {.bitmap_index = 945, .adv_w = 160, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* [114] 'U+0422' */
    {.bitmap_index = 957, .adv_w = 136, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* [115] 'U+0423' */
    {.bitmap_index = 967, .adv_w = 193, .box_w = 11, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [116] 'U+0424' */
    {.bitmap_index = 981, .adv_w = 154, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0}, /* [117] 'U+0425' */
    {.bitmap_index = 993, .adv_w = 174, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = -2}, /* [118] 'U+0426' */
    {.bitmap_index = 1007, .adv_w = 154, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [119] 'U+0427' */
    {.bitmap_index = 1016, .adv_w = 240, .box_w = 13, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [120] 'U+0428' */
    {.bitmap_index = 1033, .adv_w = 245, .box_w = 14, .box_h = 12, .ofs_x = 1, .ofs_y = -2}, /* [121] 'U+0429' */
    {.bitmap_index = 1054, .adv_w = 186, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [122] 'U+042A' */
    {.bitmap_index = 1066, .adv_w = 198, .box_w = 9, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [123] 'U+042B' */
    {.bitmap_index = 1078, .adv_w = 154, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [124] 'U+042C' */
    {.bitmap_index = 1087, .adv_w = 156, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [125] 'U+042D' */
    {.bitmap_index = 1097, .adv_w = 242, .box_w = 12, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [126] 'U+042E' */
    {.bitmap_index = 1112, .adv_w = 156, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0}, /* [127] 'U+042F' */
    {.bitmap_index = 1121, .adv_w = 137, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [128] 'U+0430' */
    {.bitmap_index = 1127, .adv_w = 138, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 1}, /* [129] 'U+0431' */
    {.bitmap_index = 1137, .adv_w = 132, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [130] 'U+0432' */
    {.bitmap_index = 1143, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [131] 'U+0433' */
    {.bitmap_index = 1148, .adv_w = 155, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = -2}, /* [132] 'U+0434' */
    {.bitmap_index = 1158, .adv_w = 138, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [133] 'U+0435' */
    {.bitmap_index = 1165, .adv_w = 202, .box_w = 11, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [134] 'U+0436' */
    {.bitmap_index = 1176, .adv_w = 119, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [135] 'U+0437' */
    {.bitmap_index = 1182, .adv_w = 146, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [136] 'U+0438' */
    {.bitmap_index = 1189, .adv_w = 146, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0}, /* [137] 'U+0439' */
    {.bitmap_index = 1199, .adv_w = 135, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [138] 'U+043A' */
    {.bitmap_index = 1206, .adv_w = 143, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [139] 'U+043B' */
    {.bitmap_index = 1213, .adv_w = 169, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [140] 'U+043C' */
    {.bitmap_index = 1221, .adv_w = 146, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [141] 'U+043D' */
    {.bitmap_index = 1228, .adv_w = 137, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [142] 'U+043E' */
    {.bitmap_index = 1235, .adv_w = 146, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [143] 'U+043F' */
    {.bitmap_index = 1242, .adv_w = 142, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -3}, /* [144] 'U+0440' */
    {.bitmap_index = 1252, .adv_w = 123, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [145] 'U+0441' */
    {.bitmap_index = 1258, .adv_w = 130, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [146] 'U+0442' */
    {.bitmap_index = 1265, .adv_w = 132, .box_w = 7, .box_h = 11, .ofs_x = -1, .ofs_y = -3}, /* [147] 'U+0443' */
    {.bitmap_index = 1275, .adv_w = 192, .box_w = 11, .box_h = 14, .ofs_x = 1, .ofs_y = -3}, /* [148] 'U+0444' */
    {.bitmap_index = 1295, .adv_w = 132, .box_w = 7, .box_h = 8, .ofs_x = -1, .ofs_y = 0}, /* [149] 'U+0445' */
    {.bitmap_index = 1302, .adv_w = 152, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = -2}, /* [150] 'U+0446' */
    {.bitmap_index = 1312, .adv_w = 132, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [151] 'U+0447' */
    {.bitmap_index = 1318, .adv_w = 205, .box_w = 11, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [152] 'U+0448' */
    {.bitmap_index = 1329, .adv_w = 211, .box_w = 12, .box_h = 10, .ofs_x = 1, .ofs_y = -2}, /* [153] 'U+0449' */
    {.bitmap_index = 1344, .adv_w = 158, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0}, /* [154] 'U+044A' */
    {.bitmap_index = 1353, .adv_w = 177, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [155] 'U+044B' */
    {.bitmap_index = 1361, .adv_w = 132, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [156] 'U+044C' */
    {.bitmap_index = 1367, .adv_w = 123, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [157] 'U+044D' */
    {.bitmap_index = 1373, .adv_w = 188, .box_w = 10, .box_h = 8, .ofs_x = 1, .ofs_y = 0}, /* [158] 'U+044E' */
    {.bitmap_index = 1383, .adv_w = 135, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0}, /* [159] 'U+044F' */
    {.bitmap_index = 1390, .adv_w = 142, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0}, /* [160] 'U+0401' */
    {.bitmap_index = 1401, .adv_w = 138, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0}, /* [161] 'U+0451' */
    {.bitmap_index = 1411, .adv_w = 132, .box_w = 4, .box_h = 4, .ofs_x = 2, .ofs_y = 3}, /* [162] 'U+2022' */
};

/* Unicode CMAP tables */
static const lv_font_fmt_txt_cmap_t cmaps_latin_cyrillic_14[] = {
    {
        .range_start = 0x0020,
        .range_length = 95,
        .glyph_id_start = 1,
        .unicode_list = NULL,
        .glyph_id_ofs_list = NULL,
        .list_length = 0,
        .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 0x0410,
        .range_length = 64,
        .glyph_id_start = 96,
        .unicode_list = NULL,
        .glyph_id_ofs_list = NULL,
        .list_length = 0,
        .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 0x0401,
        .range_length = 1,
        .glyph_id_start = 160,
        .unicode_list = NULL,
        .glyph_id_ofs_list = NULL,
        .list_length = 0,
        .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 0x0451,
        .range_length = 1,
        .glyph_id_start = 161,
        .unicode_list = NULL,
        .glyph_id_ofs_list = NULL,
        .list_length = 0,
        .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 0x2022,
        .range_length = 1,
        .glyph_id_start = 162,
        .unicode_list = NULL,
        .glyph_id_ofs_list = NULL,
        .list_length = 0,
        .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};

static const lv_font_fmt_txt_dsc_t font_dsc_latin_cyrillic_14 = {
    .glyph_bitmap = glyph_bitmap_latin_cyrillic_14,
    .glyph_dsc = glyph_dsc_latin_cyrillic_14,
    .cmaps = cmaps_latin_cyrillic_14,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 5,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
};

const lv_font_t veebha_font_latin_cyrillic_14 = {
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,
    .line_height = 14,
    .base_line = 3,
    .subpx = LV_FONT_SUBPX_NONE,
    .underline_position = -1,
    .underline_thickness = 1,
    .dsc = &font_dsc_latin_cyrillic_14,
    .fallback = &veebha_font_devanagari_16
};
