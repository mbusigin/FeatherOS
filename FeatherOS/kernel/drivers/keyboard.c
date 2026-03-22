/* FeatherOS - PS/2 Keyboard & Mouse Implementation
 * Sprint 15: PS/2 Keyboard & Mouse
 */

#include <keyboard.h>
#include <kernel.h>

/* External assembly functions */
extern void outb(uint16_t port, uint8_t val);
extern uint8_t inb(uint16_t port);

/*============================================================================
 * SCANCODE TO ASCII TRANSLATION TABLES
 *============================================================================*/

/* US QWERTY layout - normal keys */
static const uint8_t kbd_us_normal[] = {
    0,      KBD_SCAN_ESC,       '1',    '2',    '3',    '4',    '5',    '6',
    '7',    '8',    '9',    '0',    '-',    '=',    0x08,   '\t',
    'q',    'w',    'e',    'r',    't',    'y',    'u',    'i',
    'o',    'p',    '[',    ']',    '\n',   KBD_SCAN_LCTRL, 'a',    's',
    'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';',
    '\'',   '`',    KBD_SCAN_LSHIFT, '\\',   'z',    'x',    'c',    'v',
    'b',    'n',    'm',    ',',    '.',    '/',    KBD_SCAN_RSHIFT, '*',
    KBD_SCAN_LALT,  ' ',    KBD_SCAN_CAPSLOCK, KBD_SCAN_F1,    KBD_SCAN_F2,
    KBD_SCAN_F3,    KBD_SCAN_F4,    KBD_SCAN_F5,    KBD_SCAN_F6,
    KBD_SCAN_F7,    KBD_SCAN_F8,    KBD_SCAN_F9,    KBD_SCAN_F10,
    KBD_SCAN_NUMLOCK, KBD_SCAN_SCROLLLOCK, '7',    '8',    '9',    '-',
    '4',    '5',    '6',    '+',    '1',    '2',    '3',    '0',
    '.',    0,      0,      KBD_SCAN_F11,   KBD_SCAN_F12
};

/* US QWERTY layout - shifted keys */
static const uint8_t kbd_us_shift[] = {
    0,      KBD_SCAN_ESC,       '!',    '@',    '#',    '$',    '%',    '^',
    '&',    '*',    '(',    ')',    '_',    '+',    0x08,   '\t',
    'Q',    'W',    'E',    'R',    'T',    'Y',    'U',    'I',
    'O',    'P',    '{',    '}',    '\n',   KBD_SCAN_LCTRL, 'A',    'S',
    'D',    'F',    'G',    'H',    'J',    'K',    'L',    ':',
    '"',    '~',    KBD_SCAN_LSHIFT, '|',    'Z',    'X',    'C',    'V',
    'B',    'N',    'M',    '<',    '>',    '?',    KBD_SCAN_RSHIFT, '*',
    KBD_SCAN_LALT,  ' ',    KBD_SCAN_CAPSLOCK, KBD_SCAN_F1,    KBD_SCAN_F2,
    KBD_SCAN_F3,    KBD_SCAN_F4,    KBD_SCAN_F5,    KBD_SCAN_F6,
    KBD_SCAN_F7,    KBD_SCAN_F8,    KBD_SCAN_F9,    KBD_SCAN_F10,
    KBD_SCAN_NUMLOCK, KBD_SCAN_SCROLLLOCK, '7',    '8',    '9',    '-',
    '4',    '5',    '6',    '+',    '1',    '2',    '3',    '0',
    '.',    0,      0,      KBD_SCAN_F11,   KBD_SCAN_F12
};

/*============================================================================
 * PS/2 CONTROLLER STATE
 *============================================================================*/

static bool ps2_initialized = false;
static bool keyboard_initialized = false;
static bool mouse_initialized = false;

/* Input buffers */
static keyboard_buffer_t kbd_buffer = { .head = 0, .tail = 0 };
static mouse_buffer_t mouse_buffer = { .head = 0, .tail = 0 };

/* Modifier state */
static volatile uint8_t kbd_modifiers = 0;
static volatile bool kbd_capslock = false;
static volatile bool kbd_numlock = false;
static volatile bool kbd_scrolllock = false;
static volatile bool kbd_extended = false;

/* Mouse state */
static volatile uint8_t mouse_packet[3] = {0};
static volatile int mouse_packet_idx = 0;
static volatile bool mouse_has_wheel = false;

/* Input statistics */
static input_stats_t input_stats = {0};

/*============================================================================
 * PS/2 HELPER FUNCTIONS
 *============================================================================*/

static void ps2_wait_input(void) {
    for (int i = 0; i < 10000; i++) {
        if (!(inb(PS2_PORT_STATUS) & PS2_STATUS_INPBUF)) {
            return;
        }
    }
}

