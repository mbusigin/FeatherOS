/* FeatherOS - Framebuffer Implementation
 * Sprint 16: VGA/Framebuffer
 */

#include <framebuffer.h>
#include <kernel.h>
#include <datastructures.h>
#include <string.h>

/*============================================================================
 * FRAMEBUFFER DEVICE
 *============================================================================*/

static framebuffer_t fb_info = {0};
static uint32_t *back_buffer = NULL;
static bool double_buffer_enabled = false;
static bool initialized = false;

/* Framebuffer statistics */
static fb_stats_t fb_stats = {0};

/*============================================================================
 * BUILT-IN 8x8 FONT
 *============================================================================*/

const uint8_t font_8x8[256][8] = {
    /* 0 - NULL */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 1 - SOH */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 2 - STX */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 3 - ETX */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 4 - EOT */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 5 - ENQ */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 6 - ACK */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 7 - BEL */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 8 - BS */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 9 - TAB */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 10 - LF */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 11 - VT */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 12 - FF */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 13 - CR */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 14 - SO */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 15 - SI */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 16 - DLE */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 17 - DC1 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 18 - DC2 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 19 - DC3 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 20 - DC4 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 21 - NAK */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 22 - SYN */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 23 - ETB */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 24 - CAN */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 25 - EM */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 26 - SUB */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 27 - ESC */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 28 - FS */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 29 - GS */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 30 - RS */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 31 - US */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 32 - SPACE */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 33 - ! */
    {0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00},
    /* 34 - " */
    {0x6C, 0x6C, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 35 - # */
    {0x24, 0x7E, 0x24, 0x24, 0x7E, 0x24, 0x00, 0x00},
    /* 36 - $ */
    {0x04, 0x62, 0x7C, 0x60, 0x3E, 0x22, 0x00, 0x00},
    /* 37 - % */
    {0x70, 0x2C, 0x18, 0x30, 0x66, 0x1C, 0x00, 0x00},
    /* 38 - & */
    {0x30, 0x48, 0x30, 0x5A, 0x84, 0x76, 0x00, 0x00},
    /* 39 - ' */
    {0x30, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 40 - ( */
    {0x08, 0x18, 0x30, 0x30, 0x30, 0x18, 0x08, 0x00},
    /* 41 - ) */
    {0x20, 0x18, 0x0C, 0x0C, 0x0C, 0x18, 0x20, 0x00},
    /* 42 - * */
    {0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00},
    /* 43 - + */
    {0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00},
    /* 44 - , */
    {0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x08, 0x00},
    /* 45 - - */
    {0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00},
    /* 46 - . */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00},
    /* 47 - / */
    {0x00, 0x02, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x00},
    /* 48 - 0 */
    {0x3C, 0x66, 0x6E, 0x76, 0x66, 0x3C, 0x00, 0x00},
    /* 49 - 1 */
    {0x18, 0x18, 0x38, 0x18, 0x18, 0x7E, 0x00, 0x00},
    /* 50 - 2 */
    {0x3C, 0x66, 0x06, 0x1C, 0x30, 0x7E, 0x00, 0x00},
    /* 51 - 3 */
    {0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C, 0x00},
    /* 52 - 4 */
    {0x0C, 0x1C, 0x3C, 0x6C, 0x7E, 0x0C, 0x00, 0x00},
    /* 53 - 5 */
    {0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C, 0x00},
    /* 54 - 6 */
    {0x3C, 0x66, 0x60, 0x7C, 0x66, 0x3C, 0x00, 0x00},
    /* 55 - 7 */
    {0x7E, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x00, 0x00},
    /* 56 - 8 */
    {0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C, 0x00},
    /* 57 - 9 */
    {0x3C, 0x66, 0x66, 0x3E, 0x06, 0x66, 0x3C, 0x00},
    /* 58 - : */
    {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00},
    /* 59 - ; */
    {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x08, 0x00},
    /* 60 - < */
    {0x08, 0x18, 0x30, 0x60, 0x30, 0x18, 0x08, 0x00},
    /* 61 - = */
    {0x00, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x00, 0x00},
    /* 62 - > */
    {0x20, 0x18, 0x0C, 0x06, 0x0C, 0x18, 0x20, 0x00},
    /* 63 - ? */
    {0x3C, 0x66, 0x06, 0x1C, 0x18, 0x00, 0x18, 0x00},
    /* 64 - @ */
    {0x3C, 0x66, 0x6E, 0x6E, 0x60, 0x3E, 0x00, 0x00},
    /* 65 - A */
    {0x18, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x00, 0x00},
    /* 66 - B */
    {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00},
    /* 67 - C */
    {0x3C, 0x66, 0x60, 0x60, 0x66, 0x3C, 0x00, 0x00},
    /* 68 - D */
    {0x78, 0x6C, 0x66, 0x66, 0x6C, 0x78, 0x00, 0x00},
    /* 69 - E */
    {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00},
    /* 70 - F */
    {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00},
    /* 71 - G */
    {0x3C, 0x66, 0x60, 0x6E, 0x66, 0x3E, 0x00, 0x00},
    /* 72 - H */
    {0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00, 0x00},
    /* 73 - I */
    {0x7E, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00, 0x00},
    /* 74 - J */
    {0x3E, 0x0C, 0x0C, 0x0C, 0x6C, 0x38, 0x00, 0x00},
    /* 75 - K */
    {0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x00, 0x00},
    /* 76 - L */
    {0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00, 0x00},
    /* 77 - M */
    {0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x00, 0x00},
    /* 78 - N */
    {0x66, 0x76, 0x7E, 0x7E, 0x6E, 0x66, 0x00, 0x00},
    /* 79 - O */
    {0x3C, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00, 0x00},
    /* 80 - P */
    {0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x00, 0x00},
    /* 81 - Q */
    {0x3C, 0x66, 0x66, 0x66, 0x6C, 0x36, 0x00, 0x00},
    /* 82 - R */
    {0x7C, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0x00, 0x00},
    /* 83 - S */
    {0x3C, 0x66, 0x30, 0x18, 0x0C, 0x66, 0x3C, 0x00},
    /* 84 - T */
    {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00},
    /* 85 - U */
    {0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00, 0x00},
    /* 86 - V */
    {0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00, 0x00},
    /* 87 - W */
    {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x00, 0x00},
    /* 88 - X */
    {0x66, 0x66, 0x3C, 0x3C, 0x66, 0x66, 0x00, 0x00},
    /* 89 - Y */
    {0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x00, 0x00},
    /* 90 - Z */
    {0x7E, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E, 0x00},
    /* 91 - [ */
    {0x38, 0x30, 0x30, 0x30, 0x30, 0x30, 0x38, 0x00},
    /* 92 - \ */
    {0x60, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x02, 0x00},
    /* 93 - ] */
    {0x1C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1C, 0x00},
    /* 94 - ^ */
    {0x18, 0x3C, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 95 - _ */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00},
    /* 96 - ` */
    {0x18, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 97 - a */
    {0x00, 0x00, 0x3C, 0x06, 0x7E, 0x66, 0x00, 0x00},
    /* 98 - b */
    {0x60, 0x60, 0x6C, 0x76, 0x66, 0x66, 0x7C, 0x00},
    /* 99 - c */
    {0x00, 0x00, 0x3C, 0x60, 0x60, 0x66, 0x3C, 0x00},
    /* 100 - d */
    {0x06, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x3C, 0x00},
    /* 101 - e */
    {0x00, 0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3C, 0x00},
    /* 102 - f */
    {0x1C, 0x30, 0x30, 0x7C, 0x30, 0x30, 0x30, 0x00},
    /* 103 - g */
    {0x00, 0x00, 0x3C, 0x66, 0x66, 0x3E, 0x06, 0x7C},
    /* 104 - h */
    {0x60, 0x60, 0x6C, 0x76, 0x66, 0x66, 0x00, 0x00},
    /* 105 - i */
    {0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x7E, 0x00},
    /* 106 - j */
    {0x0C, 0x00, 0x3C, 0x0C, 0x0C, 0x6C, 0x38, 0x00},
    /* 107 - k */
    {0x60, 0x60, 0x66, 0x6C, 0x78, 0x6C, 0x00, 0x00},
    /* 108 - l */
    {0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00},
    /* 109 - m */
    {0x00, 0x00, 0x6C, 0x7F, 0x7F, 0x6B, 0x00, 0x00},
    /* 110 - n */
    {0x00, 0x00, 0x7C, 0x66, 0x66, 0x66, 0x00, 0x00},
    /* 111 - o */
    {0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C, 0x00},
    /* 112 - p */
    {0x00, 0x00, 0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60},
    /* 113 - q */
    {0x00, 0x00, 0x3C, 0x66, 0x66, 0x3C, 0x06, 0x06},
    /* 114 - r */
    {0x00, 0x00, 0x6E, 0x76, 0x60, 0x60, 0x00, 0x00},
    /* 115 - s */
    {0x00, 0x00, 0x3E, 0x60, 0x3C, 0x06, 0x7C, 0x00},
    /* 116 - t */
    {0x30, 0x30, 0x7C, 0x30, 0x30, 0x34, 0x18, 0x00},
    /* 117 - u */
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x6E, 0x3C, 0x00},
    /* 118 - v */
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00},
    /* 119 - w */
    {0x00, 0x00, 0x63, 0x6B, 0x7F, 0x77, 0x00, 0x00},
    /* 120 - x */
    {0x00, 0x00, 0x66, 0x3C, 0x3C, 0x66, 0x00, 0x00},
    /* 121 - y */
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x7C},
    /* 122 - z */
    {0x00, 0x00, 0x7E, 0x0C, 0x18, 0x30, 0x7E, 0x00},
    /* 123 - { */
    {0x0C, 0x18, 0x18, 0x70, 0x18, 0x18, 0x0C, 0x00},
    /* 124 - | */
    {0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00},
    /* 125 - } */
    {0x30, 0x18, 0x18, 0x0E, 0x18, 0x18, 0x30, 0x00},
    /* 126 - ~ */
    {0x72, 0x4C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 127 - DEL */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};

/*============================================================================
 * FRAMEBUFFER INITIALIZATION
 *============================================================================*/

int framebuffer_init(void) {
    if (initialized) {
        return 0;
    }
    
    /* For now, set up a simple 80x25 text mode framebuffer simulation */
    /* In real hardware, we would use VBE to set a graphics mode */
    
    fb_info.width = 80 * FONT_WIDTH;   /* 640 pixels */
    fb_info.height = 25 * FONT_HEIGHT; /* 200 pixels */
    fb_info.pitch = fb_info.width * 4; /* 4 bytes per pixel (32-bit) */
    fb_info.bpp = 32;
    fb_info.size = fb_info.pitch * fb_info.height;
    fb_info.framebuffer = (uint32_t*)0xB8000; /* VGA text mode memory */
    fb_info.initialized = true;
    
    /* Color format: BGR (like VGA) */
    fb_info.red_pos = 16;
    fb_info.red_size = 8;
    fb_info.green_pos = 8;
    fb_info.green_size = 8;
    fb_info.blue_pos = 0;
    fb_info.blue_size = 8;
    
    initialized = true;
    
    console_print("Framebuffer initialized: %dx%d @ %d bpp\n",
                  fb_info.width, fb_info.height, fb_info.bpp);
    
    return 0;
}

framebuffer_t *framebuffer_get_info(void) {
    return &fb_info;
}

/*============================================================================
 * PIXEL OPERATIONS
 *============================================================================*/

static inline uint32_t *get_pixel_addr(int x, int y) {
    return fb_info.framebuffer + y * (fb_info.pitch / 4) + x;
}

void fb_putpixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= (int)fb_info.width || y < 0 || y >= (int)fb_info.height) {
        return;
    }
    
    uint32_t *addr = get_pixel_addr(x, y);
    if (double_buffer_enabled && back_buffer) {
        back_buffer[y * (fb_info.pitch / 4) + x] = color;
    } else {
        *addr = color;
    }
    
    fb_stats.pixels_drawn++;
}

