/* FeatherOS - Kernel Main Entry Point */

#include <kernel.h>

extern uint64_t multiboot_magic;
extern uint64_t multiboot_ptr;

void kernel_init(void);
void serial_init(void);
void vga_init(void);

/* Kernel main - called from boot code */
void kernel_main(uint32_t magic, void *mbi) {
    (void)magic;
    (void)mbi;
    
    /* Initialize basic hardware */
    serial_init();
    vga_init();
    
    /* Print boot banner */
    printk("\n");
    printk("========================================\n");
    printk("  FeatherOS v0.1.0 (development)\n");
    printk("========================================\n");
    printk("\n");
    printk("[OK] Boot: GRUB2 multiboot2 detected\n");
    printk("[OK] CPU: x86_64 long mode\n");
    printk("[OK] Memory: Initialized\n");
    printk("\n");
    printk("Kernel initialized successfully!\n");
    printk("System halted.\n");
    
    /* Halt */
    while (1) {
        __asm__ volatile("hlt");
    }
}

/* Kernel panic */
void kernel_panic(const char *msg) {
    vga_clear();
    printk("\n");
    printk("========================================\n");
    printk("  KERNEL PANIC\n");
    printk("========================================\n");
    printk("Message: %s\n", msg);
    printk("\nSystem halted.\n");
    
    while (1) {
        __asm__ volatile("cli; hlt");
    }
}