static void ps2_wait_output(void) {
    for (int i = 0; i < 10000; i++) {
        if (inb(PS2_PORT_STATUS) & PS2_STATUS_OUTBUF) {
            return;
        }
    }
}

static uint8_t ps2_read_data(void) {
    ps2_wait_output();
    return inb(PS2_PORT_DATA);
}

static void ps2_write_command(uint8_t cmd) {
    ps2_wait_input();
    outb(PS2_PORT_COMMAND, cmd);
}

static void ps2_write_data(uint8_t data) {
    ps2_wait_input();
    outb(PS2_PORT_DATA, data);
}

/*============================================================================
 * PS/2 CONTROLLER INITIALIZATION
 *============================================================================*/

int ps2_init(void) {
    if (ps2_initialized) {
        return 0;
    }
    
    /* Disable devices during configuration */
    ps2_write_command(PS2_CMD_DISABLE_P1);
    ps2_wait_input();
    ps2_write_command(PS2_CMD_DISABLE_P2);
    ps2_wait_input();
    
    /* Flush output buffer */
    inb(PS2_PORT_DATA);
    
    /* Set configuration */
    ps2_write_command(PS2_CMD_READ_CFG);
    uint8_t cfg = ps2_read_data();
    
    /* Disable IRQs and clock for both ports initially */
    cfg &= ~(PS2_CFG_P1_INT | PS2_CFG_P2_INT);
    cfg |= PS2_CFG_P1_CLK | PS2_CFG_P2_CLK;
    
    ps2_write_command(PS2_CMD_WRITE_CFG);
    ps2_write_data(cfg);
    
    /* Perform self-test */
    ps2_write_command(PS2_CMD_TEST_CTLR);
    uint8_t test = ps2_read_data();
    
    if (test != 0x55) {
        console_print("PS/2 controller self-test failed: 0x%02x\n", test);
        return -1;
    }
    
    /* Reconfigure */
    ps2_write_command(PS2_CMD_WRITE_CFG);
    cfg = ps2_read_data();
    
    /* Check for port 2 */
    if (cfg & 0x20) {
        console_print("PS/2 port 2 present (mouse capable)\n");
    }
    
    ps2_initialized = true;
    console_print("PS/2 controller initialized\n");
    
    return 0;
}

/*============================================================================
 * KEYBOARD FUNCTIONS
 *============================================================================*/

int keyboard_init(void) {
    if (!ps2_initialized) {
        ps2_init();
    }
    
    /* Enable port 1 interrupt */
    ps2_write_command(PS2_CMD_READ_CFG);
    uint8_t cfg = ps2_read_data();
    cfg |= PS2_CFG_P1_INT | PS2_CFG_P1_CLK;
    ps2_write_command(PS2_CMD_WRITE_CFG);
    ps2_write_data(cfg);
    
    /* Enable keyboard */
    ps2_write_data(KBD_CMD_ENABLE);
    uint8_t resp = ps2_read_data();
    
    if (resp != KBD_RESP_ACK) {
        console_print("Keyboard enable failed: 0x%02x\n", resp);
        return -1;
    }
    
    keyboard_initialized = true;
    console_print("PS/2 keyboard initialized\n");
    
    return 0;
}

/* Convert scancode to ASCII */
static uint8_t scancode_to_ascii(uint8_t scancode) {
    bool shift = kbd_modifiers & KBD_MOD_SHIFT;
    (void)kbd_modifiers;  /* ctrl modifier available for future use */
    
    /* Handle caps lock */
    if (kbd_capslock && !(kbd_modifiers & KBD_MOD_SHIFT)) {
        if (scancode >= KBD_SCAN_A && scancode <= KBD_SCAN_Z) {
            shift = true;
        }
    }
    
    if (scancode < sizeof(kbd_us_normal)) {
        if (shift) {
            return kbd_us_shift[scancode];
        }
        return kbd_us_normal[scancode];
    }
    
    return 0;
}