uint32_t fb_getpixel(int x, int y) {
    if (x < 0 || x >= (int)fb_info.width || y < 0 || y >= (int)fb_info.height) {
        return 0;
    }
    
    uint32_t *addr = get_pixel_addr(x, y);
    return *addr;
}

void fb_putpixel_clip(int x, int y, uint32_t color, int clip_x, int clip_y, int clip_w, int clip_h) {
    if (x < clip_x || x >= clip_x + clip_w || y < clip_y || y >= clip_y + clip_h) {
        return;
    }
    fb_putpixel(x, y, color);
}

/*============================================================================
 * 2D PRIMITIVES
 *============================================================================*/

void fb_draw_hline(int x1, int y, int width, uint32_t color) {
    for (int x = x1; x < x1 + width; x++) {
        fb_putpixel(x, y, color);
    }
}

void fb_draw_vline(int x, int y1, int height, uint32_t color) {
    for (int y = y1; y < y1 + height; y++) {
        fb_putpixel(x, y, color);
    }
}

void fb_draw_line(int x1, int y1, int x2, int y2, uint32_t color) {
    /* Bresenham's algorithm */
    int dx = x2 - x1;
    int dy = y2 - y1;
    int sx = dx > 0 ? 1 : -1;
    int sy = dy > 0 ? 1 : -1;
    
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    
    int err = (dx > dy ? dx : -dy) / 2;
    int e2;
    
    while (1) {
        fb_putpixel(x1, y1, color);
        
        if (x1 == x2 && y1 == y2) break;
        
        e2 = err;
        if (e2 > -dx) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dy) {
            err += dx;
            y1 += sy;
        }
    }
}

