#!/usr/bin/env python3
"""
VeebhaOS 1-bit Font Generator
Generates 1-bpp monochrome font descriptors for LVGL 9:
- fonts/veebha_font_latin_cyrillic_14.c
- fonts/veebha_font_devanagari_16.c
"""

import os
from PIL import Image, ImageDraw, ImageFont

def render_glyph(font, char, font_size, baseline_val, is_combining=False, x_offset_adj=0):
    canvas_w = 48
    canvas_h = 48
    base_x = 16
    base_y = 24
    
    img = Image.new('1', (canvas_w, canvas_h), 0)
    draw = ImageDraw.Draw(img)
    
    # Draw character anchored at baseline (left-baseline 'ls')
    draw.text((base_x, base_y), char, font=font, fill=1, anchor='ls')
    
    bbox = img.getbbox()
    
    try:
        adv_w_px = font.getlength(char)
    except Exception:
        adv_w_px = font_size * 0.6
        
    if is_combining:
        adv_w_px = 0
    
    if bbox is None or char == ' ':
        # Empty glyph (space or unrenderable)
        adv_w_val = int(round(adv_w_px * 16))
        if adv_w_val <= 0 and not is_combining:
            adv_w_val = int(round(font_size * 0.35 * 16))
        return {
            'char': char,
            'adv_w': adv_w_val,
            'box_w': 0,
            'box_h': 0,
            'ofs_x': 0,
            'ofs_y': 0,
            'bytes': []
        }
    
    min_x, min_y, max_x, max_y = bbox
    box_w = max_x - min_x
    box_h = max_y - min_y
    ofs_x = (min_x - base_x) + x_offset_adj
    ofs_y = base_y - max_y
    
    adv_w_val = int(round(adv_w_px * 16))
    if not is_combining and adv_w_val < (box_w + ofs_x) * 16:
        adv_w_val = (box_w + ofs_x + 1) * 16
        
    # Extract bitstream
    bits = []
    for y in range(min_y, max_y):
        for x in range(min_x, max_x):
            pixel = img.getpixel((x, y))
            bits.append(1 if pixel else 0)
    
    # Pack bits into bytes (MSB first)
    byte_list = []
    for i in range(0, len(bits), 8):
        chunk = bits[i:i+8]
        b = 0
        for idx, bit in enumerate(chunk):
            if bit:
                b |= (1 << (7 - idx))
        byte_list.append(b)
    
    return {
        'char': char,
        'adv_w': adv_w_val,
        'box_w': box_w,
        'box_h': box_h,
        'ofs_x': ofs_x,
        'ofs_y': ofs_y,
        'bytes': byte_list
    }

def generate_latin_cyrillic():
    font_path = '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
    font_size = 14
    baseline = 3
    font = ImageFont.truetype(font_path, font_size)
    
    # Range 1: ASCII 0x20..0x7E (95 characters)
    ascii_chars = [chr(c) for c in range(0x20, 0x7F)]
    # Range 2: Cyrillic 0x0410..0x044F (64 characters)
    cyrillic_chars = [chr(c) for c in range(0x0410, 0x0450)]
    # Range 3: Cyrillic 0x0401 (Ё)
    yo_upper = [chr(0x0401)]
    # Range 4: Cyrillic 0x0451 (ё)
    yo_lower = [chr(0x0451)]
    # Range 5: Bullet 0x2022 (•)
    bullet = [chr(0x2022)]
    
    all_chars = ascii_chars + cyrillic_chars + yo_upper + yo_lower + bullet
    
    glyph_records = []
    # Reserved index 0
    glyph_records.append({
        'char': '\0',
        'adv_w': 0, 'box_w': 0, 'box_h': 0, 'ofs_x': 0, 'ofs_y': 0,
        'bytes': [], 'bitmap_index': 0
    })
    
    total_bytes = 0
    all_bitmap_bytes = []
    
    for ch in all_chars:
        g = render_glyph(font, ch, font_size, baseline)
        g['bitmap_index'] = total_bytes
        total_bytes += len(g['bytes'])
        all_bitmap_bytes.extend(g['bytes'])
        glyph_records.append(g)
    
    # Generate C file
    c_code = '''/*
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
'''
    
    # Write bitmap bytes
    for i in range(0, len(all_bitmap_bytes), 16):
        chunk = all_bitmap_bytes[i:i+16]
        hex_str = ', '.join(f'0x{b:02X}' for b in chunk)
        c_code += f'    {hex_str},\n'
    if c_code.endswith(',\n'):
        c_code = c_code[:-2] + '\n'
    
    c_code += '''};

/* Glyph descriptions table */
static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc_latin_cyrillic_14[] __attribute__((section(".rodata.fonts"))) = {
'''
    for idx, g in enumerate(glyph_records):
        ch_repr = g['char'] if (32 <= ord(g['char']) <= 126 and g['char'] != '\\') else f"U+{ord(g['char']):04X}"
        c_code += f"    {{.bitmap_index = {g['bitmap_index']}, .adv_w = {g['adv_w']}, .box_w = {g['box_w']}, .box_h = {g['box_h']}, .ofs_x = {g['ofs_x']}, .ofs_y = {g['ofs_y']}}}, /* [{idx}] '{ch_repr}' */\n"
    
    c_code += '''};

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
'''
    with open('/home/vixxkigoli/pm/VeebhaOS/fonts/veebha_font_latin_cyrillic_14.c', 'w') as f:
        f.write(c_code)
    print(f"Generated veebha_font_latin_cyrillic_14.c ({len(all_bitmap_bytes)} bytes bitmap data)")

