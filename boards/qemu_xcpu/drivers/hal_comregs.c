/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Communication Registers (comregs) HAL Implementation for RDA8809
 */

#include "cs_types.h"
#include "global_macros.h"
#include "comregs.h"
#include "hal_comregs.h"
#include "sys_ctrl.h"

extern void os_log_printf(const char *fmt, ...);

static HAL_COMREGS_IRQ_HANDLER_T g_comregsHandler[HAL_COMREGS_XCPU_IRQ_SLOT_QTY] = {NULL, NULL};

// =============================================================================
// hal_ComregsInit - Initialize COMREGS peripheral clock and clear registers
// =============================================================================
void hal_ComregsInit(void)
{
    // 1. Enable System clock for COM_REGS in sysCtrl
    hwp_sysCtrl->Clk_Sys_Enable = SYS_CTRL_ENABLE_SYS_COM_REGS;

    // 2. Clear all interrupts and masks
    hwp_sysComregs->Mask_Clr = 0xFFFF;
    hwp_sysComregs->ItReg_Clr = 0xFFFF;

    os_log_printf("[COMREGS] Driver ready at 0x%08X (Snapshot: 0x%02X)\n", 
                  REG_SYS_COMREGS_BASE, (unsigned int)(hwp_sysComregs->Snapshot & 0xFF));
}

// =============================================================================
// Software Event / IRQ generation & clearing
// =============================================================================
void hal_ComregsSetIrq(HAL_COMREGS_IRQ_SLOT_T slot, UINT8 mask)
{
    if (slot == HAL_COMREGS_XCPU_IRQ_SLOT_0)
    {
        hwp_sysComregs->ItReg_Set = COMREGS_IRQ0_SET(mask);
    }
    else if (slot == HAL_COMREGS_XCPU_IRQ_SLOT_1)
    {
        hwp_sysComregs->ItReg_Set = COMREGS_IRQ1_SET(mask);
    }
}

void hal_ComregsClrIrq(HAL_COMREGS_IRQ_SLOT_T slot, UINT8 mask)
{
    if (slot == HAL_COMREGS_XCPU_IRQ_SLOT_0)
    {
        hwp_sysComregs->ItReg_Clr = COMREGS_IRQ0_CLR(mask);
    }
    else if (slot == HAL_COMREGS_XCPU_IRQ_SLOT_1)
    {
        hwp_sysComregs->ItReg_Clr = COMREGS_IRQ1_CLR(mask);
    }
}

UINT32 hal_ComregsGetCause(void)
{
    return hwp_sysComregs->Cause;
}

// =============================================================================
// Mask control
// =============================================================================
void hal_ComregsSetMask(HAL_COMREGS_IRQ_SLOT_T slot, UINT8 mask)
{
    if (slot == HAL_COMREGS_XCPU_IRQ_SLOT_0)
    {
        hwp_sysComregs->Mask_Set = COMREGS_IRQ0_MASK_SET(mask);
    }
    else if (slot == HAL_COMREGS_XCPU_IRQ_SLOT_1)
    {
        hwp_sysComregs->Mask_Set = COMREGS_IRQ1_MASK_SET(mask);
    }
}

void hal_ComregsClrMask(HAL_COMREGS_IRQ_SLOT_T slot, UINT8 mask)
{
    if (slot == HAL_COMREGS_XCPU_IRQ_SLOT_0)
    {
        hwp_sysComregs->Mask_Clr = COMREGS_IRQ0_MASK_CLR(mask);
    }
    else if (slot == HAL_COMREGS_XCPU_IRQ_SLOT_1)
    {
        hwp_sysComregs->Mask_Clr = COMREGS_IRQ1_MASK_CLR(mask);
    }
}

// =============================================================================
// Snapshot Registers
// =============================================================================
UINT32 hal_ComregsGetSnap(void)
{
    return hwp_sysComregs->Snapshot;
}

void hal_ComregsSetSnapCfg(BOOL wrap3)
{
    hwp_sysComregs->Snapshot_Cfg = wrap3 ? COMREGS_SNAPSHOT_CFG_WRAP_3 : COMREGS_SNAPSHOT_CFG_WRAP_2;
}

// =============================================================================
// IRQ Handling
// =============================================================================
void hal_ComregsIrqSetHandler(HAL_COMREGS_IRQ_SLOT_T slot, HAL_COMREGS_IRQ_HANDLER_T handler)
{
    if (slot < HAL_COMREGS_XCPU_IRQ_SLOT_QTY)
    {
        g_comregsHandler[slot] = handler;
    }
}

void hal_ComregsIrqHandler(void)
{
    UINT32 cause = hwp_sysComregs->Cause;
    UINT8 cause0 = (UINT8)(cause & 0xFF);
    UINT8 cause1 = (UINT8)((cause >> 8) & 0xFF);

    if (cause0)
    {
        hwp_sysComregs->ItReg_Clr = COMREGS_IRQ0_CLR(cause0);
        if (g_comregsHandler[HAL_COMREGS_XCPU_IRQ_SLOT_0])
        {
            g_comregsHandler[HAL_COMREGS_XCPU_IRQ_SLOT_0](HAL_COMREGS_XCPU_IRQ_SLOT_0, cause0);
        }
    }

    if (cause1)
    {
        hwp_sysComregs->ItReg_Clr = COMREGS_IRQ1_CLR(cause1);
        if (g_comregsHandler[HAL_COMREGS_XCPU_IRQ_SLOT_1])
        {
            g_comregsHandler[HAL_COMREGS_XCPU_IRQ_SLOT_1](HAL_COMREGS_XCPU_IRQ_SLOT_1, cause1);
        }
    }
}
