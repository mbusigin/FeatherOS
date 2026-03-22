/* FeatherOS - PS/2 Keyboard & Mouse Input
 * Sprint 15: PS/2 Keyboard & Mouse
 */

#ifndef FEATHEROS_KEYBOARD_H
#define FEATHEROS_KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

/*============================================================================
 * PS/2 CONTROLLER
 *============================================================================*/

/* PS/2 I/O Ports */
#define PS2_PORT_DATA      0x60
#define PS2_PORT_STATUS    0x64
#define PS2_PORT_COMMAND   0x64

/* PS/2 Status Register Bits */
#define PS2_STATUS_OUTBUF   0x01  /* Output buffer full */
#define PS2_STATUS_INPBUF   0x02  /* Input buffer full */
#define PS2_STATUS_SYSFLAG  0x04  /* System flag */
#define PS2_STATUS_CMD      0x08  /* Command/data */
#define PS2_STATUS_TIMEOUT  0x40  /* Transmission timeout */
#define PS2_STATUS_PARITY   0x80  /* Parity error */

/* PS/2 Commands */
#define PS2_CMD_READ_CFG    0x20
#define PS2_CMD_WRITE_CFG   0x60
#define PS2_CMD_DISABLE_P1  0xAD
#define PS2_CMD_ENABLE_P1   0xAE
#define PS2_CMD_DISABLE_P2  0xA7
#define PS2_CMD_ENABLE_P2   0xA8
#define PS2_CMD_TEST_P1    0xAB
#define PS2_CMD_TEST_P2    0xA9
#define PS2_CMD_TEST_CTLR   0xAA
#define PS2_CMD_SELFTEST    0xFE
#define PS2_CMD_CONTROLLER_TEST 0xAA

/* PS/2 Configuration Bits */
#define PS2_CFG_P1_INT     0x01  /* Port 1 interrupt enable */
#define PS2_CFG_P2_INT     0x02  /* Port 2 interrupt enable */
#define PS2_CFG_SYSFLAG    0x04  /* System flag */
#define PS2_CFG_P1_CLK     0x10  /* Port 1 clock enable */
#define PS2_CFG_P2_CLK     0x20  /* Port 2 clock enable */
#define PS2_CFG_P1_XLAT    0x40  /* Port 1 translation */

/* PS/2 Responses */
#define PS2_RESP_ACK       0xFA
#define PS2_RESP_RESEND    0xFE
#define PS2_RESP_ERROR     0xFF

/*============================================================================
 * KEYBOARD
 *============================================================================*/

/* Keyboard Commands */
#define KBD_CMD_SET_LEDS   0xED
#define KBD_CMD_ECHO       0xEE
#define KBD_CMD_SCANCODE   0xF0
#define KBD_CMD_TYPEMATIC  0xF3
#define KBD_CMD_ENABLE     0xF4
#define KBD_CMD_DISABLE    0xF5
#define KBD_CMD_DEFAULTS   0xF6
#define KBD_CMD_ALL_TYPEMATIC  0xF7
#define KBD_CMD_ALL_MAKE       0xF8
#define KBD_CMD_ALL_MAKE_REL   0xF9
#define KBD_CMD_SET_KEYTABLE    0xFB
#define KBD_CMD_SET_TABLES  0xFC
#define KBD_CMD_RESET   0xFF

/* Keyboard Responses */
#define KBD_RESP_ACK       0xFA
#define KBD_RESP_RESEND    0xFE
#define KBD_RESP_ERROR     0xFF
#define KBD_RESP_BAT_OK    0xAA
#define KBD_RESP_BAT_FAIL  0xFC
#define KBD_RESP_ECHO      0xEE

/* Special Key Codes */
#define KBD_KEY_BREAK      0x80  /* Break code prefix */
#define KBD_KEY_EXTENDED   0xE0  /* Extended code prefix */
#define KBD_KEY_PAUSE      0xE1  /* Pause key prefix */

/* Modifier Keys */
#define KBD_MOD_LSHIFT     0x01
#define KBD_MOD_RSHIFT     0x02
#define KBD_MOD_LCTRL      0x04
#define KBD_MOD_RCTRL      0x08
#define KBD_MOD_LALT       0x10
#define KBD_MOD_RALT       0x20
#define KBD_MOD_LGUI       0x40
#define KBD_MOD_RGUI       0x80
#define KBD_MOD_SHIFT      (KBD_MOD_LSHIFT | KBD_MOD_RSHIFT)
#define KBD_MOD_CTRL       (KBD_MOD_LCTRL | KBD_MOD_RCTRL)
#define KBD_MOD_ALT        (KBD_MOD_LALT | KBD_MOD_RALT)
#define KBD_MOD_GUI        (KBD_MOD_LGUI | KBD_MOD_RGUI)

/* Keyboard LED Flags */
#define KBD_LED_SCROLL     0x01
#define KBD_LED_NUM        0x02
#define KBD_LED_CAPS       0x04

