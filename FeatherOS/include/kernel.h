/* FeatherOS - Kernel Header Files */
#ifndef FEATHEROS_KERNEL_H
#define FEATHEROS_KERNEL_H

/* Standard includes */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Boolean values */
#ifndef true
#define true  1
#endif
#ifndef false
#define false 0
#endif

/* Compiler attributes */
#define __noreturn __attribute__((noreturn))
#define __aligned(x) __attribute__((aligned(x)))
#define __packed __attribute__((packed))

/* Kernel macros */
#define UNUSED(x) (void)(x)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Memory constants */
#define PAGE_SIZE      4096
#define PAGE_ALIGN(n) (((n) + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1))

/* Error codes */
#define SUCCESS        0
#define E_UNSPECIFIED -1
#define E_NOMEM       -2
#define E_INVAL       -3
#define E_NOSUCH      -4

/* I/O inline functions */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void cli(void) {
    __asm__ volatile("cli");
}

static inline void sti(void) {
    __asm__ volatile("sti");
}

static inline void hlt(void) {
    __asm__ volatile("hlt");
}

/* Kernel entry */
void kernel_main(uint32_t magic, void *mbi) __noreturn;
void kernel_init(void);
void kernel_panic(const char *msg) __noreturn;

/* Serial port */
void serial_init(void);
void serial_putchar(char c);
char serial_getchar(void);
int serial_write(const char *buf, size_t len);

/* VGA text mode */
void vga_init(void);
void vga_clear(void);
void vga_putchar(char c);
void vga_write(const char *str);

/* Printf */
int printk(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif /* FEATHEROS_KERNEL_H */
