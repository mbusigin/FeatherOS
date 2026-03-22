/* FeatherOS - Serial Driver (COM1) */

#include <kernel.h>

#define COM1_PORT 0x3F8

void serial_init(void) {
    /* Disable interrupts */
    outb(COM1_PORT + 1, 0x00);
    
    /* Set baud rate to 38400 */
    outb(COM1_PORT + 3, 0x80);    /* Enable DLAB */
    outb(COM1_PORT + 0, 0x03);    /* Baud low */
    outb(COM1_PORT + 1, 0x00);    /* Baud high */
    
    /* 8 bits, no parity, one stop bit */
    outb(COM1_PORT + 3, 0x03);
    
    /* Enable FIFO */
    outb(COM1_PORT + 2, 0xC7);
    
    /* IRQs enabled, RTS/DSR set */
    outb(COM1_PORT + 4, 0x0B);
    
    /* Loopback mode off */
    outb(COM1_PORT + 4, 0x0F);
}

void serial_putchar(char c) {
    if (c == '\n') serial_putchar('\r');
    
    /* Wait for transmit buffer empty */
    for (int i = 0; i < 128; i++) {
        if (inb(COM1_PORT + 5) & 0x20) break;
    }
    outb(COM1_PORT, c);
}

char serial_getchar(void) {
    for (int i = 0; i < 128; i++) {
        if (inb(COM1_PORT + 5) & 0x01) break;
    }
    return inb(COM1_PORT);
}
