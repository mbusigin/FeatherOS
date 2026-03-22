/* FeatherOS - Printf Implementation */

#include <stdint.h>
#include <stdarg.h>

extern void vga_putchar(char c);

static void putchar(char c) {
    vga_putchar(c);
}

static void puts(const char *s) {
    while (*s) putchar(*s++);
}

static void puthex(uint64_t value, int width) {
    const char hex[] = "0123456789ABCDEF";
    char buf[20];
    int i = 0;
    
    if (value == 0) {
        while (width-- > 1) putchar('0');
        putchar('0');
        return;
    }
    
    while (value > 0) {
        buf[i++] = hex[value & 0xF];
        value >>= 4;
    }
    
    while (i < width) buf[i++] = '0';
    
    while (i > 0) putchar(buf[--i]);
}

int printk(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    int written = 0;
    
    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            putchar(*p);
            written++;
            continue;
        }
        
        p++;
        switch (*p) {
            case 'c': {
                char c = va_arg(args, int);
                putchar(c);
                written++;
                break;
            }
            case 's': {
                const char *s = va_arg(args, const char *);
                puts(s);
                written++;
                break;
            }
            case 'd':
            case 'i': {
                int value = va_arg(args, int);
                if (value < 0) {
                    putchar('-');
                    written++;
                    value = -value;
                }
                puthex(value, 0);
                written++;
                break;
            }
            case 'u': {
                unsigned int value = va_arg(args, unsigned int);
                puthex(value, 0);
                written++;
                break;
            }
            case 'x':
            case 'X': {
                unsigned int value = va_arg(args, unsigned int);
                puthex(value, 8);
                written++;
                break;
            }
            case 'p': {
                void *ptr = va_arg(args, void *);
                putchar('0'); putchar('x');
                puthex((uint64_t)ptr, 16);
                written += 18;
                break;
            }
            case '%':
                putchar('%');
                written++;
                break;
            default:
                putchar('%');
                putchar(*p);
                written += 2;
                break;
        }
    }
    
    va_end(args);
    return written;
}
