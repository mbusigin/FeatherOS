/* FeatherOS - Kernel Header Files */

#ifndef FEATHEROS_KERNEL_H
#define FEATHEROS_KERNEL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

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

/* I/O Port Access */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t val;
    __asm__ volatile("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t val;
    __asm__ volatile("inl %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* Memory-mapped I/O */
static inline uint8_t mmio_read8(volatile void *addr) {
    return *(volatile uint8_t*)addr;
}

static inline void mmio_write8(volatile void *addr, uint8_t val) {
    *(volatile uint8_t*)addr = val;
}

static inline uint32_t mmio_read32(volatile void *addr) {
    return *(volatile uint32_t*)addr;
}

static inline void mmio_write32(volatile void *addr, uint32_t val) {
    *(volatile uint32_t*)addr = val;
}

/* Interrupt handling */
static inline void cli(void) {
    __asm__ volatile("cli" : : : "memory");
}

static inline void sti(void) {
    __asm__ volatile("sti" : : : "memory");
}

static inline void hlt(void) {
    __asm__ volatile("hlt" : : : "memory");
}

static inline unsigned long cpu_flags(void) {
    unsigned long flags;
    __asm__ volatile("pushf; pop %0" : "=r"(flags));
    return flags;
}

static inline void lgdt(void *ptr) {
    __asm__ volatile("lgdt %0" : : "m"(*(uint16_t*)ptr));
}

/* Logging levels */
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3,
    LOG_PANIC = 4
} log_level_t;

extern log_level_t current_log_level;

#define LOG_LEVEL_NAMES {"DEBUG", "INFO", "WARN", "ERROR", "PANIC"}

/* Variadic macros with at least one argument */
#define printk(fmt, ...) console_print(fmt, ##__VA_ARGS__)
#define debug_print(fmt, ...) console_print("[DEBUG] " fmt, ##__VA_ARGS__)
#define info_print(fmt, ...) console_print("[INFO] " fmt, ##__VA_ARGS__)
#define warn_print(fmt, ...) console_print("[WARN] " fmt, ##__VA_ARGS__)
#define error_print(fmt, ...) console_print("[ERROR] " fmt, ##__VA_ARGS__)

/* Kernel functions */
void kernel_main(uint32_t magic, void *mbi) __noreturn;
void kernel_panic(const char *msg) __noreturn;
void kernel_halt(void) __noreturn;

/* Console subsystem */
void console_init(void);
void console_clear(void);
void console_putchar(char c);
void console_puts(const char *s);
void console_print(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int console_printf(const char *fmt, va_list args);
int console_readline(char *buf, size_t size);
char console_getchar(void);
bool console_has_input(void);

/* VGA text mode driver */
void vga_init(void);
void vga_clear(void);
void vga_set_color(uint8_t fg, uint8_t bg);
void vga_get_cursor(uint16_t *x, uint16_t *y);
void vga_set_cursor(uint16_t x, uint16_t y);
void vga_enable_cursor(void);
void vga_disable_cursor(void);
void vga_putchar(char c);

/* Serial driver (16550 UART) */
void serial_init(void);
void serial_init_port(uint16_t port);
int serial_received(uint16_t port);
int serial_can_transmit(uint16_t port);
char serial_read_char(uint16_t port);
void serial_write_char(uint16_t port, char c);
void serial_putchar(char c);
void serial_puts(const char *s);
char serial_getchar(void);
bool serial_has_input(void);

/* Keyboard driver */
void keyboard_init(void);
char keyboard_read_scancode(void);
char keyboard_getchar(void);
bool keyboard_has_key(void);
const char *keyboard_scancode_to_string(uint8_t scancode);

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
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);
char *strchr(const char *s, int c);
char *strstr(const char *haystack, const char *needle);

#endif /* FEATHEROS_KERNEL_H */
