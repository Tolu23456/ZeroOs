#include "vga.h"

static uint16_t *vga_buffer = (uint16_t*)VGA_BUFFER;
static int vga_row = 0;
static int vga_col = 0;
static uint8_t vga_attr = 0x0F;  // White on black

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | (bg << 4);
}

static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

static void vga_scroll(void) {
    for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
        vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }
    for (int i = VGA_WIDTH * (VGA_HEIGHT - 1); i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = vga_entry(' ', vga_attr);
    }
    vga_row = VGA_HEIGHT - 1;
}

void vga_init(void) {
    vga_clear();
}

void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = vga_entry(' ', vga_attr);
    }
    vga_row = 0;
    vga_col = 0;
}

void vga_set_color(enum vga_color fg, enum vga_color bg) {
    vga_attr = vga_entry_color(fg, bg);
}

void vga_putchar(char c) {
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_col = 0;
    } else if (c == '\t') {
        vga_col = (vga_col + 8) & ~7;
    } else if (c == '\b') {
        if (vga_col > 0) {
            vga_col--;
            vga_buffer[vga_row * VGA_WIDTH + vga_col] = vga_entry(' ', vga_attr);
        }
    } else {
        vga_buffer[vga_row * VGA_WIDTH + vga_col] = vga_entry(c, vga_attr);
        vga_col++;
    }

    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        vga_row++;
    }

    if (vga_row >= VGA_HEIGHT) {
        vga_scroll();
    }
}

void vga_puts(const char *str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

// Simple number to string conversion for printf
static void put_num(int64_t n, int base) {
    char buf[32];
    int i = 0;
    uint64_t un;

    if (n < 0 && base == 10) {
        vga_putchar('-');
        un = (uint64_t)(-n);
    } else {
        un = (uint64_t)n;
    }

    if (un == 0) {
        vga_putchar('0');
        return;
    }

    while (un > 0) {
        buf[i++] = "0123456789abcdef"[un % base];
        un /= base;
    }

    while (i > 0) {
        vga_putchar(buf[--i]);
    }
}

void vga_printf(const char *fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'd':
                case 'i':
                    put_num(__builtin_va_arg(args, int64_t), 10);
                    break;
                case 'u':
                    put_num(__builtin_va_arg(args, uint64_t), 10);
                    break;
                case 'x':
                    vga_puts("0x");
                    put_num(__builtin_va_arg(args, uint64_t), 16);
                    break;
                case 's': {
                    const char *s = __builtin_va_arg(args, const char*);
                    if (s) vga_puts(s);
                    break;
                }
                case 'c':
                    vga_putchar((char)__builtin_va_arg(args, int));
                    break;
                case 'p':
                    vga_puts("0x");
                    put_num((uint64_t)__builtin_va_arg(args, void*), 16);
                    break;
                case '%':
                    vga_putchar('%');
                    break;
                default:
                    vga_putchar('%');
                    vga_putchar(*fmt);
                    break;
            }
        } else {
            vga_putchar(*fmt);
        }
        fmt++;
    }

    __builtin_va_end(args);
}
