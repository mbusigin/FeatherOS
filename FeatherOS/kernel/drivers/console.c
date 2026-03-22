/* FeatherOS - Console Driver
 * Sprint 3: Console & Basic I/O
 * Provides unified interface for VGA + Serial I/O
 */

#include <kernel.h>

/* Console state */
static bool console_initialized = false;

/* Initialize console subsystem */
void console_init(void) {
    if (console_initialized) return;
    
    /* Initialize hardware */
    vga_init();
    serial_init();
    
    /* Welcome message */
    console_print("FeatherOS Console Initialized\n");
    console_print("=====================================\n");
    console_print("Serial: COM1 (115200 8N1)\n");
    console_print("VGA: 80x25 Text Mode\n");
    console_print("=====================================\n\n");
    
    console_initialized = true;
}

/* Clear the console */
void console_clear(void) {
    vga_clear();
}

/* Put a character to console */
void console_putchar(char c) {
    vga_putchar(c);
}

/* Put a string to console */
void console_puts(const char *s) {
    while (*s) {
        console_putchar(*s++);
    }
}

/* Read a character from console (waits for input) */
char console_getchar(void) {
    /* Try keyboard first, then serial */
    if (keyboard_has_key()) {
        return keyboard_getchar();
    }
    
    while (!serial_has_input()) {
        __asm__ volatile("pause");
    }
    
    return serial_getchar();
}

/* Check if input is available */
bool console_has_input(void) {
    return keyboard_has_key() || serial_has_input();
}

/* Read a line from console with editing */
int console_readline(char *buf, size_t size) {
    if (size == 0) return 0;
    
    size_t input_len = 0;
    size_t max_len = size - 1;
    
    while (input_len < max_len) {
        char c = console_getchar();
        
        /* Handle special characters */
        if (c == '\n' || c == '\r') {
            console_putchar('\n');
            buf[input_len] = '\0';
            return input_len;
        }
        
        if (c == '\b' || c == 0x7F) {
            if (input_len > 0) {
                input_len--;
                console_putchar('\b');
                console_putchar(' ');
                console_putchar('\b');
            }
            continue;
        }
        
        if (c == 0x03) {  /* Ctrl+C */
            console_putchar('^');
            console_putchar('C');
            console_putchar('\n');
            buf[0] = '\0';
            return 0;
        }
        
        if (c == 0x04) {  /* Ctrl+D */
            if (input_len == 0) {
                return -1;  /* EOF */
            }
            continue;
        }
        
        if (c < 32 || c > 126) {
            continue;  /* Ignore non-printable characters */
        }
        
        /* Add to buffer and echo */
        buf[input_len++] = c;
        console_putchar(c);
    }
    
    buf[input_len] = '\0';
    return input_len;
}
