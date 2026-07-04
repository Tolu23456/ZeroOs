#include "vga.h"
#include "io.h"
#include "idt.h"
#include "timer.h"
#include "keyboard.h"
#include "mm.h"
#include "vmm.h"
#include "heap.h"
#include "process.h"
#include "syscall.h"
#include "string.h"

void kmain(void *e820_map, uint64_t e820_entries) {
    // Initialize VGA
    vga_init();
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_printf("ZeroOs Kernel v0.1\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);

    // Parse E820 memory map
    e820_entry_t *entries = (e820_entry_t*)e820_map;
    uint64_t total_memory = 0;

    const char *type_names[] = {
        "Reserved", "Available", "ACPI Reclaimable", "ACPI NVS", "Bad"
    };

    for (uint64_t i = 0; i < e820_entries; i++) {
        if (entries[i].type == 1) {
            total_memory += entries[i].length;
        }
        vga_set_color(VGA_DARK_GREY, VGA_BLACK);
        vga_printf("  [%d] Base: 0x%x Len: 0x%x Type: %s\n",
            i, entries[i].base, entries[i].length,
            type_names[entries[i].type < 5 ? entries[i].type : 4]);
    }

    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_printf("\nTotal Available Memory: %d MB\n", total_memory / (1024 * 1024));

    // Initialize kernel subsystems
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_printf("\nInitializing kernel subsystems...\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);

    // IDT (interrupts)
    idt_init();
    vga_printf("[OK] IDT initialized\n");

    // Physical memory manager
    pmm_init(entries, e820_entries);

    // Virtual memory manager
    vmm_init();
    vga_printf("[OK] VMM initialized\n");

    // Kernel heap
    heap_init();
    vga_printf("[OK] Heap initialized\n");

    // Timer (100 Hz)
    timer_init(100);
    vga_printf("[OK] Timer initialized (100 Hz)\n");

    // Keyboard
    keyboard_init();
    vga_printf("[OK] Keyboard initialized\n");

    // Process scheduler
    process_init();
    vga_printf("[OK] Process scheduler initialized\n");

    // System calls
    syscall_init();
    vga_printf("[OK] System calls initialized\n");

    // Test kernel heap
    void *test1 = kmalloc(128);
    void *test2 = kmalloc(256);
    if (test1 && test2) {
        vga_printf("[OK] Heap test passed (allocated 128 + 256 bytes)\n");
        kfree(test1);
        kfree(test2);
    }

    // Test timer
    vga_printf("[OK] Timer test: waiting 500ms...\n");
    timer_sleep(500);
    vga_printf("[OK] Timer test passed\n");

    // Kernel is running!
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_printf("\nZeroOs kernel initialized successfully!\n");
    vga_printf("Phase 2 complete - kernel core operational.\n");
    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_printf("System halted.\n");

    // Halt
    cli();
    while (1) {
        hlt();
    }
}
