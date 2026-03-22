/* FeatherOS - Kernel Main Entry Point */

#include "kernel.h"

#define MULTIBOOT2_MAGIC 0xE85250D6

/* External functions */
extern void serial_init(void);
extern void vga_init(void);

/* Kernel entry - called from boot.S */
void kernel_main(uint32_t magic, void *mbi) {
    (void)magic;
    (void)mbi;
    
    /* Initialize kernel subsystems */
    kernel_init();
    
    /* Main kernel loop */
    while (1) {
        __asm__ volatile("hlt");
    }
}

/* Kernel initialization */
void kernel_init(void) {
    /* Initialize subsystems */
    serial_init();
    vga_init();
    
    /* Print boot banner */
    printk("\n");
    printk("========================================\n");
    printk("  FeatherOS v0.1.0 (development)\n");
    printk("========================================\n");
    printk("\n");
    printk("[OK] Kernel initialized\n");
    printk("\n");
    printk("System halted - Boot successful!\n");
}

/* Kernel panic */
void kernel_panic(const char *message) {
    printk("\nKERNEL PANIC: %s\n", message);
    while (1) {
        __asm__ volatile("cli; hlt");
    }
}
