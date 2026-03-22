/* FeatherOS - Linker Script */
/* Multiboot header must be at file offset 0 (within first 8KB) */

ENTRY(_start)

PHDRS {
    boot PT_LOAD AT(0);
    text PT_LOAD;
}

SECTIONS
{
    /* Multiboot header at file offset 0, virtual address 0x100000 */
    . = 0x100000;
    .multiboot : {
        *(.multiboot)
        *(.multiboot.*)
    } :boot
    
    /* ELF notes */
    .note : {
        *(.note)
    } :boot
    
    .text : {
        *(.text)
        *(.text.*)
    } :text
    
    .rodata : {
        *(.rodata)
        *(.rodata.*)
    } :text
    
    .data : {
        *(.data)
        *(.data.*)
    } :text
    
    .bss : {
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        _bss_end = .;
    } :text
    
    /DISCARD/ : {
        *(.comment)
        *(.note.gnu.build-id)
    }
}
