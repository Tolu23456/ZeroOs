#include "timer.h"
#include "idt.h"
#include "io.h"

static volatile uint64_t ticks = 0;

static void timer_callback(isr_regs_t *regs) {
    (void)regs;
    ticks++;
}

void timer_init(uint32_t freq) {
    register_interrupt_handler(32, timer_callback);

    // PIT (Programmable Interval Timer) channel 0
    // Frequency = 1193182 / divisor
    uint32_t divisor = 1193182 / freq;

    // Send command byte: channel 0, lobyte/hibyte, rate generator
    outb(0x43, 0x36);

    // Send divisor
    outb(0x40, divisor & 0xFF);         // Low byte
    outb(0x40, (divisor >> 8) & 0xFF);  // High byte

    // Unmask IRQ 0
    uint8_t mask = inb(0x21);
    mask &= ~(1 << 0);  // Clear bit 0 (IRQ 0)
    outb(0x21, mask);
}

uint64_t timer_get_ticks(void) {
    return ticks;
}

void timer_sleep(uint64_t ms) {
    uint64_t target = ticks + ms;
    while (ticks < target) {
        __asm__ volatile ("hlt");
    }
}
