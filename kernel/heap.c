#include "heap.h"
#include "mm.h"
#include "vmm.h"
#include "string.h"
#include "vga.h"

#define HEAP_START   0x200000   // 2MB
#define HEAP_INITIAL_PAGES 16   // 64KB initial heap
#define PDE_WRITABLE 0x02

typedef struct block {
    size_t size;            // Size of data (excluding header)
    bool free;
    struct block *next;
} block_t;

static block_t *heap_head = NULL;
static uint64_t heap_current = HEAP_START;
static size_t heap_used = 0;

static void heap_expand(size_t needed) {
    size_t pages = (needed + PAGE_SIZE - 1) / PAGE_SIZE;
    for (size_t i = 0; i < pages; i++) {
        void *page = pmm_alloc_page();
        if (!page) return;

        // Identity map the page
        vmm_map_page(heap_current, (uint64_t)page, PDE_WRITABLE);
        heap_current += PAGE_SIZE;
    }
}

void heap_init(void) {
    // Allocate initial pages for heap
    heap_current = HEAP_START;
    for (size_t i = 0; i < HEAP_INITIAL_PAGES; i++) {
        void *page = pmm_alloc_page();
        if (!page) break;
        vmm_map_page(heap_current, (uint64_t)page, PDE_WRITABLE);
        heap_current += PAGE_SIZE;
    }

    // Create initial free block
    heap_head = (block_t*)HEAP_START;
    heap_head->size = (HEAP_INITIAL_PAGES * PAGE_SIZE) - sizeof(block_t);
    heap_head->free = true;
    heap_head->next = NULL;

    vga_printf("HEAP: Initialized at 0x%x, %d KB\n",
        HEAP_START, HEAP_INITIAL_PAGES * 4);
}

static block_t *find_free_block(size_t size) {
    block_t *current = heap_head;
    while (current) {
        if (current->free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

static void split_block(block_t *block, size_t size) {
    if (block->size >= size + sizeof(block_t) + 16) {
        block_t *new_block = (block_t*)((uint8_t*)block + sizeof(block_t) + size);
        new_block->size = block->size - size - sizeof(block_t);
        new_block->free = true;
        new_block->next = block->next;
        block->next = new_block;
        block->size = size;
    }
}

static void merge_free_blocks(void) {
    block_t *current = heap_head;
    while (current && current->next) {
        if (current->free && current->next->free) {
            current->size += sizeof(block_t) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;

    block_t *block = find_free_block(size);
    if (!block) {
        // Expand heap
        heap_expand(size + sizeof(block_t));
        block = find_free_block(size);
        if (!block) return NULL;
    }

    split_block(block, size);
    block->free = false;
    heap_used += block->size;

    return (void*)((uint8_t*)block + sizeof(block_t));
}

void *kmalloc_aligned(size_t size) {
    // Allocate with alignment to page boundary
    size_t aligned_size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    return kmalloc(aligned_size);
}

void kfree(void *ptr) {
    if (!ptr) return;

    block_t *block = (block_t*)((uint8_t*)ptr - sizeof(block_t));
    heap_used -= block->size;
    block->free = true;

    merge_free_blocks();
}

size_t heap_get_used(void) {
    return heap_used;
}
