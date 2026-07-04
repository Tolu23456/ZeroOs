#include "mm.h"
#include "string.h"
#include "vga.h"

#define BITMAP_SIZE (128 * 1024)  // 128KB bitmap = 1M pages = 4GB address space
#define BITMAP_BYTES (BITMAP_SIZE / 8)

static uint8_t bitmap[BITMAP_SIZE];
static uint64_t total_pages = 0;
static uint64_t used_pages = 0;

static inline void bitmap_set(uint64_t page) {
    bitmap[page / 8] |= (1 << (page % 8));
}

static inline void bitmap_clear(uint64_t page) {
    bitmap[page / 8] &= ~(1 << (page % 8));
}

static inline bool bitmap_test(uint64_t page) {
    return bitmap[page / 8] & (1 << (page % 8));
}

void pmm_init(e820_entry_t *map, uint64_t entries) {
    // Clear bitmap
    memset(bitmap, 0, sizeof(bitmap));

    // Find the highest memory address to determine total pages
    uint64_t max_addr = 0;
    for (uint64_t i = 0; i < entries; i++) {
        uint64_t end = map[i].base + map[i].length;
        if (end > max_addr) {
            max_addr = end;
        }
    }

    total_pages = max_addr / PAGE_SIZE;

    // Mark all pages as used initially
    for (uint64_t i = 0; i < total_pages; i++) {
        bitmap_set(i);
    }

    // Mark available pages as free
    for (uint64_t i = 0; i < entries; i++) {
        if (map[i].type != 1) continue;  // Only type 1 = available

        uint64_t start_page = (map[i].base + PAGE_SIZE - 1) / PAGE_SIZE;
        uint64_t end_page = (map[i].base + map[i].length) / PAGE_SIZE;

        // Don't allow identity mapping to conflict with kernel at 1MB
        if (start_page < 0x100) start_page = 0x100;  // Skip first 1MB

        for (uint64_t p = start_page; p < end_page; p++) {
            bitmap_clear(p);
            used_pages--;
        }
    }

    // Mark bitmap itself as used
    uint64_t bitmap_start = (uint64_t)bitmap / PAGE_SIZE;
    uint64_t bitmap_pages = (sizeof(bitmap) + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t p = bitmap_start; p < bitmap_start + bitmap_pages; p++) {
        if (!bitmap_test(p)) {
            bitmap_set(p);
            used_pages++;
        }
    }

    // Mark kernel as used (from 1MB to current end)
    extern uint64_t __kernel_end;
    uint64_t kernel_end = (uint64_t)&__kernel_end;
    uint64_t kernel_start_page = 0x100000 / PAGE_SIZE;
    uint64_t kernel_end_page = (kernel_end + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t p = kernel_start_page; p < kernel_end_page; p++) {
        if (!bitmap_test(p)) {
            bitmap_set(p);
            used_pages++;
        }
    }

    vga_printf("PMM: %d pages total, %d used, %d free\n",
        total_pages, used_pages, total_pages - used_pages);
}

void *pmm_alloc_page(void) {
    for (uint64_t i = 0; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            used_pages++;
            return (void*)(i * PAGE_SIZE);
        }
    }
    return NULL;  // Out of memory
}

void pmm_free_page(void *addr) {
    uint64_t page = (uint64_t)addr / PAGE_SIZE;
    if (page < total_pages && bitmap_test(page)) {
        bitmap_clear(page);
        used_pages--;
    }
}

uint64_t pmm_get_free_pages(void) {
    return total_pages - used_pages;
}
