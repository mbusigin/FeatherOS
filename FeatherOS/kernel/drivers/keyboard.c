/* FeatherOS - PS/2 Keyboard Driver
 * Sprint 3: Console & Basic I/O
 */

#include <kernel.h>

/* PS/2 Controller I/O ports */
#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64
#define PS2_COMMAND_PORT 0x64

/* PS/2 Status Register bits */
#define PS2_STATUS_OUT_BUF   0x01  /* Output buffer full */
#define PS2_STATUS_IN_BUF    0x02  /* Input buffer full */
#define PS2_STATUS_SYSTEM    0x04  /* System flag */
#define PS2_STATUS_CMD_DATA  0x08  /* 0 = data, 1 = command */
#define PS2_STATUS_TIMEOUT   0x40  /* Timeout error */
#define PS2_STATUS_PARITY   0x80  /* Parity error */

/* PS/2 Controller Commands */
#define PS2_CMD_READ_CFG    0x20
#define PS2_CMD_WRITE_CFG   0x60
#define PS2_CMD_DISABLE_KB  0xAD
#define PS2_CMD_ENABLE_KB   0xAE
#define PS2_CMD_DISABLE_MOUSE 0xA7
#define PS2_CMD_ENABLE_MOUSE  0xA8
#define PS2_CMD_TEST        0xAA
#define PS2_CMD_KB_TEST      0xAB
#define PS2_CMD_RESET_KB    0xFF

/* Keyboard Scancode Set 1 - US Layout */
static const char scancode_map[128] = {
    /* 0x00 */ 0,    27,   '1',  '2',  '3',  '4',  '5',  '6',
    /* 0x08 */ '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
    /* 0x10 */ 'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',
    /* 0x18 */ 'o',  'p',  '[',  ']',  '\n', 0,    'a',  's',
    /* 0x20 */ 'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',
    /* 0x28 */ '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',
    /* 0x30 */ 'b',  'n',  'm',  ',',  '.',  '/',  0,    '*',
    /* 0x38 */ 0,    ' ',  0,    0,    0,    0,    0,    0,
};

/* Shifted characters */
static const char scancode_shift[128] = {
    /* 0x00 */ 0,    27,   '!',  '@',  '#',  '$',  '%',  '^',
    /* 0x08 */ '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
    /* 0x10 */ 'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
    /* 0x18 */ 'O',  'P',  '{',  '}',  '\n', 0,    'A',  'S',
    /* 0x20 */ 'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',
    /* 0x28 */ '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',
    /* 0x30 */ 'B',  'N',  'M',  '<',  '>',  '?',  0,    '*',
    /* 0x38 */ 0,    ' ',  0,    0,    0,    0,    0,    0,
};

/* Keyboard state */
static bool shift_pressed = false;
static bool caps_lock = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;

/* Input buffer */
#define KEYBOARD_BUFFER_SIZE 32
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile uint8_t keyboard_head = 0;
static volatile uint8_t keyboard_tail = 0;

/* Add character to keyboard buffer */
static void keyboard_buffer_put(char c) {
    uint8_t next = (keyboard_head + 1) % KEYBOARD_BUFFER_SIZE;
    if (next != keyboard_tail) {
        keyboard_buffer[keyboard_head] = c;
        keyboard_head = next;
    }
}

