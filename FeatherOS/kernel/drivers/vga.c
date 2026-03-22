/* FeatherOS - VGA Text Mode Driver */

#include <kernel.h>

#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static size_t vga_col = 0;
static size_t vga_row = 0;

void vga_init(void) {
    vga_col = 0;
    vga_row = 0;
}

void vga_clear(void) {
    volatile uint16_t *vga = (volatile uint16_t *)VGA_MEMORY;
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = 0x0F00;
    }
}

void vga_putchar(char c) {
    volatile uint16_t *vga = (volatile uint16_t *)VGA_MEMORY;
    
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
        if (vga_row >= VGA_HEIGHT) vga_row = 0;
        return;
    }
    
    if (c == '\r') {
        vga_col = 0;
        return;
    }
    
    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        vga_row++;
        if (vga_row >= VGA_HEIGHT) vga_row = 0;
    }
    
    vga[vga_row * VGA_WIDTH + vga_col] = (0x0F << 8) | (unsigned char)c;
    vga_col++;
}
