/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * Consolidated Exception Vector Trampoline & FreeRTOS Context Switching
 * for RDA8809 / RDA88XCL (MIPS XCPU)
 */

.set noat
.set noreorder

.global isram_vector_stub_start
.global isram_vector_stub_end

.global vPortYield
.global vPortStartFirstTask

.global cp0_get_status
.global cp0_set_status
.global cp0_get_cause
.global cp0_clear_bev
.global cp0_set_bev

.extern pxCurrentTCB
.extern vTaskSwitchContext
.extern os_master_c_dispatcher
.extern uxCriticalNesting
.extern g_freertos_scheduler_started

.text

// =============================================================================
//  ISRAM EXCEPTION & HARDWARE IRQ VECTOR STUB (Target Origin: 0x81C00280)
// =============================================================================
isram_vector_stub_start:

    // 1. Allocate full 128-byte context frame on current task stack
    addiu   $sp, $sp, -128
    sw      $at, 0x00($sp)
    sw      $v0, 0x04($sp)
    sw      $v1, 0x08($sp)
    sw      $a0, 0x0c($sp)
    sw      $a1, 0x10($sp)
    sw      $a2, 0x14($sp)
    sw      $a3, 0x18($sp)
    sw      $t0, 0x1c($sp)
    sw      $t1, 0x20($sp)
    sw      $t2, 0x24($sp)
    sw      $t3, 0x28($sp)
    sw      $t4, 0x2c($sp)
    sw      $t5, 0x30($sp)
    sw      $t6, 0x34($sp)
    sw      $t7, 0x38($sp)
    sw      $s0, 0x3c($sp)
    sw      $s1, 0x40($sp)
    sw      $s2, 0x44($sp)
    sw      $s3, 0x48($sp)
    sw      $s4, 0x4c($sp)
    sw      $s5, 0x50($sp)
    sw      $s6, 0x54($sp)
    sw      $s7, 0x58($sp)
    sw      $t8, 0x5c($sp)
    sw      $t9, 0x60($sp)
    sw      $gp, 0x64($sp)
    sw      $fp, 0x68($sp)
    sw      $ra, 0x6c($sp)

    mflo    $t0
    mfhi    $t1
    sw      $t0, 0x70($sp)
    sw      $t1, 0x74($sp)
    ori     $t0, $zero, 0xff01  // Task Status is ALWAYS 0xFF01 (IM=0xFF, IE=1, EXL=0)
    mfc0    $t1, $14            // CP0 EPC
    sw      $t0, 0x78($sp)
    sw      $t1, 0x7c($sp)

    // Check if FreeRTOS scheduler is actively running
    lui     $t2, %hi(g_freertos_scheduler_started)
    lw      $t2, %lo(g_freertos_scheduler_started)($t2)
    nop
    nop

    // Check nesting: load uxInterruptNesting
    lui     $t0, %hi(uxInterruptNesting)
    lw      $t1, %lo(uxInterruptNesting)($t0)
    nop
    nop
    bne     $t1, $zero, 1f
    nop

    // If non-nested IRQ (nesting == 0), save task SP to pxCurrentTCB & switch to IRQ stack
    lui     $t2, %hi(pxCurrentTCB)
    lw      $t2, %lo(pxCurrentTCB)($t2)
    nop
    nop
    beq     $t2, $zero, 1f
    nop
    sw      $sp, 0($t2)         // pxCurrentTCB->pxTopOfStack = $sp

    // Switch $sp directly to top of dedicated ISRAM IRQ stack (0x81C0FFF8, 8-byte aligned)
    lui     $sp, 0x81c0
    ori     $sp, $sp, 0xfff8