/* Get character from keyboard buffer */
static char keyboard_buffer_get(void) {
    if (keyboard_tail == keyboard_head) {
        return 0;
    }
    char c = keyboard_buffer[keyboard_tail];
    keyboard_tail = (keyboard_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

/* Check if keyboard buffer has data */
static bool keyboard_buffer_has_data(void) {
    return keyboard_tail != keyboard_head;
}

/* Read a raw scancode from keyboard */
char keyboard_read_scancode(void) {
    while (!(inb(PS2_STATUS_PORT) & PS2_STATUS_OUT_BUF)) {
        __asm__ volatile("pause");
    }
    return inb(PS2_DATA_PORT);
}

/* Handle a scancode and return ASCII character */
static char handle_scancode(uint8_t scancode) {
    bool release = false;
    
    if (scancode & 0x80) {
        release = true;
        scancode &= 0x7F;
    }
    
    switch (scancode) {
        case 0x2A:  /* Left Shift */
        case 0x36:  /* Right Shift */
            shift_pressed = !release;
            return 0;
            
        case 0x1D:  /* Left Ctrl */
            ctrl_pressed = !release;
            return 0;
            
        case 0x38:  /* Left Alt */
            alt_pressed = !release;
            return 0;
            
        case 0x3A:  /* Caps Lock */
            if (!release) {
                caps_lock = !caps_lock;
            }
            return 0;
            
        default:
            break;
    }
    
    if (release) {
        return 0;  /* Ignore key releases except special keys */
    }
    
    if (scancode >= 128) {
        return 0;
    }
    
    char c = scancode_map[scancode];
    
    /* Apply shift and caps lock */
    if (c >= 'a' && c <= 'z') {
        if (caps_lock ^ shift_pressed) {
            c = c - 'a' + 'A';
        }
    } else if (shift_pressed) {
        c = scancode_shift[scancode];
    }
    
    return c;
}

/* Poll for keyboard input */
void keyboard_poll(void) {
    if (inb(PS2_STATUS_PORT) & PS2_STATUS_OUT_BUF) {
        uint8_t scancode = inb(PS2_DATA_PORT);
        char c = handle_scancode(scancode);
        if (c) {
            keyboard_buffer_put(c);
        }
    }
}

/* Get a character from keyboard (non-blocking) */
char keyboard_getchar(void) {
    /* First poll for new input */
    keyboard_poll();
    
    /* Then return from buffer */
    if (keyboard_buffer_has_data()) {
        return keyboard_buffer_get();
    }
    
    /* If nothing in buffer, wait */
    while (!keyboard_buffer_has_data()) {
        keyboard_poll();
    }
    
    return keyboard_buffer_get();
}

/* Check if keyboard has input */
bool keyboard_has_key(void) {
    keyboard_poll();
    return keyboard_buffer_has_data();
}

/* Convert scancode to readable string */
const char *keyboard_scancode_to_string(uint8_t scancode) {
    static const char *names[] = {
        "ERROR", "Esc", "1", "2", "3", "4", "5", "6",
        "7", "8", "9", "0", "-", "=", "Backspace", "Tab",
        "Q", "W", "E", "R", "T", "Y", "U", "I",
        "O", "P", "[", "]", "Enter", "LCtrl", "A", "S",
        "D", "F", "G", "H", "J", "K", "L", ";",
        "'", "`", "LShift", "\\", "Z", "X", "C", "V",
        "B", "N", "M", ",", ".", "/", "RShift", "Keypad *",
        "LAlt", "Space", "CapsLock", "F1", "F2", "F3", "F4", "F5",
        "F6", "F7", "F8", "F9", "F10", "NumLock", "ScrollLock", "Keypad 7",
        "Keypad 8", "Keypad 9", "Keypad -", "Keypad 4", "Keypad 5", "Keypad 6", "Keypad +", "Keypad 1",
        "Keypad 2", "Keypad 3", "Keypad 0", "Keypad .", NULL
    };
    
    if (scancode < 0x3C) {
        return names[scancode];
    }
    return "Unknown";
}

/* Initialize keyboard */
void keyboard_init(void) {
    /* Reset keyboard state */
    shift_pressed = false;
    caps_lock = false;
    ctrl_pressed = false;
    alt_pressed = false;
    keyboard_head = 0;
    keyboard_tail = 0;
    
    /* Reset keyboard */
    outb(PS2_DATA_PORT, PS2_CMD_RESET_KB);
    
    /* Wait for ACK */
    while (!(inb(PS2_STATUS_PORT) & PS2_STATUS_OUT_BUF)) {
        __asm__ volatile("pause");
    }
    inb(PS2_DATA_PORT);  /* Should be 0xFA (ACK) */
}
