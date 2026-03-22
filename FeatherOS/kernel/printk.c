/* FeatherOS - Printf/Printk Implementation
 * Sprint 3: Console & Basic I/O
 */

#include <kernel.h>

/* Global log level */
log_level_t current_log_level = LOG_INFO;

/* Forward declarations */
extern void console_putchar(char c);
extern void serial_putchar(char c);

/* Output character to both VGA and serial */
static void output_char(char c) {
    console_putchar(c);
    serial_putchar(c);
}

/* Output string to both VGA and serial */
static void output_string(const char *s) {
    while (*s) {
        output_char(*s++);
    }
}

/* Convert integer to string */
static size_t int_to_str(char *buf, int64_t value, int base, int width, bool leading_zeros, bool is_negative) {
    static const char digits[] = "0123456789abcdef";
    char tmp[32];
    int i = 0;
    size_t len = 0;
    
    if (value == 0) {
        tmp[i++] = '0';
    } else {
        uint64_t v = (uint64_t)value;
        if (is_negative) {
            v = (uint64_t)(-(int64_t)value);
        }
        while (v > 0) {
            tmp[i++] = digits[v % base];
            v /= base;
        }
    }
    
    len = i;
    
    /* Add negative sign */
    if (is_negative) {
        *buf++ = '-';
        width--;
    }
    
    /* Add padding */
    int padding = width - (int)len;
    if (padding > 0 && !leading_zeros) {
        while (padding-- > 0) {
            *buf++ = ' ';
        }
    }
    
    /* Add leading zeros if specified */
    if (leading_zeros && padding > 0) {
        while (padding-- > 0) {
            *buf++ = '0';
        }
    }
    
    /* Write digits in reverse order */
    while (i > 0) {
        *buf++ = tmp[--i];
    }
    
    return len + (is_negative ? 1 : 0) + ((size_t)width > len ? (size_t)width - len : 0);
}

/* Convert unsigned integer to string */
static size_t uint_to_str(char *buf, uint64_t value, int base, int width, bool leading_zeros) {
    static const char digits[] = "0123456789abcdef";
    char tmp[32];
    int i = 0;
    size_t len = 0;
    
    if (value == 0) {
        tmp[i++] = '0';
    } else {
        while (value > 0) {
            tmp[i++] = digits[value % base];
            value /= base;
        }
    }
    
    len = i;
    
    /* Add padding */
    int padding = width - (int)len;
    if (padding > 0 && !leading_zeros) {
        while (padding-- > 0) {
            *buf++ = ' ';
        }
    }
    
    /* Add leading zeros if specified */
    if (leading_zeros && padding > 0) {
        while (padding-- > 0) {
            *buf++ = '0';
        }
    }
    
    /* Write digits in reverse order */
    while (i > 0) {
        *buf++ = tmp[--i];
    }
    
    return len + ((size_t)width > len ? (size_t)width - len : 0);
}