void fb_draw_rect(int x, int y, int width, int height, uint32_t color) {
    fb_draw_hline(x, y, width, color);
    fb_draw_hline(x, y + height - 1, width, color);
    fb_draw_vline(x, y, height, color);
    fb_draw_vline(x + width - 1, y, height, color);
}

void fb_draw_rect_fill(int x, int y, int width, int height, uint32_t color) {
    for (int dy = 0; dy < height; dy++) {
        fb_draw_hline(x, y + dy, width, color);
    }
}

void fb_draw_circle(int cx, int cy, int radius, uint32_t color) {
    int x = radius;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        fb_putpixel(cx + x, cy + y, color);
        fb_putpixel(cx + y, cy + x, color);
        fb_putpixel(cx - y, cy + x, color);
        fb_putpixel(cx - x, cy + y, color);
        fb_putpixel(cx - x, cy - y, color);
        fb_putpixel(cx - y, cy - x, color);
        fb_putpixel(cx + y, cy - x, color);
        fb_putpixel(cx + x, cy - y, color);
        
        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void fb_draw_circle_fill(int cx, int cy, int radius, uint32_t color) {
    int x = radius;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        fb_draw_hline(cx - x, cy + y, x * 2 + 1, color);
        fb_draw_hline(cx - x, cy - y, x * 2 + 1, color);
        fb_draw_hline(cx - y, cy + x, y * 2 + 1, color);
        fb_draw_hline(cx - y, cy - x, y * 2 + 1, color);
        
        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

/*============================================================================
 * BITMAP BLITTING
 *============================================================================*/

void fb_blit(uint32_t *src, int src_x, int src_y, int dst_x, int dst_y, 
             int width, int height, int src_pitch) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint32_t color = src[(src_y + y) * (src_pitch / 4) + src_x + x];
            fb_putpixel(dst_x + x, dst_y + y, color);
        }
    }
    fb_stats.pixels_blitted += width * height;
}

