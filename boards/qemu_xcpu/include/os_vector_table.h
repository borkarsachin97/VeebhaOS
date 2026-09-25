/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * OS Vector Table & ISRAM Jump Table Header for RDA8809 / RDA88XCL
 *
 * MIPS Interrupt Vector Architecture (RDA8809):
 *   - BEV bit in CP0 Status stays SET (=1) permanently on this chip.
 *   - With BEV=1, MIPS vectors ALL interrupts to:  0xBFC00180 (KSEG1 ROM)
 *   - The ROM's boot_IrqHandler is physically located at ISRAM offset 0x180:
 *       ISRAM Physical: 0x01C00000 + 0x180 = 0x01C00180
 *       ISRAM KSEG1:    0xA1C00180  (uncached access)
 *       ISRAM KSEG0:    0x81C00180  (cached access, what the ROM maps to)
 *   - We OVERWRITE the ROM's handler stub at 0x81C00180 with our own
 *     context-saving trampoline, which then calls our C dispatcher.
 *
 * Jump Table Layout (at 0x81C00380, safely after the vector stub):
 *   Slot 0: os_master_c_dispatcher  (called by our trampoline)
 *   Slot 1: timer_irq_handler
 *   Slot 2: keypad_irq_handler
 *   Slot 3: usb_irq_handler
 */

#ifndef _OS_VECTOR_TABLE_H_
#define _OS_VECTOR_TABLE_H_

#include "cs_types.h"

// ============================================================================
// MIPS EXCEPTION VECTOR ADDRESS (BEV=1 always on RDA8809)
// The ROM maps ISRAM to the BEV=1 exception vector space.
// Our stub overwrites the ROM handler at ISRAM offset +0x180.
// ============================================================================
#define ISRAM_VECTOR_ENTRY      0x81C00180   // = ISRAM_BASE + 0x180

// ============================================================================
// ISRAM JUMP TABLE - placed well clear of the ~990-byte vector stub
// ============================================================================
#define ISRAM_JUMP_TABLE_ADDR   0x81C00700   // Base + 0x700 (safely clear of stub)
#define MAX_IRQ_HANDLERS        8

// Handler prototype: void handler(uint32_t cause, uint32_t epc)
typedef void (*os_irq_handler_t)(uint32_t cause, uint32_t epc);

// Full MIPS CPU register context structure (saved during Tier 2 exceptions)
typedef struct {
    uint32_t zero;      // 0x00
    uint32_t at;        // 0x04
    uint32_t v0;        // 0x08
    uint32_t v1;        // 0x0C
    uint32_t a0;        // 0x10
    uint32_t a1;        // 0x14
    uint32_t a2;        // 0x18
    uint32_t a3;        // 0x1C
    uint32_t t0;        // 0x20
    uint32_t t1;        // 0x24
    uint32_t t2;        // 0x28
    uint32_t t3;        // 0x2C
    uint32_t t4;        // 0x30
    uint32_t t5;        // 0x34
    uint32_t t6;        // 0x38
    uint32_t t7;        // 0x3C
    uint32_t s0;        // 0x40
    uint32_t s1;        // 0x44
    uint32_t s2;        // 0x48
    uint32_t s3;        // 0x4C
    uint32_t s4;        // 0x50
    uint32_t s5;        // 0x54
    uint32_t s6;        // 0x58
    uint32_t s7;        // 0x5C
    uint32_t t8;        // 0x60
    uint32_t t9;        // 0x64
    uint32_t k0;        // 0x68
    uint32_t k1;        // 0x6C
    uint32_t gp;        // 0x70
    uint32_t sp;        // 0x74
    uint32_t fp;        // 0x78 (s8)
    uint32_t ra;        // 0x7C
    uint32_t status;    // 0x80
    uint32_t lo;        // 0x84
    uint32_t hi;        // 0x88
    uint32_t badvaddr;  // 0x8C
    uint32_t cause;     // 0x90
    uint32_t epc;       // 0x94
} os_exception_frame_t;

// IRQ slot index enumeration
typedef enum {
    IRQ_INDEX_MASTER_DISPATCHER = 0,
    IRQ_INDEX_TIMER             = 1,
    IRQ_INDEX_KEYPAD            = 2,
    IRQ_INDEX_USB               = 3,
    IRQ_INDEX_BBIFC             = 4,
    IRQ_INDEX_GOUDA             = 5
} irq_index_t;

// ISRAM Jump Table Structure
typedef struct {
    os_irq_handler_t handlers[MAX_IRQ_HANDLERS];
} os_jump_table_t;

// Global jump table pointer (mapped at ISRAM_JUMP_TABLE_ADDR)
extern os_jump_table_t * const g_isram_jump_table;

// Public API
void os_vector_system_init(void);
void os_register_irq_handler(irq_index_t irq, os_irq_handler_t handler);
void os_master_c_dispatcher(uint32_t cause, uint32_t epc);
void os_exception_c_handler(uint32_t exc_code, uint32_t cause, uint32_t epc, os_exception_frame_t *frame);
void reboot(void);

// Diagnostic helpers for test UI
uint32_t os_get_irq_count(void);
uint32_t os_get_last_cause(void);
uint32_t os_get_last_epc(void);
BOOL os_is_bev_cleared(void);

// Assembly trampoline labels (placed in .text, copied to ISRAM at runtime)
extern char isram_vector_stub_start[];
extern char isram_vector_stub_end[];

#endif // _OS_VECTOR_TABLE_H_