/* Convert printf format string and output */
int console_printf(const char *fmt, va_list args) {
    char buf[64];
    const char *p = fmt;
    int count = 0;
    
    while (*p) {
        if (*p == '%' && *(p + 1)) {
            p++;
            
            /* Parse flags */
            bool leading_zeros = false;
            int width = 0;
            int precision = -1;
            
            while (*p) {
                if (*p == '0') {
                    if (width == 0) {
                        leading_zeros = true;
                    } else {
                        width = width * 10;
                    }
                    p++;
                } else if (*p >= '1' && *p <= '9') {
                    width = width * 10 + (*p - '0');
                    p++;
                } else if (*p == '.') {
                    precision = 0;
                    p++;
                    while (*p >= '0' && *p <= '9') {
                        precision = precision * 10 + (*p - '0');
                        p++;
                    }
                } else {
                    break;
                }
            }
            
            UNUSED(precision);
            
            /* Parse format specifier */
            switch (*p) {
                case 'c': {
                    char c = (char)va_arg(args, int);
                    output_char(c);
                    count++;
                    break;
                }
                
                case 's': {
                    const char *s = va_arg(args, const char *);
                    if (!s) s = "(null)";
                    output_string(s);
                    count += strlen(s);
                    break;
                }
                
                case 'd':
                case 'i': {
                    int value = va_arg(args, int);
                    size_t len = int_to_str(buf, value, 10, width, leading_zeros, value < 0);
                    output_string(buf);
                    count += len;
                    break;
                }
                
                case 'u': {
                    unsigned int value = va_arg(args, unsigned int);
                    size_t len = uint_to_str(buf, value, 10, width, leading_zeros);
                    output_string(buf);
                    count += len;
                    break;
                }
                
                case 'x': {
                    unsigned int value = va_arg(args, unsigned int);
                    size_t len = uint_to_str(buf, value, 16, width, leading_zeros);
                    output_string(buf);
                    count += len;
                    break;
                }
                
                case 'X': {
                    unsigned int value = va_arg(args, unsigned int);
                    size_t len = uint_to_str(buf, value, 16, width, leading_zeros);
                    /* Uppercase hex */
                    char *p = buf;
                    while (*p) {
                        if (*p >= 'a' && *p <= 'f') {
                            *p = *p - 'a' + 'A';
                        }
                        p++;
                    }
                    output_string(buf);
                    count += len;
                    break;
                }
                
                case 'p': {
                    uint64_t ptr = (uint64_t)(uintptr_t)va_arg(args, void *);
                    output_string("0x");
                    size_t len = uint_to_str(buf, ptr, 16, 16, true);
                    output_string(buf);
                    count += 2 + len;
                    break;
                }
                
                case 'l': {
                    p++;
                    if (*p == 'd' || *p == 'i') {
                        long value = va_arg(args, long);
                        size_t len = int_to_str(buf, value, 10, width, leading_zeros, value < 0);
                        output_string(buf);
                        count += len;
                    } else if (*p == 'u') {
                        unsigned long value = va_arg(args, unsigned long);
                        size_t len = uint_to_str(buf, value, 10, width, leading_zeros);
                        output_string(buf);
                        count += len;
                    } else if (*p == 'x') {
                        unsigned long value = va_arg(args, unsigned long);
                        size_t len = uint_to_str(buf, value, 16, width, leading_zeros);
                        output_string(buf);
                        count += len;
                    } else {
                        p--;
                        output_char('%');
                        count++;
                    }
                    break;
                }
                
                case 'z': {
                    p++;
                    if (*p == 'd' || *p == 'i') {
                        int64_t value = va_arg(args, int64_t);
                        size_t len = int_to_str(buf, value, 10, width, leading_zeros, value < 0);
                        output_string(buf);
                        count += len;
                    } else if (*p == 'u') {
                        uint64_t value = va_arg(args, uint64_t);
                        size_t len = uint_to_str(buf, value, 10, width, leading_zeros);
                        output_string(buf);
                        count += len;
                    } else {
                        p--;
                        output_char('%');
                        count++;
                    }
                    break;
                }
                
                case '%': {
                    output_char('%');
                    count++;
                    break;
                }
                
                default: {
                    output_char('%');
                    output_char(*p);
                    count += 2;
                    break;
                }
            }
        } else {
            output_char(*p);
            count++;
        }
        p++;
    }
    
    return count;
}

/* Print formatted message to console */
void console_print(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    console_printf(fmt, args);
    va_end(args);
}

/* Kernel panic - halt with message */
void kernel_panic(const char *msg) {
    cli();
    
    output_string("\n\n*** KERNEL PANIC ***\n");
    output_string(msg);
    output_string("\n\nSystem halted.\n");
    
    while (1) {
        hlt();
    }
}

/* Kernel halt */
void kernel_halt(void) {
    cli();
    while (1) {
        hlt();
    }
}