/*============================================================================
 * SCANCODE SET 1 (XT-compatible)
 *============================================================================*/

/* US Layout - Scan Code Set 1 */
enum {
    KBD_SCAN_ESC       = 0x01,
    KBD_SCAN_1         = 0x02,
    KBD_SCAN_2         = 0x03,
    KBD_SCAN_3         = 0x04,
    KBD_SCAN_4         = 0x05,
    KBD_SCAN_5         = 0x06,
    KBD_SCAN_6         = 0x07,
    KBD_SCAN_7         = 0x08,
    KBD_SCAN_8         = 0x09,
    KBD_SCAN_9         = 0x0A,
    KBD_SCAN_0         = 0x0B,
    KBD_SCAN_MINUS     = 0x0C,
    KBD_SCAN_EQUAL     = 0x0D,
    KBD_SCAN_BACKSPACE = 0x0E,
    KBD_SCAN_TAB       = 0x0F,
    KBD_SCAN_Q         = 0x10,
    KBD_SCAN_W         = 0x11,
    KBD_SCAN_E         = 0x12,
    KBD_SCAN_R         = 0x13,
    KBD_SCAN_T         = 0x14,
    KBD_SCAN_Y         = 0x15,
    KBD_SCAN_U         = 0x16,
    KBD_SCAN_I         = 0x17,
    KBD_SCAN_O         = 0x18,
    KBD_SCAN_P         = 0x19,
    KBD_SCAN_LBRACKET  = 0x1A,
    KBD_SCAN_RBRACKET  = 0x1B,
    KBD_SCAN_ENTER     = 0x1C,
    KBD_SCAN_LCTRL     = 0x1D,
    KBD_SCAN_A         = 0x1E,
    KBD_SCAN_S         = 0x1F,
    KBD_SCAN_D         = 0x20,
    KBD_SCAN_F         = 0x21,
    KBD_SCAN_G         = 0x22,
    KBD_SCAN_H         = 0x23,
    KBD_SCAN_J         = 0x24,
    KBD_SCAN_K         = 0x25,
    KBD_SCAN_L         = 0x26,
    KBD_SCAN_SEMICOLON = 0x27,
    KBD_SCAN_QUOTE     = 0x28,
    KBD_SCAN_BACKQUOTE = 0x29,
    KBD_SCAN_LSHIFT    = 0x2A,
    KBD_SCAN_BACKSLASH = 0x2B,
    KBD_SCAN_Z         = 0x2C,
    KBD_SCAN_X         = 0x2D,
    KBD_SCAN_C         = 0x2E,
    KBD_SCAN_V         = 0x2F,
    KBD_SCAN_B         = 0x30,
    KBD_SCAN_N         = 0x31,
    KBD_SCAN_M         = 0x32,
    KBD_SCAN_COMMA     = 0x33,
    KBD_SCAN_PERIOD    = 0x34,
    KBD_SCAN_SLASH     = 0x35,
    KBD_SCAN_RSHIFT    = 0x36,
    KBD_SCAN_KP_MULT   = 0x37,
    KBD_SCAN_LALT      = 0x38,
    KBD_SCAN_SPACE     = 0x39,
    KBD_SCAN_CAPSLOCK  = 0x3A,
    KBD_SCAN_F1        = 0x3B,
    KBD_SCAN_F2        = 0x3C,
    KBD_SCAN_F3        = 0x3D,
    KBD_SCAN_F4        = 0x3E,
    KBD_SCAN_F5        = 0x3F,
    KBD_SCAN_F6        = 0x40,
    KBD_SCAN_F7        = 0x41,
    KBD_SCAN_F8        = 0x42,
    KBD_SCAN_F9        = 0x43,
    KBD_SCAN_F10       = 0x44,
    KBD_SCAN_NUMLOCK   = 0x45,
    KBD_SCAN_SCROLLLOCK= 0x46,
    KBD_SCAN_KP_7      = 0x47,
    KBD_SCAN_KP_8      = 0x48,
    KBD_SCAN_KP_9      = 0x49,
    KBD_SCAN_KP_MINUS  = 0x4A,
    KBD_SCAN_KP_4      = 0x4B,
    KBD_SCAN_KP_5      = 0x4C,
    KBD_SCAN_KP_6      = 0x4D,
    KBD_SCAN_KP_PLUS   = 0x4E,
    KBD_SCAN_KP_1      = 0x4F,
    KBD_SCAN_KP_2      = 0x50,
    KBD_SCAN_KP_3      = 0x51,
    KBD_SCAN_KP_0      = 0x52,
    KBD_SCAN_KP_DELETE = 0x53,
    KBD_SCAN_F11       = 0x57,
    KBD_SCAN_F12       = 0x58,
};

/*============================================================================
 * MOUSE
 *============================================================================*/

