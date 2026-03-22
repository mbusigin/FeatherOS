/* FeatherOS - Framebuffer & Graphics
 * Sprint 16: VGA/Framebuffer
 */

#ifndef FEATHEROS_FRAMEBUFFER_H
#define FEATHEROS_FRAMEBUFFER_H

#include <stdint.h>
#include <stdbool.h>

/*============================================================================
 * VESA / VBE INFORMATION
 *============================================================================*/

/* VBE Mode Attributes */
#define VBE_MODE_SUPPORTED    0x01
#define VBE_MODE_EXT_INFO    0x08
#define VBE_MODE_COLOR       0x10
#define VBE_MODE_GRAPHICS    0x20
#define VBE_MODE_NOT_VGA     0x40
#define VBE_MODE_LINEAR       0x80

/* VESA Mode Numbers */
#define VBE_MODE_640x480x8   0x101
#define VBE_MODE_800x600x15  0x110
#define VBE_MODE_800x600x16  0x111
#define VBE_MODE_800x600x24  0x115
#define VBE_MODE_1024x768x8  0x105
#define VBE_MODE_1024x768x15 0x118
#define VBE_MODE_1024x768x16 0x119
#define VBE_MODE_1024x768x24 0x11B
#define VBE_MODE_1280x1024x8 0x107
#define VBE_MODE_1280x1024x15 0x11C
#define VBE_MODE_1280x1024x16 0x11D
#define VBE_MODE_1280x1024x24 0x11F

/* VBE Info Block (at 0x500) */
typedef struct {
    char signature[4];           /* "VESA" */
    uint16_t version;            /* VBE version */
    uint32_t oem_string;        /* OEM string pointer */
    uint32_t capabilities;       /* Capabilities */
    uint32_t vbe_modes_ptr;     /* Pointer to mode list */
    uint16_t memory_64k;         /* Video memory in 64KB blocks */
    uint16_t oem_software_rev;  /* OEM software revision */
    uint32_t oem_vendor_name;   /* Vendor name */
    uint32_t oem_product_name;  /* Product name */
    uint32_t oem_product_rev;   /* Product revision */
    uint8_t reserved[222];      /* Reserved */
    uint8_t oem_data[256];      /* OEM data */
} __attribute__((packed)) vbe_info_t;

/* VBE Mode Info Block */
typedef struct {
    uint16_t attributes;         /* Mode attributes */
    uint8_t window_a;           /* Window A attributes */
    uint8_t window_b;           /* Window B attributes */
    uint16_t granularity;       /* Window granularity */
    uint16_t window_size;       /* Window size */
    uint16_t segment_a;         /* Window A segment */
    uint16_t segment_b;         /* Window B segment */
    uint32_t window_func;        /* Window function pointer */
    uint16_t pitch;             /* Bytes per scanline */
    uint16_t width;             /* Width in pixels */
    uint16_t height;            /* Height in pixels */
    uint8_t char_width;         /* Character width */
    uint8_t char_height;        /* Character height */
    uint8_t planes;             /* Number of planes */
    uint8_t bits_per_pixel;     /* Bits per pixel */
    uint8_t banks;              /* Number of banks */
    uint8_t memory_model;       /* Memory model type */
    uint8_t bank_size;          /* Bank size in KB */
    uint8_t image_pages;        /* Number of image pages */
    uint8_t reserved1;           /* Reserved */
    
    /* Direct Color Fields */
    uint8_t red_mask_size;      /* Red mask size */
    uint8_t red_field_pos;      /* Red field position */
    uint8_t green_mask_size;    /* Green mask size */
    uint8_t green_field_pos;    /* Green field position */
    uint8_t blue_mask_size;     /* Blue mask size */
    uint8_t blue_field_pos;     /* Blue field position */
    uint8_t rsvd_mask_size;     /* Reserved mask size */
    uint8_t rsvd_field_pos;      /* Reserved field position */
    uint8_t direct_color_info;   /* Direct color mode info */
    
    /* Framebuffer */
    uint32_t physbase;          /* Physical framebuffer base */
    uint32_t reserved2;         /* Reserved */
    uint16_t reserved3;         /* Reserved */
} __attribute__((packed)) vbe_mode_info_t;

