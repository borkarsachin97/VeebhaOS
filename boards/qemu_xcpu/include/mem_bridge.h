/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Memory Bridge & External Bus Controller (EBC) Definitions for RDA8809
 */

#ifndef _MEM_BRIDGE_H_
#define _MEM_BRIDGE_H_

#include "cs_types.h"
#include "global_macros.h"

#define EBC_NB_CS                   5
#define NB_ROM_PACHT                16

#define REG_MEM_BRIDGE_BASE         0x01A04000
#define REG_BB_MEM_BRIDGE_BASE      0x01912000

typedef volatile struct
{
    REG32                          FIFO_Ctrl;                    //0x00000000
    REG32                          FIFO_Status;                  //0x00000004
    REG32                          Monitor_Ctrl;                 //0x00000008
    REG32                          Rom_Bist;                     //0x0000000C
    REG32                          SRam_Bist;                    //0x00000010
    REG32                          Reserved_00000014[59];        //0x00000014
    REG32                          Rom_Patch[NB_ROM_PACHT];      //0x00000100
    REG32                          Reserved_00000140[48];        //0x00000140
    REG32                          Reserved_00000200[64];        //0x00000200
    REG32                          Reserved_00000300[64];        //0x00000300
    REG32                          EBC_Ctrl;                     //0x00000400
    REG32                          EBC_Status;                   //0x00000404
    REG32                          Reserved_00000408;            //0x00000408
    REG32                          CS_Time_Write;                //0x0000040C
    struct
    {
        REG32                      CS_Mode;                      //0x00000410 + i*8
        REG32                      CS_Time;                      //0x00000414 + i*8
    } CS_Config[EBC_NB_CS];
    REG32                          Reserved_00000438[50];        //0x00000438
    REG32                          AHBM_Ctrl;                    //0x00000500
    REG32                          AHBM_Status;                  //0x00000504
    REG32                          Reserved_00000508[2];         //0x00000508
    REG32                          SpaceBase[EBC_NB_CS];         //0x00000510
} HWP_MEM_BRIDGE_T;

#define hwp_memBridge               ((HWP_MEM_BRIDGE_T*) KSEG1(REG_MEM_BRIDGE_BASE))
#define hwp_bbMemBridge             ((HWP_MEM_BRIDGE_T*) KSEG1(REG_BB_MEM_BRIDGE_BASE))

#define MEM_BRIDGE_ENABLE           (1<<0)

#endif // _MEM_BRIDGE_H_