/* Mouse Commands */
#define MOUSE_CMD_SETDEFAULTS    0xF6
#define MOUSE_CMD_DISABLE        0xF5
#define MOUSE_CMD_ENABLE         0xF4
#define MOUSE_CMD_SET_SAMPLE     0xF3
#define MOUSE_CMD_GET_DEV_ID     0xF2
#define MOUSE_CMD_SET_REMOTE     0xF0
#define MOUSE_CMD_SET_WRAP       0xEE
#define MOUSE_CMD_RESET_WRAP     0xEC
#define MOUSE_CMD_READ_DATA      0xEB
#define MOUSE_CMD_SET_STREAM     0xEA
#define MOUSE_CMD_STATUS_REQUEST 0xE9
#define MOUSE_CMD_SET_RESOLUTION 0xE8
#define MOUSE_CMD_SET_SCALING_1  0xE7
#define MOUSE_CMD_SET_SCALING_2  0xE6

/* Mouse Responses */
#define MOUSE_RESP_ACK           0xFA
#define MOUSE_RESP_NACK          0xFE
#define MOUSE_RESP_ERROR         0xFC

/* Mouse Device IDs */
#define MOUSE_ID_STANDARD        0x00
#define MOUSE_ID_WHEEL           0x03
#define MOUSE_ID_5BUTTON         0x04

/* Mouse Packet Byte 1 Flags */
#define MOUSE_BTN_LEFT           0x01
#define MOUSE_BTN_RIGHT          0x02
#define MOUSE_BTN_MIDDLE         0x04
#define MOUSE_BTN_4              0x08
#define MOUSE_BTN_5              0x10
#define MOUSE_X_SIGN             0x10
#define MOUSE_Y_SIGN             0x20
#define MOUSE_X_OVERFLOW         0x40
#define MOUSE_Y_OVERFLOW         0x80

/*============================================================================
 * INPUT EVENTS
 *============================================================================*/

/* Key event structure */
typedef struct {
    uint8_t keycode;     /* Hardware scancode */
    uint8_t ascii;       /* ASCII character */
    uint8_t modifiers;   /* Modifier keys state */
    bool pressed;        /* true = pressed, false = released */
} key_event_t;

/* Mouse event structure */
typedef struct {
    int8_t dx;          /* X movement delta */
    int8_t dy;          /* Y movement delta */
    int8_t dz;           /* Z (wheel) movement delta */
    uint8_t buttons;     /* Button states */
    bool moved;         /* Movement occurred */
    bool clicked;       /* Click occurred */
} mouse_event_t;

/* Input buffer */
#define KEYBOARD_BUFFER_SIZE  64
#define MOUSE_BUFFER_SIZE     32

typedef struct {
    uint8_t buffer[KEYBOARD_BUFFER_SIZE];
    volatile int head;
    volatile int tail;
} keyboard_buffer_t;

typedef struct {
    mouse_event_t buffer[MOUSE_BUFFER_SIZE];
    volatile int head;
    volatile int tail;
} mouse_buffer_t;

/*============================================================================
 * INPUT INITIALIZATION
 *============================================================================*/

/* Initialize PS/2 controller */
int ps2_init(void);

/* Initialize keyboard */
int keyboard_init(void);

/* Initialize mouse */
int mouse_init(void);

/* Initialize all input devices */
int input_init(void);

/*============================================================================
 * KEYBOARD FUNCTIONS
 *============================================================================*/

/* Get key from keyboard buffer (blocking) */
int keyboard_getchar(void);

/* Get key from keyboard buffer (non-blocking) */
int keyboard_getchar_nb(void);

/* Check if keyboard has pending input */
bool keyboard_has_input(void);

/* Get current modifier state */
uint8_t keyboard_get_modifiers(void);

/* Set keyboard LEDs */
void keyboard_set_leds(uint8_t leds);

/* Clear keyboard buffer */
void keyboard_clear_buffer(void);

/*============================================================================
 * MOUSE FUNCTIONS
 *============================================================================*/

/* Get mouse event from buffer (non-blocking) */
bool mouse_get_event(mouse_event_t *event);

/* Check if mouse has pending events */
bool mouse_has_events(void);

/* Clear mouse buffer */
void mouse_clear_buffer(void);

/* Enable mouse */
void mouse_enable(void);

/* Disable mouse */
void mouse_disable(void);

/*============================================================================
 * CURSOR (Text Mode)
 *============================================================================*/

/* Cursor position */
typedef struct {
    int x;
    int y;
} cursor_pos_t;

/* Cursor visibility */
void cursor_show(void);
void cursor_hide(void);
void cursor_set_position(int x, int y);
cursor_pos_t cursor_get_position(void);

/*============================================================================
 * INPUT STATISTICS
 *============================================================================*/

typedef struct {
    uint64_t keys_pressed;
    uint64_t keys_released;
    uint64_t mouse_events;
    uint64_t mouse_moves;
    uint64_t mouse_clicks;
    uint64_t buffer_overflows;
} input_stats_t;

input_stats_t *get_input_stats(void);
void print_input_stats(void);

#endif /* FEATHEROS_KEYBOARD_H */