1:
    // Increment IRQ nesting level in memory
    lui     $t0, %hi(uxInterruptNesting)
    lw      $t1, %lo(uxInterruptNesting)($t0)
    nop
    nop
    addiu   $t1, $t1, 1
    sw      $t1, %lo(uxInterruptNesting)($t0)

    // Read CP0 Cause and EPC
    mfc0    $a0, $13            // a0 = Cause
    mfc0    $a1, $14            // a1 = EPC

    // Clear / latch hardware sysIrq SC
    lui     $t0, 0xa1a0
    lw      $zero, 0x1014($t0)
    nop
    nop

    // Keep CPU interrupts disabled during master C dispatcher (IM=0xFF, IE=0)
    ori     $t0, $zero, 0xff00
    mtc0    $t0, $12
    nop
    nop
    nop

    // Call master IRQ dispatcher in C (located in PSRAM: 0x82000300)
    lui     $t9, %hi(os_master_c_dispatcher)
    addiu   $t9, $t9, %lo(os_master_c_dispatcher)
    jalr    $t9
    nop
    nop
    nop

    // Re-lock interrupts
    ori     $t0, $zero, 0xff04
    mtc0    $t0, $12
    nop
    nop
    nop

    // Decrement IRQ nesting level in memory
    lui     $t0, %hi(uxInterruptNesting)
    lw      $t1, %lo(uxInterruptNesting)($t0)
    nop
    nop
    addiu   $t1, $t1, -1
    sw      $t1, %lo(uxInterruptNesting)($t0)

    // If returning to task level (uxInterruptNesting == 0) and scheduler is running:
    lui     $t2, %hi(g_freertos_scheduler_started)
    lw      $t2, %lo(g_freertos_scheduler_started)($t2)
    nop
    nop
    beq     $t2, $zero, 2f
    nop
    bne     $t1, $zero, 2f      // if t1 != 0, still nested
    nop

    lui     $t2, %hi(pxCurrentTCB)
    lw      $t2, %lo(pxCurrentTCB)($t2)
    nop
    nop
    beq     $t2, $zero, 2f
    nop

    // Load active task SP from pxCurrentTCB (may have changed during context switch)
    lw      $sp, 0($t2)
    nop
    nop

2:
    // Restore hardware System IRQ Critical Section (SC = 1)
    lui     $t0, 0xa1a0
    li      $t1, 1
    sw      $t1, 0x1014($t0)
    nop
    nop

    // Restore task context frame with proper load delay slots
    lw      $t0, 0x70($sp)
    lw      $t1, 0x74($sp)
    nop
    mtlo    $t0
    mthi    $t1

    // Restore EPC into $k0
    lw      $k0, 0x7c($sp)
    nop

    // Prepare CP0 Status for rfe (IEp=1, EXL=1: 0xff04 shifts to 0xff01 with IE=1)
    ori     $t0, $zero, 0xff04
    mtc0    $t0, $12
    nop
    nop
    nop

    lw      $at, 0x00($sp)
    lw      $v0, 0x04($sp)
    lw      $v1, 0x08($sp)
    lw      $a0, 0x0c($sp)
    lw      $a1, 0x10($sp)
    lw      $a2, 0x14($sp)
    lw      $a3, 0x18($sp)
    lw      $t0, 0x1c($sp)
    lw      $t1, 0x20($sp)
    lw      $t2, 0x24($sp)
    lw      $t3, 0x28($sp)
    lw      $t4, 0x2c($sp)
    lw      $t5, 0x30($sp)
    lw      $t6, 0x34($sp)
    lw      $t7, 0x38($sp)
    lw      $s0, 0x3c($sp)
    lw      $s1, 0x40($sp)
    lw      $s2, 0x44($sp)
    lw      $s3, 0x48($sp)
    lw      $s4, 0x4c($sp)
    lw      $s5, 0x50($sp)
    lw      $s6, 0x54($sp)
    lw      $s7, 0x58($sp)
    lw      $t8, 0x5c($sp)
    lw      $t9, 0x60($sp)
    lw      $gp, 0x64($sp)
    lw      $fp, 0x68($sp)
    lw      $ra, 0x6c($sp)
    nop

    addiu   $sp, $sp, 128

    jr      $k0
    rfe

isram_vector_stub_end:


// =============================================================================
//  FreeRTOS Voluntary Yield: vPortYield()
// =============================================================================
.ent vPortYield
vPortYield:
    // Disable interrupts during yield context switch
    ori     $t0, $zero, 0xff00
    mtc0    $t0, $12
    nop
    nop
    nop

    // 1. Allocate context frame on task stack
    addiu   $sp, $sp, -128
    sw      $at, 0x00($sp)
    sw      $v0, 0x04($sp)
    sw      $v1, 0x08($sp)
    sw      $a0, 0x0c($sp)
    sw      $a1, 0x10($sp)
    sw      $a2, 0x14($sp)
    sw      $a3, 0x18($sp)
    sw      $t0, 0x1c($sp)
    sw      $t1, 0x20($sp)
    sw      $t2, 0x24($sp)
    sw      $t3, 0x28($sp)
    sw      $t4, 0x2c($sp)
    sw      $t5, 0x30($sp)
    sw      $t6, 0x34($sp)
    sw      $t7, 0x38($sp)
    sw      $s0, 0x3c($sp)
    sw      $s1, 0x40($sp)
    sw      $s2, 0x44($sp)
    sw      $s3, 0x48($sp)
    sw      $s4, 0x4c($sp)
    sw      $s5, 0x50($sp)
    sw      $s6, 0x54($sp)
    sw      $s7, 0x58($sp)
    sw      $t8, 0x5c($sp)
    sw      $t9, 0x60($sp)
    sw      $gp, 0x64($sp)
    sw      $fp, 0x68($sp)
    sw      $ra, 0x6c($sp)

    mflo    $t1
    mfhi    $t2
    sw      $t1, 0x70($sp)
    sw      $t2, 0x74($sp)
    ori     $t0, $zero, 0xff01  // Save pure task CP0 Status (with IE=1, EXL=0)
    sw      $t0, 0x78($sp)
    sw      $ra, 0x7c($sp)      // EPC = return address ($ra)

    // Save SP into pxCurrentTCB
    lui     $t1, %hi(pxCurrentTCB)
    lw      $t1, %lo(pxCurrentTCB)($t1)
    nop
    nop
    sw      $sp, 0($t1)

    // Switch task context
    lui     $t9, %hi(vTaskSwitchContext)
    addiu   $t9, $t9, %lo(vTaskSwitchContext)
    jalr    $t9
    nop
    nop
    nop
 
    // Ensure hardware SC = 1
    lui     $t0, 0xa1a0
    li      $t1, 1
    sw      $t1, 0x1014($t0)
    nop
    nop

    // Load new task SP from pxCurrentTCB
    lui     $t1, %hi(pxCurrentTCB)
    lw      $t1, %lo(pxCurrentTCB)($t1)
    nop
    nop
    lw      $sp, 0($t1)
    nop
    nop

    // Restore new context with proper load delay slots
    lw      $t0, 0x70($sp)
    lw      $t1, 0x74($sp)
    nop
    mtlo    $t0
    mthi    $t1

    lw      $k0, 0x7c($sp)      // Target PC into $k0
    nop

    // Prepare CP0 Status for rfe (0xff04 so rfe shifts to 0xff01 with IE=1)
    ori     $t0, $zero, 0xff04
    mtc0    $t0, $12
    nop
    nop
    nop

    lw      $at, 0x00($sp)
    lw      $v0, 0x04($sp)
    lw      $v1, 0x08($sp)
    lw      $a0, 0x0c($sp)
    lw      $a1, 0x10($sp)
    lw      $a2, 0x14($sp)
    lw      $a3, 0x18($sp)
    lw      $t0, 0x1c($sp)
    lw      $t1, 0x20($sp)
    lw      $t2, 0x24($sp)
    lw      $t3, 0x28($sp)
    lw      $t4, 0x2c($sp)
    lw      $t5, 0x30($sp)
    lw      $t6, 0x34($sp)
    lw      $t7, 0x38($sp)
    lw      $s0, 0x3c($sp)
    lw      $s1, 0x40($sp)
    lw      $s2, 0x44($sp)
    lw      $s3, 0x48($sp)
    lw      $s4, 0x4c($sp)
    lw      $s5, 0x50($sp)
    lw      $s6, 0x54($sp)
    lw      $s7, 0x58($sp)
    lw      $t8, 0x5c($sp)
    lw      $t9, 0x60($sp)      // Restore $t9 from frame
    lw      $gp, 0x64($sp)
    lw      $fp, 0x68($sp)
    lw      $ra, 0x6c($sp)
    nop

    addiu   $sp, $sp, 128
    
    jr      $k0
    rfe