/*============================================================================
 * FRAMEBUFFER DEVICE
 *============================================================================*/

typedef struct {
    uint32_t *framebuffer;      /* Linear framebuffer address */
    uint32_t width;            /* Screen width in pixels */
    uint32_t height;           /* Screen height in pixels */
    uint32_t pitch;             /* Bytes per line */
    uint32_t bpp;              /* Bits per pixel */
    uint32_t size;              /* Framebuffer size in bytes */
    bool initialized;           /* Whether FB is initialized */
    
    /* Color format */
    uint8_t red_pos;           /* Red field position */
    uint8_t red_size;          /* Red mask size */
    uint8_t green_pos;         /* Green field position */
    uint8_t green_size;        /* Green mask size */
    uint8_t blue_pos;          /* Blue field position */
    uint8_t blue_size;         /* Blue mask size */
} framebuffer_t;

/*============================================================================
 * COLOR DEFINITIONS
 *============================================================================*/

/* 8-bit VGA palette indices */
#define VGA_BLACK       0
#define VGA_BLUE        1
#define VGA_GREEN       2
#define VGA_CYAN        3
#define VGA_RED         4
#define VGA_MAGENTA     5
#define VGA_BROWN       6
#define VGA_LIGHT_GRAY  7
#define VGA_DARK_GRAY   8
#define VGA_LIGHT_BLUE  9
#define VGA_LIGHT_GREEN 10
#define VGA_LIGHT_CYAN  11
#define VGA_LIGHT_RED   12
#define VGA_LIGHT_MAGENTA 13
#define VGA_YELLOW      14
#define VGA_WHITE       15

/* 32-bit RGB colors (for 24/32 bpp modes) */
#define COLOR_BLACK     0x000000
#define COLOR_BLUE      0x0000AA
#define COLOR_GREEN     0x00AA00
#define COLOR_CYAN      0x00AAAA
#define COLOR_RED       0xAA0000
#define COLOR_MAGENTA   0xAA00AA
#define COLOR_BROWN     0xAA5500
#define COLOR_LIGHT_GRAY 0xAAAAAA
#define COLOR_DARK_GRAY 0x555555
#define COLOR_LIGHT_BLUE 0x5555FF
#define COLOR_LIGHT_GREEN 0x55FF55
#define COLOR_LIGHT_CYAN 0x55FFFF
#define COLOR_LIGHT_RED 0xFF5555
#define COLOR_LIGHT_MAGENTA 0xFF55FF
#define COLOR_YELLOW    0xFFFF55
#define COLOR_WHITE     0xFFFFFF

/*============================================================================
 * COORDINATE TYPES
 *============================================================================*/

typedef struct {
    int x, y;
} point_t;

typedef struct {
    int x, y;
    int width, height;
} rect_t;

/*============================================================================
 * BITMAP FONT (8x8)
 *============================================================================*/

#define FONT_WIDTH   8
#define FONT_HEIGHT  8

/* PSF2 font header */
typedef struct {
    uint32_t magic;         /* Magic number (0x864AB572) */
    uint32_t version;       /* Version */
    uint32_t header_size;  /* Size of header */
    uint32_t flags;         /* Flags */
    uint32_t num_glyphs;   /* Number of glyphs */
    uint32_t bytes_per_glyph; /* Bytes per glyph */
    uint32_t height;       /* Height of glyph */
    uint32_t width;        /* Width of glyph */
} psf2_header_t;

/* Built-in 8x8 font (PC BIOS font) */
extern const uint8_t font_8x8[256][8];

/*============================================================================
 * FRAMEBUFFER OPERATIONS
 *============================================================================*/

/* Initialize framebuffer */
int framebuffer_init(void);

/* Get framebuffer info */
framebuffer_t *framebuffer_get_info(void);

/* Clear framebuffer to a color */
void framebuffer_clear(uint32_t color);

/* Sync double buffer (if enabled) */
void framebuffer_sync(void);

/*============================================================================
 * PIXEL OPERATIONS
 *============================================================================*/

/* Set pixel at (x, y) to color */
void fb_putpixel(int x, int y, uint32_t color);

/* Get pixel at (x, y) */
uint32_t fb_getpixel(int x, int y);

