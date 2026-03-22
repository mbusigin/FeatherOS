/* FeatherOS - VGA Text Mode Driver
 * Sprint 3: Console & Basic I/O
 */

#include <kernel.h>

/* VGA text mode I/O ports */
#define VGA_CTRL_PORT 0x3D4
#define VGA_DATA_PORT 0x3D5

/* VGA memory-mapped I/O */
#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

/* VGA color attributes */
typedef enum {
    VGA_BLACK = 0,
    VGA_BLUE = 1,
    VGA_GREEN = 2,
    VGA_CYAN = 3,
    VGA_RED = 4,
    VGA_MAGENTA = 5,
    VGA_BROWN = 6,
    VGA_LIGHT_GREY = 7,
    VGA_DARK_GREY = 8,
    VGA_LIGHT_BLUE = 9,
    VGA_LIGHT_GREEN = 10,
    VGA_LIGHT_CYAN = 11,
    VGA_LIGHT_RED = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_YELLOW = 14,
    VGA_WHITE = 15
} vga_color_t;

/* VGA entry structure */
typedef uint16_t vga_entry_t;

static inline vga_entry_t vga_entry(unsigned char c, uint8_t color) {
    return (vga_entry_t)c | (vga_entry_t)color << 8;
}

static inline uint8_t vga_entry_color(vga_entry_t entry) {
    return (uint8_t)(entry >> 8);
}

static inline unsigned char vga_entry_char(vga_entry_t entry) {
    return (unsigned char)(entry & 0xFF);
}

/* Global state */
static volatile vga_entry_t *vga_buffer = (volatile vga_entry_t *)VGA_MEMORY;
static uint16_t vga_x = 0;
static uint16_t vga_y = 0;
static uint8_t vga_color = VGA_LIGHT_GREY | (VGA_BLACK << 4);

/* Get current VGA entry at position */
static vga_entry_t vga_get_entry(uint16_t x, uint16_t y) {
    return vga_buffer[y * VGA_WIDTH + x];
}

/* Set VGA entry at position */
static void vga_set_entry(uint16_t x, uint16_t y, vga_entry_t entry) {
    vga_buffer[y * VGA_WIDTH + x] = entry;
}

/* Initialize VGA text mode */
void vga_init(void) {
    vga_color = VGA_LIGHT_GREY | (VGA_BLACK << 4);
    vga_clear();
    vga_enable_cursor();
    vga_set_cursor(0, 0);
}

/* Clear the screen */
void vga_clear(void) {
    for (uint16_t y = 0; y < VGA_HEIGHT; y++) {
        for (uint16_t x = 0; x < VGA_WIDTH; x++) {
            vga_set_entry(x, y, vga_entry(' ', vga_color));
        }
    }
    vga_x = 0;
    vga_y = 0;
}

/* Set foreground and background color */
void vga_set_color(uint8_t fg, uint8_t bg) {
    vga_color = (bg << 4) | (fg & 0x0F);
}

/* Get current cursor position */
void vga_get_cursor(uint16_t *x, uint16_t *y) {
    *x = vga_x;
    *y = vga_y;
}

/* Set cursor position */
void vga_set_cursor(uint16_t x, uint16_t y) {
    vga_x = x < VGA_WIDTH ? x : VGA_WIDTH - 1;
    vga_y = y < VGA_HEIGHT ? y : VGA_HEIGHT - 1;
    
    uint16_t pos = vga_y * VGA_WIDTH + vga_x;
    
    outb(VGA_CTRL_PORT, 0x0F);
    outb(VGA_DATA_PORT, (uint8_t)(pos & 0xFF));
    outb(VGA_CTRL_PORT, 0x0E);
    outb(VGA_DATA_PORT, (uint8_t)((pos >> 8) & 0xFF));
}

/* Enable text cursor */
void vga_enable_cursor(void) {
    outb(VGA_CTRL_PORT, 0x0A);
    uint8_t cur_start = inb(VGA_DATA_PORT);
    outb(VGA_DATA_PORT, (cur_start & 0xC0) | 0x0D);  /* Scanline 13 */
    
    outb(VGA_CTRL_PORT, 0x0B);
    uint8_t cur_end = inb(VGA_DATA_PORT);
    outb(VGA_DATA_PORT, (cur_end & 0xE0) | 0x0F);   /* Scanline 15 */
}

/* Disable text cursor */
void vga_disable_cursor(void) {
    outb(VGA_CTRL_PORT, 0x0A);
    outb(VGA_DATA_PORT, 0x20);
}

/* Scroll the screen up by one line */
static void vga_scroll(void) {
    /* Move all lines up */
    for (uint16_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (uint16_t x = 0; x < VGA_WIDTH; x++) {
            vga_set_entry(x, y, vga_get_entry(x, y + 1));
        }
    }
    
    /* Clear bottom line */
    for (uint16_t x = 0; x < VGA_WIDTH; x++) {
        vga_set_entry(x, VGA_HEIGHT - 1, vga_entry(' ', vga_color));
    }
    
    /* Move cursor to start of last line */
    vga_y = VGA_HEIGHT - 1;
}

/* Put a character at current cursor position and advance */
void vga_putchar(char c) {
    if (c == '\n') {
        vga_x = 0;
        vga_y++;
    } else if (c == '\r') {
        vga_x = 0;
    } else if (c == '\t') {
        vga_x = (vga_x + 8) & ~7;  /* Tab to next 8-column boundary */
        if (vga_x >= VGA_WIDTH) {
            vga_x = 0;
            vga_y++;
        }
    } else if (c == '\b') {
        if (vga_x > 0) {
            vga_x--;
            vga_set_entry(vga_x, vga_y, vga_entry(' ', vga_color));
        }
    } else {
        vga_set_entry(vga_x, vga_y, vga_entry(c, vga_color));
        vga_x++;
    }
    
    /* Handle line wrap and scrolling */
    if (vga_x >= VGA_WIDTH) {
        vga_x = 0;
        vga_y++;
    }
    
    if (vga_y >= VGA_HEIGHT) {
        vga_scroll();
    }
    
    /* Update hardware cursor */
    vga_set_cursor(vga_x, vga_y);
}