def generate_devanagari():
    dev_fonts = [
        '/usr/share/fonts/truetype/Gargi/Gargi.ttf',
        '/usr/share/fonts/truetype/annapurna/AnnapurnaSIL-Regular.ttf',
        '/usr/share/fonts/truetype/Sarai/Sarai.ttf',
        '/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf'
    ]
    dev_path = None
    for p in dev_fonts:
        if os.path.exists(p):
            dev_path = p
            break
            
    font_size = 14
    baseline = 3
    font = ImageFont.truetype(dev_path, font_size)
    
    # Complete Devanagari range from 0x0901 to 0x096F
    combining_marks = {
        0x0901: -5, # Chandrabindu
        0x0902: -5, # Anusvara
        0x093C: -4, # Nukta
        0x0941: -5, # Short U
        0x0942: -5, # Long UU
        0x0943: -5, # Ri
        0x0944: -5, # Rii
        0x0947: -6, # E
        0x0948: -6, # AI
        0x094D: -4, # Halant / Virama
    }
    
    all_codepoints = list(range(0x0901, 0x0970))
    
    glyph_records = []
    # Reserved index 0
    glyph_records.append({
        'cp': 0, 'char': '\0',
        'adv_w': 0, 'box_w': 0, 'box_h': 0, 'ofs_x': 0, 'ofs_y': 0,
        'bytes': [], 'bitmap_index': 0
    })
    
    total_bytes = 0
    all_bitmap_bytes = []
    valid_codepoints = []
    
    for cp in all_codepoints:
        ch = chr(cp)
        is_comb = cp in combining_marks
        x_adj = combining_marks.get(cp, 0)
        g = render_glyph(font, ch, font_size, baseline, is_combining=is_comb, x_offset_adj=x_adj)
        g['cp'] = cp
        g['bitmap_index'] = total_bytes
        total_bytes += len(g['bytes'])
        all_bitmap_bytes.extend(g['bytes'])
        glyph_records.append(g)
        valid_codepoints.append(cp)
    
    unicode_list = [cp - 0x0900 for cp in valid_codepoints]
    
    c_code = '''/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * Copyright (C) 2026 VeebhaOS Project Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "fonts/veebha_font_devanagari_16.h"

/* 1-bpp monochrome Devanagari 14px bitmap array */
static const uint8_t glyph_bitmap_devanagari_16[] __attribute__((section(".rodata.fonts"))) = {
'''
    for i in range(0, len(all_bitmap_bytes), 16):
        chunk = all_bitmap_bytes[i:i+16]
        hex_str = ', '.join(f'0x{b:02X}' for b in chunk)
        c_code += f'    {hex_str},\n'
    if c_code.endswith(',\n'):
        c_code = c_code[:-2] + '\n'
        
    c_code += '''};

/* Glyph descriptions table */
static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc_devanagari_16[] __attribute__((section(".rodata.fonts"))) = {
'''
    for idx, g in enumerate(glyph_records):
        c_code += f"    {{.bitmap_index = {g['bitmap_index']}, .adv_w = {g['adv_w']}, .box_w = {g['box_w']}, .box_h = {g['box_h']}, .ofs_x = {g['ofs_x']}, .ofs_y = {g['ofs_y']}}}, /* [{idx}] U+{g.get('cp', 0):04X} */\n"
    
    c_code += '''};

/* Unicode sparse relative codepoints table (base 0x0900) */
static const uint16_t unicode_list_devanagari_16[] = {
'''
    for i in range(0, len(unicode_list), 8):
        chunk = unicode_list[i:i+8]
        hex_str = ', '.join(f'0x{u:04X}' for u in chunk)
        c_code += f'    {hex_str},\n'
    if c_code.endswith(',\n'):
        c_code = c_code[:-2] + '\n'
        
    c_code += f'''}};

/* Unicode CMAP table (SPARSE_TINY) */
static const lv_font_fmt_txt_cmap_t cmaps_devanagari_16[] = {{
    {{
        .range_start = 0x0900,
        .range_length = 0x0100,
        .glyph_id_start = 1,
        .unicode_list = unicode_list_devanagari_16,
        .glyph_id_ofs_list = NULL,
        .list_length = {len(unicode_list)},
        .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }}
}};

static const lv_font_fmt_txt_dsc_t font_dsc_devanagari_16 = {{
    .glyph_bitmap = glyph_bitmap_devanagari_16,
    .glyph_dsc = glyph_dsc_devanagari_16,
    .cmaps = cmaps_devanagari_16,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
}};

LV_FONT_DECLARE(lv_font_montserrat_14);

const lv_font_t veebha_font_devanagari_16 = {{
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,
    .line_height = 14,
    .base_line = 3,
    .subpx = LV_FONT_SUBPX_NONE,
    .underline_position = -1,
    .underline_thickness = 1,
    .dsc = &font_dsc_devanagari_16,
    .fallback = &lv_font_montserrat_14
}};
'''
    with open('/home/vixxkigoli/pm/VeebhaOS/fonts/veebha_font_devanagari_16.c', 'w') as f:
        f.write(c_code)
    print(f"Generated veebha_font_devanagari_16.c ({len(all_bitmap_bytes)} bytes bitmap data, {len(valid_codepoints)} glyphs)")

if __name__ == '__main__':
    generate_latin_cyrillic()
    generate_devanagari()
