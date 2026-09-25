/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * OBTEL B10 (RDA8809) ISRAM Boot Stub & Exception Handlers
 *
 * SPDX-License-Identifier: MIT
 */

    .section .isram_stub, "ax", @progbits
    .set noreorder
    .set noat
    .global _start
    .global vPortRestoreContextStub
    .extern main
    .extern vPortSystemTickHandler
    .extern vTaskSwitchContext

_start:
    /* Reset CP0 Status: Disable interrupts and clear BEV */
    mfc0    $k0, $12
    li      $k1, 0x00400000
    or      $k0, $k0, $k1
    mtc0    $k0, $12
    ehb

    /* Setup Stack Pointer to top of internal SRAM (0x81C0FFF0) */
    la      $sp, 0x81C0FFF0
    la      $gp, _gp

    /* Clear BSS */
    la      $t0, _bss_start
    la      $t1, _bss_end
1:
    bgeu    $t0, $t1, 2f
    nop
    sw      $zero, 0($t0)
    addiu   $t0, $t0, 4
    b       1b
    nop
2:
    /* Jump to C entry point */
    jal     main
    nop

3:
    b       3b
    nop

vPortRestoreContextStub:
    /* Restore context from task stack */
    lw      $k0, 0($sp)         /* Critical nesting */
    la      $k1, uxCriticalNesting
    sw      $k0, 0($k1)
    addiu   $sp, $sp, 4

    /* Restore GP registers */
    lw      $1,  0($sp)
    lw      $2,  4($sp)
    lw      $3,  8($sp)
    lw      $4,  12($sp)
    lw      $5,  16($sp)
    lw      $6,  20($sp)
    lw      $7,  24($sp)
    lw      $8,  28($sp)
    lw      $9,  32($sp)
    lw      $10, 36($sp)
    lw      $11, 40($sp)
    lw      $12, 44($sp)
    lw      $13, 48($sp)
    lw      $14, 52($sp)
    lw      $15, 56($sp)
    lw      $16, 60($sp)
    lw      $17, 64($sp)
    lw      $18, 68($sp)
    lw      $19, 72($sp)
    lw      $20, 76($sp)
    lw      $21, 80($sp)
    lw      $22, 84($sp)
    lw      $23, 88($sp)
    lw      $24, 92($sp)
    lw      $25, 96($sp)
    lw      $28, 100($sp)       /* $gp */
    lw      $30, 104($sp)       /* $fp */
    lw      $31, 108($sp)       /* $ra */

    lw      $k0, 112($sp)       /* LO */
    mtlo    $k0
    lw      $k0, 116($sp)       /* HI */
    mthi    $k0

    lw      $k0, 120($sp)       /* EPC */
    mtc0    $k0, $14

    lw      $k0, 124($sp)       /* Status */
    mtc0    $k0, $12
    ehb

    addiu   $sp, $sp, 128
    eret
    nop
