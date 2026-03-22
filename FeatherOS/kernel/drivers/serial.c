/* FeatherOS - 16550 UART Serial Driver
 * Sprint 3: Console & Basic I/O
 */

#include <kernel.h>

/* Standard serial ports */
#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8

/* 16550 UART registers (offset from base port) */
#define UART_DATA     0   /* Data register (R/W) */
#define UART_DLL      0   /* Divisor Latch Low (when DLAB=1) */
#define UART_IER      1   /* Interrupt Enable Register */
#define UART_DLM      1   /* Divisor Latch High (when DLAB=1) */
#define UART_IIR      2   /* Interrupt Identification Register (R) */
#define UART_FCR      2   /* FIFO Control Register (W) */
#define UART_LCR      3   /* Line Control Register */
#define UART_MCR      4   /* Modem Control Register */
#define UART_LSR      5   /* Line Status Register */
#define UART_MSR      6   /* Modem Status Register */
#define UART_SR       7   /* Scratch Register */

/* LCR bits */
#define LCR_DLAB      0x80  /* Divisor Latch Access Bit */
#define LCR_8N1       0x03  /* 8 data bits, no parity, 1 stop bit */

/* LSR bits */
#define LSR_DATA_READY   0x01
#define LSR_OVERRUN_ERR  0x02
#define LSR_PARITY_ERR   0x04
#define LSR_FRAMING_ERR  0x08
#define LSR_BREAK_INT    0x10
#define LSR_THR_EMPTY    0x20
#define LSR_TSR_EMPTY    0x40

/* Circular buffer for receive */
#define SERIAL_BUFFER_SIZE 256

typedef struct {
    char buffer[SERIAL_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile bool has_data;
} serial_buffer_t;

static serial_buffer_t rx_buffer = {
    .head = 0,
    .tail = 0,
    .has_data = false
};

static uint16_t current_port = COM1;

/* Initialize a specific serial port */
void serial_init_port(uint16_t port) {
    current_port = port;
    
    /* Disable all interrupts */
    outb(port + UART_IER, 0x00);
    
    /* Set DLAB to configure baud rate */
    outb(port + UART_LCR, LCR_DLAB);
    
    /* Set divisor to 115200 / 115200 = 1 (115200 baud) */
    outb(port + UART_DLL, 0x01);
    outb(port + UART_DLM, 0x00);
    
    /* 8N1 mode */
    outb(port + UART_LCR, LCR_8N1);
    
    /* Enable FIFO, clear receive/transmit FIFOs */
    outb(port + UART_FCR, 0x01);  /* Enable FIFO, trigger at 1 byte */
    
    /* IRQs disabled, RTS/DSR set */
    outb(port + UART_MCR, 0x0B);
    
    /* Loopback mode test */
    outb(port + UART_MCR, 0x1E);
    outb(port + UART_DATA, 0xAE);
    
    /* Check if serial is functioning */
    if (inb(port + UART_DATA) != 0xAE) {
        /* Serial port not found */
        return;
    }
    
    /* Set normal operating mode */
    outb(port + UART_MCR, 0x0F);
    
    /* Clear receive buffer */
    inb(port + UART_DATA);
}

/* Initialize serial ports (COM1-COM4) */
void serial_init(void) {
    serial_init_port(COM1);
    serial_init_port(COM2);
    serial_init_port(COM3);
    serial_init_port(COM4);
}

/* Check if data is ready to be read */
int serial_received(uint16_t port) {
    return inb(port + UART_LSR) & LSR_DATA_READY;
}

/* Check if we can transmit */
int serial_can_transmit(uint16_t port) {
    return inb(port + UART_LSR) & LSR_THR_EMPTY;
}

/* Read a character from serial port */
char serial_read_char(uint16_t port) {
    while (!serial_received(port)) {
        /* Busy wait */
        __asm__ volatile("pause");
    }
    return inb(port + UART_DATA);
}

/* Write a character to serial port */
void serial_write_char(uint16_t port, char c) {
    while (!serial_can_transmit(port)) {
        __asm__ volatile("pause");
    }
    outb(port + UART_DATA, (uint8_t)c);
}

/* Write to current serial port (COM1) */
void serial_putchar(char c) {
    serial_write_char(current_port, c);
}

/* Write string to serial port */
void serial_puts(const char *s) {
    while (*s) {
        serial_putchar(*s++);
    }
}

/* Read from serial buffer */
char serial_getchar(void) {
    while (!serial_has_input()) {
        __asm__ volatile("pause");
    }
    
    char c = rx_buffer.buffer[rx_buffer.tail];
    rx_buffer.tail = (rx_buffer.tail + 1) % SERIAL_BUFFER_SIZE;
    
    if (rx_buffer.tail == rx_buffer.head) {
        rx_buffer.has_data = false;
    }
    
    return c;
}

/* Check if serial input is available */
bool serial_has_input(void) {
    /* Poll for new data */
    if (serial_received(current_port)) {
        char c = inb(current_port + UART_DATA);
        rx_buffer.buffer[rx_buffer.head] = c;
        rx_buffer.head = (rx_buffer.head + 1) % SERIAL_BUFFER_SIZE;
        rx_buffer.has_data = true;
    }
    
    return rx_buffer.has_data;
}

/* Interrupt handler (called from ISR) */
void serial_irq_handler(void) {
    while (serial_received(current_port)) {
        char c = inb(current_port + UART_DATA);
        
        /* Add to circular buffer */
        rx_buffer.buffer[rx_buffer.head] = c;
        rx_buffer.head = (rx_buffer.head + 1) % SERIAL_BUFFER_SIZE;
        
        if (rx_buffer.head != rx_buffer.tail) {
            rx_buffer.has_data = true;
        }
        
        /* Buffer overflow - drop oldest */
        if (rx_buffer.head == rx_buffer.tail) {
            rx_buffer.tail = (rx_buffer.tail + 1) % SERIAL_BUFFER_SIZE;
        }
    }
}
