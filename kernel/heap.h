#ifndef ZEROOS_HEAP_H
#define ZEROOS_HEAP_H

#include <zeroos/types.h>

void heap_init(void);
void *kmalloc(size_t size);
void *kmalloc_aligned(size_t size);
void kfree(void *ptr);
size_t heap_get_used(void);

#endif