/* Keyboard interrupt handler */
void keyboard_interrupt_handler(void) {
    uint8_t scancode = inb(PS2_PORT_DATA);
    
    input_stats.keys_pressed++;
    
    /* Handle extended scancodes */
    if (scancode == KBD_KEY_EXTENDED) {
        kbd_extended = true;
        return;
    }
    
    /* Check for break code */
    bool pressed = !(scancode & KBD_KEY_BREAK);
    uint8_t key = scancode & ~KBD_KEY_BREAK;
    
    if (!pressed) {
        input_stats.keys_released++;
    }
    
    /* Update modifiers */
    switch (key) {
        case KBD_SCAN_LSHIFT:
            if (pressed) kbd_modifiers |= KBD_MOD_LSHIFT;
            else kbd_modifiers &= ~KBD_MOD_LSHIFT;
            return;
        case KBD_SCAN_RSHIFT:
            if (pressed) kbd_modifiers |= KBD_MOD_RSHIFT;
            else kbd_modifiers &= ~KBD_MOD_RSHIFT;
            return;
        case KBD_SCAN_LCTRL:
            if (pressed) kbd_modifiers |= KBD_MOD_LCTRL;
            else kbd_modifiers &= ~KBD_MOD_LCTRL;
            return;
        case KBD_SCAN_A:
            if (pressed) {
                kbd_capslock = !kbd_capslock;
                keyboard_set_leds((kbd_scrolllock ? KBD_LED_SCROLL : 0) |
                                (kbd_numlock ? KBD_LED_NUM : 0) |
                                (kbd_capslock ? KBD_LED_CAPS : 0));
            }
            return;
        case KBD_SCAN_NUMLOCK:
            if (pressed) {
                kbd_numlock = !kbd_numlock;
                keyboard_set_leds((kbd_scrolllock ? KBD_LED_SCROLL : 0) |
                                (kbd_numlock ? KBD_LED_NUM : 0) |
                                (kbd_capslock ? KBD_LED_CAPS : 0));
            }
            return;
        case KBD_SCAN_SCROLLLOCK:
            if (pressed) {
                kbd_scrolllock = !kbd_scrolllock;
                keyboard_set_leds((kbd_scrolllock ? KBD_LED_SCROLL : 0) |
                                (kbd_numlock ? KBD_LED_NUM : 0) |
                                (kbd_capslock ? KBD_LED_CAPS : 0));
            }
            return;
    }
    
    /* Add to buffer if pressed and buffer not full */
    if (pressed) {
        int next_head = (kbd_buffer.head + 1) % KEYBOARD_BUFFER_SIZE;
        if (next_head != kbd_buffer.tail) {
            kbd_buffer.buffer[kbd_buffer.head] = scancode_to_ascii(key);
            kbd_buffer.head = next_head;
        } else {
            input_stats.buffer_overflows++;
        }
    }
    
    kbd_extended = false;
}

int keyboard_getchar(void) {
    /* Wait for character */
    while (kbd_buffer.head == kbd_buffer.tail) {
        /* Could yield or sleep here */
    }
    
    uint8_t ch = kbd_buffer.buffer[kbd_buffer.tail];
    kbd_buffer.tail = (kbd_buffer.tail + 1) % KEYBOARD_BUFFER_SIZE;
    
    return ch;
}

int keyboard_getchar_nb(void) {
    if (kbd_buffer.head == kbd_buffer.tail) {
        return -1;
    }
    
    uint8_t ch = kbd_buffer.buffer[kbd_buffer.tail];
    kbd_buffer.tail = (kbd_buffer.tail + 1) % KEYBOARD_BUFFER_SIZE;
    
    return ch;
}

bool keyboard_has_input(void) {
    return kbd_buffer.head != kbd_buffer.tail;
}

bool keyboard_has_key(void) {
    return keyboard_has_input();
}

uint8_t keyboard_get_modifiers(void) {
    return kbd_modifiers;
}

void keyboard_set_leds(uint8_t leds) {
    ps2_write_data(KBD_CMD_SET_LEDS);
    ps2_read_data();  /* ACK */
    ps2_write_data(leds);
    ps2_read_data();  /* ACK */
}

void keyboard_clear_buffer(void) {
    kbd_buffer.head = 0;
    kbd_buffer.tail = 0;
}

/*============================================================================
 * MOUSE FUNCTIONS
 *============================================================================*/

int mouse_init(void) {
    if (!ps2_initialized) {
        ps2_init();
    }
    
    /* Enable port 2 (mouse) */
    ps2_write_command(PS2_CMD_READ_CFG);
    uint8_t cfg = ps2_read_data();
    cfg |= PS2_CFG_P2_INT | PS2_CFG_P2_CLK;
    ps2_write_command(PS2_CMD_WRITE_CFG);
    ps2_write_data(cfg);
    
    /* Enable auxiliary device */
    ps2_write_command(PS2_CMD_ENABLE_P2);
    
    /* Initialize mouse */
    ps2_write_data(MOUSE_CMD_ENABLE);
    uint8_t resp = ps2_read_data();
    
    if (resp != MOUSE_RESP_ACK) {
        console_print("Mouse enable failed: 0x%02x\n", resp);
        return -1;
    }
    
    /* Get device ID */
    ps2_write_data(MOUSE_CMD_GET_DEV_ID);
    ps2_read_data();  /* ACK */
    uint8_t dev_id = ps2_read_data();
    
    mouse_has_wheel = (dev_id == MOUSE_ID_WHEEL);
    console_print("PS/2 mouse initialized (ID: 0x%02x, wheel: %s)\n",
                  dev_id, mouse_has_wheel ? "yes" : "no");
    
    mouse_initialized = true;
    return 0;
}

