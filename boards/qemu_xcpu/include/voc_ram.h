/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * VoC RAM Memory Map for RDA8809
 */

#ifndef _VOC_RAM_H_
#define _VOC_RAM_H_

#include "cs_types.h"
#include "global_macros.h"

#define VOC_RAM_X_SIZE                           (0x5000) // 20 KB
#define VOC_RAM_Y_SIZE                           (0x5000) // 20 KB
#define VOC_RAM_I_SIZE                           (0x5000) // 20 KB

#define REG_VOC_RAM_BASE            0x01940000

typedef volatile struct
{
    UINT8 voc_ram_x_base[20480];                //0x00000000
    UINT8 voc_ram_x_hole[12288];                //0x00005000
    UINT8 voc_ram_y_base[20480];                //0x00008000
    UINT8 voc_ram_y_hole[12288];                //0x0000D000
    UINT8 voc_rom_z_hole[65536];                //0x00010000
    UINT8 voc_ram_i_base[20480];                //0x00020000
    UINT8 voc_ram_i_hole[110592];               //0x00025000
} HWP_VOC_RAM_T;

#define hwp_vocRam                  ((HWP_VOC_RAM_T*) KSEG1(REG_VOC_RAM_BASE))

#endif // _VOC_RAM_H_
