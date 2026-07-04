#ifndef ZEROOS_MM_H
#define ZEROOS_MM_H

#include <zeroos/types.h>

#define PAGE_SIZE 4096

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
} __attribute__((packed)) e820_entry_t;

void pmm_init(e820_entry_t *map, uint64_t entries);
void *pmm_alloc_page(void);
void pmm_free_page(void *addr);
uint64_t pmm_get_free_pages(void);

#endif