void fb_blit_color_key(uint32_t *src, int src_x, int src_y, int dst_x, int dst_y,
                       int width, int height, int src_pitch, uint32_t key) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint32_t color = src[(src_y + y) * (src_pitch / 4) + src_x + x];
            if (color != key) {
                fb_putpixel(dst_x + x, dst_y + y, color);
            }
        }
    }
}

void fb_stretch(uint32_t *src, int src_x, int src_y, int src_w, int src_h,
                int dst_x, int dst_y, int dst_w, int dst_h, int src_pitch) {
    int32_t x_ratio = (src_w << 16) / dst_w + 1;
    int32_t y_ratio = (src_h << 16) / dst_h + 1;
    
    for (int y = 0; y < dst_h; y++) {
        for (int x = 0; x < dst_w; x++) {
            int sx = (x * x_ratio) >> 16;
            int sy = (y * y_ratio) >> 16;
            uint32_t color = src[(src_y + sy) * (src_pitch / 4) + src_x + sx];
            fb_putpixel(dst_x + x, dst_y + y, color);
        }
    }
}

/*============================================================================
 * FONT RENDERING
 *============================================================================*/

void fb_draw_char(int x, int y, char c, uint32_t fg_color, uint32_t bg_color) {
    const uint8_t *glyph = font_8x8[(uint8_t)c];
    
    for (int row = 0; row < 8; row++) {
        uint8_t line = glyph[row];
        for (int col = 0; col < 8; col++) {
            uint32_t color = (line & (1 << (7 - col))) ? fg_color : bg_color;
            fb_putpixel(x + col, y + row, color);
        }
    }
    
    fb_stats.chars_rendered++;
}

void fb_draw_string(int x, int y, const char *str, uint32_t fg_color, uint32_t bg_color) {
    int x_pos = x;
    while (*str) {
        fb_draw_char(x_pos, y, *str, fg_color, bg_color);
        x_pos += FONT_WIDTH;
        str++;
    }
}

int fb_measure_string(const char *str) {
    int len = 0;
    while (*str) {
        len += FONT_WIDTH;
        str++;
    }
    return len;
}

void fb_draw_string_centered(int x, int y, int width, const char *str,
                             uint32_t fg_color, uint32_t bg_color) {
    int str_width = fb_measure_string(str);
    int start_x = x + (width - str_width) / 2;
    fb_draw_string(start_x, y, str, fg_color, bg_color);
}

/*============================================================================
 * WINDOW/DIALOG PRIMITIVES
 *============================================================================*/

