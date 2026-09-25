/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 */

#ifndef _USBC_H_
#define _USBC_H_

#include "cs_types.h"

#define REG_USBC_BASE               0x01A80000

typedef volatile struct
{
    REG32                          GOTGCTL;                      //0x00000000
    REG32                          GOTGINT;                      //0x00000004
    REG32                          GAHBCFG;                      //0x00000008
    REG32                          GUSBCFG;                      //0x0000000C
    REG32                          GRSTCTL;                      //0x00000010
    REG32                          GINTSTS;                      //0x00000014
    REG32                          GINTMSK;                      //0x00000018
    REG32                          GRXSTSR;                      //0x0000001C
    REG32                          GRXSTSP;                      //0x00000020
    REG32                          GRXFSIZ;                      //0x00000024
    REG32                          GNPTXFSIZ;                    //0x00000028
    REG32                          GNPTXSTS;                     //0x0000002C
    REG32                          GI2CCTL;                      //0x00000030
    REG32                          GPVNDCTL;                     //0x00000034
    REG32                          GGPIO;                        //0x00000038
    REG32                          GUID;                         //0x0000003C
    REG32                          GSNPSID;                      //0x00000040
    REG32                          GHWCFG1;                      //0x00000044
    REG32                          GHWCFG2;                      //0x00000048
    REG32                          GHWCFG3;                      //0x0000004C
    REG32                          GHWCFG4;                      //0x00000050
    REG32 Reserved_00000054[43];                //0x00000054
    REG32                          HPTXFSIZ;                     //0x00000100
    struct
    {
        REG32                      DIEnPTXF;                     //0x00000104
    } DIEPTXF[3];
    REG32 Reserved_00000110[444];               //0x00000110
    REG32                          DCFG;                         //0x00000800
    REG32                          DCTL;                         //0x00000804
    REG32                          DSTS;                         //0x00000808
    REG32 Reserved_0000080C;                    //0x0000080C
    REG32                          DIEPMSK;                      //0x00000810
    REG32                          DOEPMSK;                      //0x00000814
    REG32                          DAINT;                        //0x00000818
    REG32                          DAINTMSK;                     //0x0000081C
    REG32                          DTKNQR1;                      //0x00000820
    REG32                          DTKNQR2;                      //0x00000824
    REG32                          DVBUSDIS;                     //0x00000828
    REG32                          DVBUSPULSE;                   //0x0000082C
    REG32                          DTHRCTL;                      //0x00000830
    REG32                          DIEPEMPMSK;                   //0x00000834
    REG32 Reserved_00000838[50];                //0x00000838
    REG32                          DIEPCTL0;                     //0x00000900
    REG32 Reserved_00000904;                    //0x00000904
    REG32                          DIEPINT0;                     //0x00000908
    REG32 Reserved_0000090C;                    //0x0000090C
    REG32                          DIEPTSIZ0;                    //0x00000910
    REG32                          DIEPDMA0;                     //0x00000914
    REG32                          DIEPFSTS0;                    //0x00000918
    REG32 Reserved_0000091C;                    //0x0000091C
    struct
    {
        REG32                      DIEPCTL;                      //0x00000920
        REG32 Reserved_00000004;                //0x00000004
        REG32                      DIEPINT;                      //0x00000928
        REG32 Reserved_0000000C;                //0x0000000C
        REG32                      DIEPTSIZ;                     //0x00000930
        REG32                      DIEPDMA;                      //0x00000934
        REG32                      DIEPFSTS;                     //0x00000938
        REG32 Reserved_0000001C;                //0x0000001C
    } DIEPnCONFIG[3];
    REG32 Reserved_00000980[96];                //0x00000980
    REG32                          DOEPCTL0;                     //0x00000B00
    REG32 Reserved_00000B04;                    //0x00000B04
    REG32                          DOEPINT0;                     //0x00000B08
    REG32 Reserved_00000B0C;                    //0x00000B0C
    REG32                          DOEPTSIZ0;                    //0x00000B10
    REG32                          DOEPDMA0;                     //0x00000B14
    REG32 Reserved_00000B18[2];                 //0x00000B18
    struct
    {
        REG32                      DOEPCTL;                      //0x00000B20
        REG32 Reserved_00000004;                //0x00000004
        REG32                      DOEPINT;                      //0x00000B28
        REG32 Reserved_0000000C;                //0x0000000C
        REG32                      DOEPTSIZ;                     //0x00000B30
        REG32                      DOEPDMA;                      //0x00000B34
        REG32 Reserved_00000018[2];             //0x00000018
    } DOEPnCONFIG[2];
    REG32 Reserved_00000B60[168];               //0x00000B60
    REG32                          PCGCCTL;                      //0x00000E00
    REG32 Reserved_00000E04[127];               //0x00000E04
    struct
    {
        REG32                      TxRxData;                     //0x00001000
        REG32 Reserved_00000004[1023];          //0x00000004
    } EPnFIFO[4];
} HWP_USBC_T;

#define hwp_usbc                    ((HWP_USBC_T*) KSEG1(REG_USBC_BASE))

// GINTSTS bits
#define USBC_GINTSTS_OTGINT         (1<<2)
#define USBC_GINTSTS_RXFLVL         (1<<4)
#define USBC_GINTSTS_USBSUSP        (1<<11)
#define USBC_GINTSTS_USBRST         (1<<12)
#define USBC_GINTSTS_ENUMDONE       (1<<13)
#define USBC_GINTSTS_IEPINT         (1<<18)
#define USBC_GINTSTS_OEPINT         (1<<19)
#define USBC_GINTSTS_DISCONNINT     (1<<29)
#define USBC_GINTSTS_WKUPINT        (1<<31)

#endif // _USBC_H_
