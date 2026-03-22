/* FeatherOS - Serial Driver Implementation */

#include <kernel.h>

/* Serial port I/O ports */
#define COM1_PORT 0x3F8

void serial_init(void) {
    /* Initialize serial port COM1 */
    outb(COM1_PORT + 1, 0x00);    /* Disable interrupts */
    outb(COM1_PORT + 3, 0x80);    /* Enable DLAB */
    outb(COM1_PORT + 0, 0x03);    /* Set divisor to 3 (38400 baud) */
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x03);    /* 8 bits, no parity, one stop bit */
    outb(COM1_PORT + 2, 0xC7);    /* Enable FIFO */
    outb(COM1_PORT + 4, 0x0B);    /* IRQs enabled, RTS/DSR set */
}

void serial_putchar(char c) {
    if (c == '\n') serial_putchar('\r');
    
    /* Wait for transmit buffer to be empty */
    while ((inb(COM1_PORT + 5) & 0x20) == 0);
    outb(COM1_PORT, c);
}

char serial_getchar(void) {
    while ((inb(COM1_PORT + 5) & 0x01) == 0);
    return inb(COM1_PORT);
}

int serial_write(const char *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        serial_putchar(buf[i]);
    }
    return len;
}
