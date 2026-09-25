/*
 * Linker script for VeebhaOS on RDA8809 MIPS XCPU (QEMU Emulation)
 * Origin: 0x82000280 (8MB PSRAM)
 * Entry Point: bl_entry in ._start
 *
 * SPDX-License-Identifier: MIT
 */

ENTRY(bl_entry)

MEMORY
{
    ram (wx) : ORIGIN = 0x82000280, LENGTH = 0x800000 /* 8 MB */
}

SECTIONS
{
    .text 0x82000280 : {
        KEEP(*(._start))
        . = 0x80;
        KEEP(*(.fixed_irq_dispatcher))
        *(.text*)
    } > ram

    .rodata : { 
        . = ALIGN(4);
        *(.rodata*) 
    } > ram

    .data : { 
        . = ALIGN(4);
        *(.data*)   
        *(.sdata*)
    } > ram

    _gp = ALIGN(16) + 0x7ff0;

    .bss (NOLOAD) : {
        . = ALIGN(4);
        _bss_start = .;
        *(.sbss*)
        *(.scommon*)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > ram

    /DISCARD/ : {
        *(.rel.dyn)
        *(.rel.*)
        *(.note*)
        *(.comment)
        *(.pdr)
    }
}

