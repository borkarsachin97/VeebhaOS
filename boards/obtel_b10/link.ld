/*
 * VeebhaOS - Embedded Operating System for Feature Phones
 *
 * OBTEL B10 (RDA8809) Linker Script
 *
 * SPDX-License-Identifier: MIT
 */

OUTPUT_FORMAT("elf32-tradlittlemips")
OUTPUT_ARCH(mips)
ENTRY(_start)

MEMORY
{
    ISRAM (rwx) : ORIGIN = 0x81C00000, LENGTH = 64K
    PSRAM (rwx) : ORIGIN = 0x82000000, LENGTH = 8M
    FLASH (rx)  : ORIGIN = 0x88000000, LENGTH = 4M
}

SECTIONS
{
    .isram_stub :
    {
        KEEP(*(.isram_stub))
    } > ISRAM

    .text :
    {
        _text_start = .;
        *(.text)
        *(.text.*)
        *(.stub)
        *(.gnu.warning)
        *(.gnu.linkonce.t*)
        _text_end = .;
    } > PSRAM

    .rodata :
    {
        _rodata_start = .;
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r*)
        _rodata_end = .;
    } > PSRAM

    .data :
    {
        _data_start = .;
        _gp = ALIGN(16);
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d*)
        *(.sdata)
        *(.sdata.*)
        _data_end = .;
    } > PSRAM

    .bss (NOLOAD) :
    {
        _bss_start = .;
        *(.sbss)
        *(.sbss.*)
        *(.scommon)
        *(.bss)
        *(.bss.*)
        *(COMMON)
        _bss_end = .;
    } > PSRAM

    .heap (NOLOAD) :
    {
        _heap_start = .;
        . = . + 4M;
        _heap_end = .;
    } > PSRAM

    /DISCARD/ :
    {
        *(.MIPS.abiflags)
        *(.comment)
        *(.pdr)
        *(.reginfo)
    }
}
