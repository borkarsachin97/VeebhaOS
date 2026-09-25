/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Configuration Registers (cfg_regs) for RDA8809
 */

#ifndef _CFG_REGS_H_
#define _CFG_REGS_H_

#include "cs_types.h"
#include "global_macros.h"

// =============================================================================
//  Configuration Registers Base & Structure
// =============================================================================
#define REG_CONFIG_REGS_BASE        0x01A24000

typedef volatile struct
{
    REG32                          CHIP_ID;                      //0x00000000
    REG32                          Build_Version;                //0x00000004
    REG32                          GPIO_Mode;                    //0x00000008
    REG32                          Alt_mux_select;               //0x0000000C
    REG32                          IO_Drive1_Select;             //0x00000010
    REG32                          IO_Drive2_Select;             //0x00000014
    REG32                          audio_pd_set;                 //0x00000018
    REG32                          audio_pd_clr;                 //0x0000001C
    REG32                          audio_sel_cfg;                //0x00000020
    REG32                          audio_mic_cfg;                //0x00000024
    REG32                          audio_spk_cfg;                //0x00000028
    REG32                          audio_rcv_gain;               //0x0000002C
    REG32                          audio_head_gain;              //0x00000030
} HWP_CFG_REGS_T;

#define hwp_configRegs              ((HWP_CFG_REGS_T*) KSEG1(REG_CONFIG_REGS_BASE))

// =============================================================================
//  Pin Multiplexing & Drive Configuration
// =============================================================================
#define CFG_REGS_SPI1_SELECT_I2C_3  (1<<5)
#define CFG_REGS_FM_POWER_ON        (1<<6)
#define CFG_REGS_UART2_MASK         (1<<7)
#define CFG_REGS_UART2_UART2        (0<<7)
#define CFG_REGS_UART2_UART1        (1<<7)
#define CFG_REGS_PWL1_MASK          (3<<8)
#define CFG_REGS_PWL1_PWL1          (0<<8)
#define CFG_REGS_PWL1_GPO_6         (1<<8)
#define CFG_REGS_PWL1_CLK_32K       (2<<8)
#define CFG_REGS_SDMMC_MASK         (1<<10)
#define CFG_REGS_SDMMC_SDMMC        (0<<10)
#define CFG_REGS_SDMMC_DIGRF        (1<<10)
#define CFG_REGS_I2C1_MASK          (1<<12)
#define CFG_REGS_I2C1_UART1         (0<<12)
#define CFG_REGS_I2C1_I2C1          (1<<12)
#define CFG_REGS_SDMMC_CTRL_DRIVE_MASK       (3<<17)
#define CFG_REGS_SDMMC_CTRL_DRIVE_FAST_FAST  (0<<17)
#define CFG_REGS_I2C2_MASK          (1<<20)
#define CFG_REGS_I2C2_I2C2          (0<<20)
#define CFG_REGS_I2C2_USB           (1<<20)

// =============================================================================
//  Audio Power Down Bitfields (audio_pd_set / audio_pd_clr)
// =============================================================================
#define CFG_REGS_AU_DEEP_PD_N       (1<<0)
#define CFG_REGS_AU_REF_PD_N        (1<<1)
#define CFG_REGS_AU_MIC_PD_N        (1<<2)
#define CFG_REGS_AU_AUXMIC_PD_N     (1<<3)
#define CFG_REGS_AU_AD_PD_N         (1<<4)
#define CFG_REGS_AU_DAC_PD_N        (1<<5)
#define CFG_REGS_AU_DAC_RESET_N     (1<<8)

// =============================================================================
//  Audio Select Configuration (audio_sel_cfg)
// =============================================================================
#define CFG_REGS_AU_AUXMIC_SEL      (1<<0)
#define CFG_REGS_AU_SPK_SEL         (1<<1)
#define CFG_REGS_AU_SPK_MONO_SEL    (1<<2)
#define CFG_REGS_AU_RCV_SEL         (1<<3)
#define CFG_REGS_AU_HEAD_SEL        (1<<4)
#define CFG_REGS_AU_FM_SEL          (1<<5)

// =============================================================================
//  Audio Gain & Mute Configuration
// =============================================================================
#define CFG_REGS_AU_MIC_GAIN(n)     (((n)&15)<<0)
#define CFG_REGS_AU_MIC_MUTE_N      (1<<4)

#define CFG_REGS_AU_SPK_GAIN(n)     (((n)&15)<<0)
#define CFG_REGS_AU_SPK_MUTE_N      (1<<4)

#define CFG_REGS_AU_RCV_GAIN(n)     (((n)&15)<<0)
#define CFG_REGS_AU_HEAD_GAIN(n)    (((n)&15)<<0)

#endif // _CFG_REGS_H_
