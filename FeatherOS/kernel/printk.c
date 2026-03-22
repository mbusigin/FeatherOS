/* FeatherOS - Printf Implementation */

#include <kernel.h>

/* Forward declarations */
extern void vga_putchar(char c);

/* Minimal printf */
static void print_str(const char *s) {
    while (*s) {
        vga_putchar(*s++);
    }
}

static void print_hex(uint32_t val) {
    const char hex[] = "0123456789ABCDEF";
    char buf[11];
    buf[0] = '0';
    buf[1] = 'x';
    buf[10] = '\0';
    
    if (val == 0) {
        print_str("0x0");
        return;
    }
    
    int i = 9;
    while (val > 0 && i > 1) {
        buf[i--] = hex[val & 0xF];
        val >>= 4;
    }
    print_str(buf);
}

int printk(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            vga_putchar(*p);
            continue;
        }
        
        p++;
        switch (*p) {
            case 'c': {
                char c = (char)va_arg(args, int);
                vga_putchar(c);
                break;
            }
            case 's': {
                const char *s = va_arg(args, const char *);
                print_str(s ? s : "(null)");
                break;
            }
            case 'd': {
                int val = va_arg(args, int);
                if (val < 0) {
                    vga_putchar('-');
                    val = -val;
                }
                char buf[12];
                int i = 10;
                buf[11] = '\0';
                if (val == 0) buf[10] = '0';
                while (val > 0 && i >= 0) {
                    buf[i--] = '0' + (val % 10);
                    val /= 10;
                }
                print_str(&buf[i+1]);
                break;
            }
            case 'u': {
                unsigned int val = va_arg(args, unsigned int);
                char buf[11];
                buf[10] = '\0';
                if (val == 0) buf[9] = '0';
                int i = 9;
                while (val > 0 && i >= 0) {
                    buf[i--] = '0' + (val % 10);
                    val /= 10;
                }
                print_str(&buf[i+1]);
                break;
            }
            case 'x':
            case 'p': {
                uint32_t val = va_arg(args, uint32_t);
                print_hex(val);
                break;
            }
            case '%':
                vga_putchar('%');
                break;
            default:
                vga_putchar('%');
                vga_putchar(*p);
                break;
        }
    }
    
    va_end(args);
    return 0;
}