.end vPortYield


// =============================================================================
//  FreeRTOS Scheduler Launch: vPortStartFirstTask()
// =============================================================================
.ent vPortStartFirstTask
vPortStartFirstTask:
    // Mark FreeRTOS scheduler as started
    lui     $t0, %hi(g_freertos_scheduler_started)
    li      $t1, 1
    sw      $t1, %lo(g_freertos_scheduler_started)($t0)

    // Reset critical nesting
    lui     $t0, %hi(uxCriticalNesting)
    sw      $zero, %lo(uxCriticalNesting)($t0)

    // Reset IRQ nesting count in memory
    lui     $t0, %hi(uxInterruptNesting)
    sw      $zero, %lo(uxInterruptNesting)($t0)

    // Enable system IRQ critical section in hardware controller (SC = 1)
    lui     $t0, 0xa1a0
    li      $t1, 1
    sw      $t1, 0x1014($t0)
    nop
    nop

    // Load SP from pxCurrentTCB
    lui     $t1, %hi(pxCurrentTCB)
    lw      $t1, %lo(pxCurrentTCB)($t1)
    nop
    nop
    lw      $sp, 0($t1)
    nop
    nop

    // Load task argument into $a0 (pvParameters from offset 0x0c)
    lw      $a0, 0x0c($sp)

    // Load task GP and FP
    lw      $gp, 0x64($sp)
    lw      $fp, 0x68($sp)

    // Load task return address into $ra (prvTaskExitError from offset 0x6c)
    lw      $ra, 0x6c($sp)

    // Load initial Task PC into $k0 (from offset 0x7c) and $t9 (from offset 0x60)
    lw      $k0, 0x7c($sp)
    lw      $t9, 0x60($sp)
    nop
    nop

    // Pop the 128-byte stack frame
    addiu   $sp, $sp, 128

    // Enable CPU global interrupts via rfe (Status = 0xff04 -> rfe shifts to 0xff01)
    ori     $t0, $zero, 0xff04
    mtc0    $t0, $12
    nop
    nop
    nop
    jr      $k0
    rfe
.end vPortStartFirstTask


// =============================================================================
//  CP0 REGISTER HELPERS
// =============================================================================

cp0_get_status:
    mfc0    $v0, $12
    jr      $ra
    nop

cp0_set_status:
    mtc0    $a0, $12
    nop
    nop
    nop
    jr      $ra
    nop

cp0_get_cause:
    mfc0    $v0, $13
    jr      $ra
    nop

cp0_clear_bev:
    mfc0    $t0, $12            // Read CP0 Status ($12)
    lui     $t1, 0xFFBF         // Mask to clear BEV bit (bit 22 = 0x00400000)
    ori     $t1, $t1, 0xFFFF
    and     $t0, $t0, $t1
    mtc0    $t0, $12            // Write back to CP0 Status
    nop
    nop
    nop
    jr      $ra
    nop

cp0_set_bev:
    mfc0    $t0, $12            // Read CP0 Status ($12)
    lui     $t1, 0x0040         // BEV bit (bit 22 = 0x00400000)
    or      $t0, $t0, $t1
    mtc0    $t0, $12            // Write back to CP0 Status
    nop
    nop
    nop
    jr      $ra
    nop

.global cp0_get_prid
cp0_get_prid:
    mfc0    $v0, $15            // CP0 PRId ($15)
    jr      $ra
    nop

.global mips_cache_flush
mips_cache_flush:
    lui     $t0, 0xa1a0
    lw      $zero, 0x1014($t0)  // Dummy read uncached bus to drain write buffers
    nop
    nop
    nop
    nop
    jr      $ra
    nop

