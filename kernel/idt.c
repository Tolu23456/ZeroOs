#include "idt.h"
#include "io.h"
#include "vga.h"

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t idt_ptr;
static isr_handler_t isr_handlers[IDT_ENTRIES];

extern void idt_flush(uint64_t);

void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = base & 0xFFFF;
    idt[num].base_mid  = (base >> 16) & 0xFFFF;
    idt[num].base_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].selector  = sel;
    idt[num].ist       = 0;
    idt[num].type_attr = flags;
    idt[num].zero      = 0;
}

void register_interrupt_handler(uint8_t n, isr_handler_t handler) {
    isr_handlers[n] = handler;
}

void isr_handler(isr_regs_t *regs) {
    if (isr_handlers[regs->int_no]) {
        isr_handlers[regs->int_no](regs);
    } else {
        vga_set_color(VGA_RED, VGA_BLACK);
        vga_printf("\nUNHANDLED EXCEPTION %d at RIP=0x%x\n", regs->int_no, regs->rip);
        vga_printf("Error code: 0x%x\n", regs->err_code);
        vga_set_color(VGA_WHITE, VGA_BLACK);
        while (1) { __asm__ volatile ("cli; hlt"); }
    }
}

void irq_handler(isr_regs_t *regs) {
    // Send EOI (End of Interrupt) to PICs
    if (regs->int_no >= 40) {
        outb(0xA0, 0x20);  // Slave PIC
    }
    outb(0x20, 0x20);  // Master PIC

    if (isr_handlers[regs->int_no]) {
        isr_handlers[regs->int_no](regs);
    }
}

static void idt_remap_pic(void) {
    // ICW1: Initialize
    outb(0x20, 0x11);  // Master PIC
    outb(0xA0, 0x11);  // Slave PIC

    // ICW2: Vector offset
    outb(0x21, 0x20);  // Master: IRQ 0-7 -> INT 32-39
    outb(0xA1, 0x28);  // Slave: IRQ 8-15 -> INT 40-47

    // ICW3: Master/Slave wiring
    outb(0x21, 0x04);  // Master: slave on IRQ2
    outb(0xA1, 0x02);  // Slave: cascade identity

    // ICW4: 8086 mode
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    // Mask all IRQs
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

void idt_init() {
    idt_ptr.limit = sizeof(idt_entry_t) * IDT_ENTRIES - 1;
    idt_ptr.base = (uint64_t)&idt;

    // Clear handlers
    for (int i = 0; i < IDT_ENTRIES; i++) {
        isr_handlers[i] = 0;
        idt_set_gate(i, 0, 0, 0);
    }

    // Remap PIC
    idt_remap_pic();

    // ISRs 0-31 (CPU exceptions)
    extern void isr0();  extern void isr1();  extern void isr2();  extern void isr3();
    extern void isr4();  extern void isr5();  extern void isr6();  extern void isr7();
    extern void isr8();  extern void isr9();  extern void isr10(); extern void isr11();
    extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
    extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
    extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
    extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
    extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();

    idt_set_gate(0,  (uint64_t)isr0,  0x08, 0x8E);
    idt_set_gate(1,  (uint64_t)isr1,  0x08, 0x8E);
    idt_set_gate(2,  (uint64_t)isr2,  0x08, 0x8E);
    idt_set_gate(3,  (uint64_t)isr3,  0x08, 0x8E);
    idt_set_gate(4,  (uint64_t)isr4,  0x08, 0x8E);
    idt_set_gate(5,  (uint64_t)isr5,  0x08, 0x8E);
    idt_set_gate(6,  (uint64_t)isr6,  0x08, 0x8E);
    idt_set_gate(7,  (uint64_t)isr7,  0x08, 0x8E);
    idt_set_gate(8,  (uint64_t)isr8,  0x08, 0x8E);
    idt_set_gate(9,  (uint64_t)isr9,  0x08, 0x8E);
    idt_set_gate(10, (uint64_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint64_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint64_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint64_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint64_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint64_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint64_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint64_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint64_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint64_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint64_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint64_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint64_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint64_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint64_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint64_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint64_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint64_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint64_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint64_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint64_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint64_t)isr31, 0x08, 0x8E);

    // IRQs 0-15 (mapped to INT 32-47)
    extern void irq0();  extern void irq1();  extern void irq2();  extern void irq3();
    extern void irq4();  extern void irq5();  extern void irq6();  extern void irq7();
    extern void irq8();  extern void irq9();  extern void irq10(); extern void irq11();
    extern void irq12(); extern void irq13(); extern void irq14(); extern void irq15();

    idt_set_gate(32, (uint64_t)irq0,  0x08, 0x8E);
    idt_set_gate(33, (uint64_t)irq1,  0x08, 0x8E);
    idt_set_gate(34, (uint64_t)irq2,  0x08, 0x8E);
    idt_set_gate(35, (uint64_t)irq3,  0x08, 0x8E);
    idt_set_gate(36, (uint64_t)irq4,  0x08, 0x8E);
    idt_set_gate(37, (uint64_t)irq5,  0x08, 0x8E);
    idt_set_gate(38, (uint64_t)irq6,  0x08, 0x8E);
    idt_set_gate(39, (uint64_t)irq7,  0x08, 0x8E);
    idt_set_gate(40, (uint64_t)irq8,  0x08, 0x8E);
    idt_set_gate(41, (uint64_t)irq9,  0x08, 0x8E);
    idt_set_gate(42, (uint64_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint64_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint64_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint64_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint64_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint64_t)irq15, 0x08, 0x8E);

    // Load IDT
    __asm__ volatile ("lidt %0" : : "m"(idt_ptr));
}
