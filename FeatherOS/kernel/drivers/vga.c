/* FeatherOS - VGA Text Mode Driver */

#include <stdint.h>

#define VGA_MEMORY 0xB8000
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

static int vga_column = 0;
static int vga_row = 0;

void vga_init(void);
void vga_clear(void);
void vga_putchar(char c);
void vga_write(const char *str);

void vga_init(void) {
    vga_column = 0;
    vga_row = 0;
    vga_clear();
}

void vga_clear(void) {
    volatile uint16_t *vga = (volatile uint16_t *)VGA_MEMORY;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = 0x0F00;  /* White on black */
    }
    vga_column = 0;
    vga_row = 0;
}

void vga_putchar(char c) {
    volatile uint16_t *vga = (volatile uint16_t *)VGA_MEMORY;
    
    if (c == '\n') {
        vga_column = 0;
        vga_row++;
        return;
    }
    
    if (c == '\r') {
        vga_column = 0;
        return;
    }
    
    if (vga_column >= VGA_WIDTH) {
        vga_column = 0;
        vga_row++;
    }
    
    if (vga_row >= VGA_HEIGHT) {
        /* Scroll */
        for (int y = 0; y < VGA_HEIGHT - 1; y++) {
            for (int x = 0; x < VGA_WIDTH; x++) {
                vga[y * VGA_WIDTH + x] = vga[(y + 1) * VGA_WIDTH + x];
            }
        }
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = 0x0F00;
        }
        vga_row = VGA_HEIGHT - 1;
    }
    
    vga[vga_row * VGA_WIDTH + vga_column] = (0x0F << 8) | c;
    vga_column++;
}

void vga_write(const char *str) {
    while (*str) {
        vga_putchar(*str++);
    }
}
