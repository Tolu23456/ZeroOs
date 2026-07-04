#include "keyboard.h"
#include "idt.h"
#include "io.h"

#define KB_BUFFER_SIZE 256

static char kb_buffer[KB_BUFFER_SIZE];
static volatile uint8_t kb_head = 0;
static volatile uint8_t kb_tail = 0;
static bool shift_pressed = false;
static bool caps_lock = false;

static const char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 0,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0, 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // F1-F10
    0,  // Num Lock
    0,  // Scroll Lock
    0,  // Home
    0,  // Up
    0,  // Page Up
    '-', // Keypad -
    0,  // Left
    0,  // Keypad 5
    0,  // Right
    '+', // Keypad +
    0,  // End
    0,  // Down
    0,  // Page Down
    0,  // Insert
    0,  // Delete
    0, 0, 0,
    0, 0,  // F11, F12
};

static const char scancode_to_ascii_shift[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0, 0,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0, 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static void keyboard_callback(isr_regs_t *regs) {
    (void)regs;
    uint8_t scancode = inb(0x60);

    // Key release (bit 7 set)
    if (scancode & 0x80) {
        uint8_t released = scancode & 0x7F;
        if (released == 0x2A || released == 0x36) {  // Shift keys
            shift_pressed = false;
        }
        return;
    }

    // Special keys
    if (scancode == 0x2A || scancode == 0x36) {  // Shift press
        shift_pressed = true;
        return;
    }
    if (scancode == 0x3A) {  // Caps Lock
        caps_lock = !caps_lock;
        return;
    }

    // Convert scancode to ASCII
    char c = 0;
    if (shift_pressed) {
        c = scancode_to_ascii_shift[scancode];
    } else {
        c = scancode_to_ascii[scancode];
    }

    // Handle caps lock for letters
    if (caps_lock && c >= 'a' && c <= 'z') {
        c -= 32;  // to uppercase
    } else if (caps_lock && c >= 'A' && c <= 'Z') {
        c += 32;  // to lowercase
    }

    // Handle backspace separately
    if (scancode == 0x0E) {
        c = '\b';
    }

    if (c) {
        uint8_t next = (kb_head + 1) % KB_BUFFER_SIZE;
        if (next != kb_tail) {
            kb_buffer[kb_head] = c;
            kb_head = next;
        }
    }
}

void keyboard_init(void) {
    register_interrupt_handler(33, keyboard_callback);

    // Unmask IRQ 1
    uint8_t mask = inb(0x21);
    mask &= ~(1 << 1);  // Clear bit 1 (IRQ 1)
    outb(0x21, mask);
}

char keyboard_getchar(void) {
    while (kb_head == kb_tail) {
        __asm__ volatile ("hlt");
    }
    char c = kb_buffer[kb_tail];
    kb_tail = (kb_tail + 1) % KB_BUFFER_SIZE;
    return c;
}

bool keyboard_has_key(void) {
    return kb_head != kb_tail;
}