/* Draw pixel with clipping */
void fb_putpixel_clip(int x, int y, uint32_t color, int clip_x, int clip_y, int clip_w, int clip_h);

/*============================================================================
 * 2D PRIMITIVES
 *============================================================================*/

/* Horizontal line */
void fb_draw_hline(int x1, int y, int width, uint32_t color);

/* Vertical line */
void fb_draw_vline(int x, int y1, int height, uint32_t color);

/* Line (Bresenham's algorithm) */
void fb_draw_line(int x1, int y1, int x2, int y2, uint32_t color);

/* Rectangle (outline) */
void fb_draw_rect(int x, int y, int width, int height, uint32_t color);

/* Filled rectangle */
void fb_draw_rect_fill(int x, int y, int width, int height, uint32_t color);

/* Circle (outline) */
void fb_draw_circle(int cx, int cy, int radius, uint32_t color);

/* Filled circle */
void fb_draw_circle_fill(int cx, int cy, int radius, uint32_t color);

/* Triangle (outline) */
void fb_draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color);

/* Filled triangle */
void fb_draw_triangle_fill(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color);

/* Ellipse (outline) */
void fb_draw_ellipse(int cx, int cy, int rx, int ry, uint32_t color);

/*============================================================================
 * BITMAP BLITTING
 *============================================================================*/

/* Copy rectangular region */
void fb_blit(uint32_t *src, int src_x, int src_y, int dst_x, int dst_y, int width, int height, int src_pitch);

/* Copy with color key (transparency) */
void fb_blit_color_key(uint32_t *src, int src_x, int src_y, int dst_x, int dst_y, 
                       int width, int height, int src_pitch, uint32_t key);

/* Scale bitmap */
void fb_stretch(uint32_t *src, int src_x, int src_y, int src_w, int src_h,
                int dst_x, int dst_y, int dst_w, int dst_h, int src_pitch);

/*============================================================================
 * FONT RENDERING
 *============================================================================*/

/* Draw character using 8x8 font */
void fb_draw_char(int x, int y, char c, uint32_t fg_color, uint32_t bg_color);

/* Draw string */
void fb_draw_string(int x, int y, const char *str, uint32_t fg_color, uint32_t bg_color);

/* Measure string width */
int fb_measure_string(const char *str);

/* Draw string centered */
void fb_draw_string_centered(int x, int y, int width, const char *str, 
                             uint32_t fg_color, uint32_t bg_color);

/*============================================================================
 * WINDOW/DIALOG PRIMITIVES
 *============================================================================*/

/* Draw button */
void fb_draw_button(int x, int y, int width, int height, const char *text,
                    uint32_t fg_color, uint32_t bg_color, uint32_t border_color);

/* Draw window frame */
void fb_draw_window(int x, int y, int width, int height, const char *title);

/* Draw progress bar */
void fb_draw_progress_bar(int x, int y, int width, int height, 
                          int progress, uint32_t fg_color, uint32_t bg_color);

/* Draw scrollbar */
void fb_draw_scrollbar(int x, int y, int height, int thumb_pos, int thumb_size);

/*============================================================================
 * DOUBLE BUFFERING
 *============================================================================*/

/* Enable/disable double buffering */
void fb_enable_double_buffer(void);
void fb_disable_double_buffer(void);
bool fb_double_buffer_enabled(void);

/* Get back buffer pointer */
uint32_t *fb_get_back_buffer(void);

/* Swap buffers */
void fb_swap_buffers(void);

/*============================================================================
 * SCREENSHOT
 *============================================================================*/

/* Save screenshot to buffer (caller provides buffer) */
void fb_screenshot(uint32_t *buffer, int width, int height);

/*============================================================================
 * FRAMEBUFFER STATISTICS
 *============================================================================*/

typedef struct {
    uint64_t pixels_drawn;
    uint64_t pixels_cleared;
    uint64_t pixels_blitted;
    uint64_t chars_rendered;
    uint32_t fps;
    uint32_t last_fps_update;
    uint32_t frames_this_sec;
} fb_stats_t;

fb_stats_t *fb_get_stats(void);
void fb_print_stats(void);

#endif /* FEATHEROS_FRAMEBUFFER_H */
