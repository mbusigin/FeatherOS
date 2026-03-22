/* FeatherOS - Kernel Header Files */

#ifndef FEATHEROS_KERNEL_H
#define FEATHEROS_KERNEL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

/* Standard macros */
#ifndef NULL
#define NULL ((void*)0)
#endif
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

#define UNUSED(x) (void)(x)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Compiler attributes */
#define __noreturn __attribute__((noreturn))
#define __packed __attribute__((packed))
#define __aligned(x) __attribute__((aligned(x)))

/* I/O functions */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void cli(void) {
    __asm__ volatile("cli" : : : "memory");
}

static inline void sti(void) {
    __asm__ volatile("sti" : : : "memory");
}

static inline void hlt(void) {
    __asm__ volatile("hlt" : : : "memory");
}

static inline void lgdt(void *ptr) {
    __asm__ volatile("lgdt %0" : : "m"(*(uint16_t*)ptr));
}

/* Kernel functions */
void kernel_main(uint32_t magic, void *mbi) __noreturn;
void kernel_panic(const char *msg) __noreturn;

/* Hardware drivers */
void serial_init(void);
void serial_putchar(char c);
void vga_init(void);
void vga_clear(void);
void vga_putchar(char c);

/* Output */
int printk(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* String functions */
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

#endif /* FEATHEROS_KERNEL_H */