void fb_draw_button(int x, int y, int width, int height, const char *text,
                    uint32_t fg_color, uint32_t bg_color, uint32_t border_color) {
    fb_draw_rect_fill(x, y, width, height, bg_color);
    fb_draw_rect(x, y, width, height, border_color);
    
    /* Draw text centered */
    fb_draw_string_centered(x, y + (height - FONT_HEIGHT) / 2, width, text, fg_color, bg_color);
}

void fb_draw_window(int x, int y, int width, int height, const char *title) {
    /* Title bar */
    fb_draw_rect_fill(x, y, width, FONT_HEIGHT + 4, COLOR_LIGHT_BLUE);
    fb_draw_string_centered(x, y + 2, width, title, COLOR_WHITE, COLOR_LIGHT_BLUE);
    
    /* Window body */
    fb_draw_rect_fill(x, y + FONT_HEIGHT + 4, width, height - FONT_HEIGHT - 4, COLOR_WHITE);
    
    /* Border */
    fb_draw_rect(x, y, width, height, COLOR_DARK_GRAY);
}

void fb_draw_progress_bar(int x, int y, int width, int height,
                          int progress, uint32_t fg_color, uint32_t bg_color) {
    fb_draw_rect_fill(x, y, width, height, bg_color);
    fb_draw_rect(x, y, width, height, COLOR_DARK_GRAY);
    
    int fill_width = (width - 2) * progress / 100;
    if (fill_width > 0) {
        fb_draw_rect_fill(x + 1, y + 1, fill_width, height - 2, fg_color);
    }
}

void fb_draw_scrollbar(int x, int y, int height, int thumb_pos, int thumb_size) {
    fb_draw_vline(x, y, height, COLOR_DARK_GRAY);
    
    int thumb_y = y + (height - thumb_size) * thumb_pos / 100;
    fb_draw_rect_fill(x - 2, thumb_y, 5, thumb_size, COLOR_LIGHT_GRAY);
}

/*============================================================================
 * DOUBLE BUFFERING
 *============================================================================*/

void fb_enable_double_buffer(void) {
    if (!back_buffer) {
        back_buffer = (uint32_t*)kmalloc(fb_info.size);
    }
    double_buffer_enabled = true;
}

void fb_disable_double_buffer(void) {
    double_buffer_enabled = false;
}

bool fb_double_buffer_enabled(void) {
    return double_buffer_enabled;
}

uint32_t *fb_get_back_buffer(void) {
    return back_buffer;
}

void fb_swap_buffers(void) {
    if (back_buffer && fb_info.framebuffer) {
        memcpy(fb_info.framebuffer, back_buffer, fb_info.size);
    }
}

void framebuffer_clear(uint32_t color) {
    if (double_buffer_enabled && back_buffer) {
        for (uint32_t i = 0; i < fb_info.size / 4; i++) {
            back_buffer[i] = color;
        }
    } else {
        for (uint32_t i = 0; i < fb_info.size / 4; i++) {
            fb_info.framebuffer[i] = color;
        }
    }
    fb_stats.pixels_cleared += (fb_info.width * fb_info.height);
}

void framebuffer_sync(void) {
    fb_swap_buffers();
}

/*============================================================================
 * SCREENSHOT
 *============================================================================*/

void fb_screenshot(uint32_t *buffer, int width, int height) {
    if (buffer && fb_info.framebuffer) {
        uint32_t copy_width = (uint32_t)((width < (int)fb_info.width) ? (uint32_t)width : fb_info.width);
        uint32_t copy_height = (uint32_t)((height < (int)fb_info.height) ? (uint32_t)height : fb_info.height);
        uint32_t pitch = fb_info.pitch / 4;
        
        for (uint32_t y = 0; y < copy_height; y++) {
            for (uint32_t x = 0; x < copy_width; x++) {
                buffer[y * width + x] = fb_info.framebuffer[y * pitch + x];
            }
        }
    }
}

/*============================================================================
 * STATISTICS
 *============================================================================*/

fb_stats_t *fb_get_stats(void) {
    return &fb_stats;
}

void fb_print_stats(void) {
    console_print("\n=== Framebuffer Statistics ===\n");
    console_print("Pixels drawn: %lu\n", fb_stats.pixels_drawn);
    console_print("Pixels cleared: %lu\n", fb_stats.pixels_cleared);
    console_print("Pixels blitted: %lu\n", fb_stats.pixels_blitted);
    console_print("Characters rendered: %lu\n", fb_stats.chars_rendered);
    console_print("FPS: %u\n", fb_stats.fps);
}