/* Mouse interrupt handler */
void mouse_interrupt_handler(void) {
    uint8_t status = inb(PS2_PORT_STATUS);
    if (!(status & PS2_STATUS_OUTBUF)) {
        return;
    }
    
    uint8_t data = inb(PS2_PORT_DATA);
    
    input_stats.mouse_events++;
    
    /* Parse mouse packet */
    if (!(data & 0x08)) {
        /* First byte of packet, but bit 3 not set - out of sync */
        mouse_packet_idx = 0;
        return;
    }
    
    mouse_packet[mouse_packet_idx++] = data;
    
    if (mouse_packet_idx < (mouse_has_wheel ? 4 : 3)) {
        return;  /* Wait for complete packet */
    }
    
    mouse_packet_idx = 0;
    
    /* Parse packet */
    mouse_event_t event = {0};
    
    event.buttons = mouse_packet[0] & 0x07;
    event.moved = true;
    event.clicked = (mouse_packet[0] & 0x07) != 0;
    
    /* X movement */
    if (mouse_packet[0] & MOUSE_X_SIGN) {
        event.dx = (int8_t)mouse_packet[1] - 256;
    } else {
        event.dx = (int8_t)mouse_packet[1];
    }
    
    /* Y movement (inverted) */
    if (mouse_packet[0] & MOUSE_Y_SIGN) {
        event.dy = 256 - (int8_t)mouse_packet[2];
    } else {
        event.dy = -(int8_t)mouse_packet[2];
    }
    
    /* Wheel */
    if (mouse_has_wheel) {
        event.dz = (int8_t)mouse_packet[3];
    }
    
    if (event.dx != 0 || event.dy != 0) {
        input_stats.mouse_moves++;
    }
    if (event.clicked) {
        input_stats.mouse_clicks++;
    }
    
    /* Add to buffer */
    int next_head = (mouse_buffer.head + 1) % MOUSE_BUFFER_SIZE;
    if (next_head != mouse_buffer.tail) {
        mouse_buffer.buffer[mouse_buffer.head] = event;
        mouse_buffer.head = next_head;
    } else {
        input_stats.buffer_overflows++;
    }
}

bool mouse_get_event(mouse_event_t *event) {
    if (mouse_buffer.head == mouse_buffer.tail) {
        return false;
    }
    
    *event = mouse_buffer.buffer[mouse_buffer.tail];
    mouse_buffer.tail = (mouse_buffer.tail + 1) % MOUSE_BUFFER_SIZE;
    
    return true;
}

bool mouse_has_events(void) {
    return mouse_buffer.head != mouse_buffer.tail;
}

void mouse_clear_buffer(void) {
    mouse_buffer.head = 0;
    mouse_buffer.tail = 0;
}

void mouse_enable(void) {
    ps2_write_data(MOUSE_CMD_ENABLE);
}

void mouse_disable(void) {
    ps2_write_data(MOUSE_CMD_DISABLE);
}

/*============================================================================
 * CURSOR FUNCTIONS
 *============================================================================*/

/* VGA cursor functions would be in VGA driver, but we provide stubs here */
void cursor_show(void) {
    /* Would access VGA CRT controller */
}

void cursor_hide(void) {
    /* Would access VGA CRT controller */
}

void cursor_set_position(int x, int y) {
    (void)x;
    (void)y;
}

cursor_pos_t cursor_get_position(void) {
    cursor_pos_t pos = { .x = 0, .y = 0 };
    return pos;
}

/*============================================================================
 * INPUT INITIALIZATION
 *============================================================================*/

int input_init(void) {
    console_print("Initializing input subsystem...\n");
    
    /* Initialize PS/2 controller */
    if (ps2_init() != 0) {
        console_print("PS/2 controller init failed\n");
        return -1;
    }
    
    /* Initialize keyboard */
    if (keyboard_init() != 0) {
        console_print("Keyboard init failed\n");
        return -1;
    }
    
    /* Initialize mouse */
    if (mouse_init() != 0) {
        console_print("Mouse init failed (non-fatal)\n");
    }
    
    console_print("Input subsystem initialized\n");
    return 0;
}

/*============================================================================
 * STATISTICS
 *============================================================================*/

input_stats_t *get_input_stats(void) {
    return &input_stats;
}

void print_input_stats(void) {
    console_print("\n=== Input Statistics ===\n");
    console_print("Keys pressed: %lu\n", input_stats.keys_pressed);
    console_print("Keys released: %lu\n", input_stats.keys_released);
    console_print("Mouse events: %lu\n", input_stats.mouse_events);
    console_print("Mouse moves: %lu\n", input_stats.mouse_moves);
    console_print("Mouse clicks: %lu\n", input_stats.mouse_clicks);
    console_print("Buffer overflows: %lu\n", input_stats.buffer_overflows);
}
