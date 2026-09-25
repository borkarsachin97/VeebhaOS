/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * VoC Configuration Registers for RDA8809
 */

#ifndef _VOC_CFG_H_
#define _VOC_CFG_H_

#include "cs_types.h"
#include "global_macros.h"

#define REG_VOC_CFG_BASE            0x0195FF80

typedef volatile struct
{
    REG16                          Ctrl;                         //0x00000000
    REG16                          DMA_Wrap;                     //0x00000002
    REG16                          DMA_Size;                     //0x00000004
    REG16                          DMA_Laddr;                    //0x00000006
    REG32                          DMA_EAddr;                    //0x00000008
    REG32                          DMA_Data_Single;              //0x0000000C
    REG32                          DMA_2D;                       //0x00000010
    REG32                          Reserved_00000014;            //0x00000014
    REG16                          DAI_Data_In;                  //0x00000018
    REG16                          DAI_Data_Out;                 //0x0000001A
    REG16                          ROM_Page;                     //0x0000001C
    REG16                          Debug;                        //0x0000001E
    REG32                          BIST_Ctrl;                    //0x00000020
    REG16                          Wakeup_Mask;                  //0x00000024
    REG16                          Reserved_00000026;            //0x00000026
    REG16                          Wakeup_Status;                //0x00000028
    REG16                          Wakeup_Cause;                 //0x0000002A
    REG16                          Sema;                         //0x0000002C
    REG16                          Reserved_0000002E[9];         //0x0000002E
    REG32                          REG01;                        //0x00000040
    REG32                          REG23;                        //0x00000044
    REG32                          REG45;                        //0x00000048
    REG32                          REG67;                        //0x0000004C
    REG32                          ACC0;                         //0x00000050
    REG32                          ACC1;                         //0x00000054
    REG32                          RL6;                          //0x00000058
    REG32                          RL7;                          //0x0000005C
    REG16                          PC;                           //0x00000060
    REG16                          RA;                           //0x00000062
    REG16                          SP16;                         //0x00000064
    REG16                          SP32;                         //0x00000066
    REG32                          Reserved_00000068[2];         //0x00000068
    REG16                          BKP;                          //0x00000070
    REG16                          Reserved_00000072;            //0x00000072
    REG16                          PC_PREV;                      //0x00000074
    REG16                          LOOP;                         //0x00000076
} HWP_VOC_CFG_T;

#define hwp_vocCfg                  ((HWP_VOC_CFG_T*) KSEG1(REG_VOC_CFG_BASE))

// Ctrl
#define VOC_CFG_RUN_PAUSE           (0<<0)
#define VOC_CFG_RUN_SOFTWAKEUP0     (1<<0)
#define VOC_CFG_RUN_SOFTWAKEUP1     (3<<0)
#define VOC_CFG_RUN_STOP            (4<<0)
#define VOC_CFG_RUN_START           (5<<0)
#define VOC_CFG_BCPU_IRQ            (1<<4)
#define VOC_CFG_XCPU_IRQ            (1<<5)
#define VOC_CFG_RUNNING             (1<<0)

// DMA_EAddr
#define VOC_CFG_DMA_EADDR_MASK      (0x3FFFFFF<<2)
#define VOC_CFG_DMA_SINGLE          (1<<31)
#define VOC_CFG_DMA_WRITE_WRITE     (1<<30)

// Wakeup_Mask
#define VOC_CFG_WAKEUP_MASK(n)      (((n)&0x3F)<<0)
#define VOC_CFG_WAKEUP_STATUS(n)    (((n)&0x3F)<<0)
#define VOC_CFG_WAKEUP_STATUS_MASK  (0x3F<<0)

#define VOC_CFG_SP16(n)             (((n)&0xFFFF)<<0)
#define VOC_CFG_SP32(n)             (((n)&0xFFFF)<<0)

#endif // _VOC_CFG_H_
