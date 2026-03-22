/*
 * FeatherOS Linker Script for x86_64
 * Simple layout - just gets the kernel linked
 */

ENTRY(_start)

SECTIONS
{
    . = 0x100000;

    .text : {
        *(.bootstrap)
        *(.text)
        *(.text.*)
    }

    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }

    .data : {
        *(.data)
        *(.data.*)
    }

    .bss : {
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        *(.common)
        _bss_end = .;
    }
}
