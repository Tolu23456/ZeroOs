#ifndef ZEROOS_VMM_H
#define ZEROOS_VMM_H

#include <zeroos/types.h>
#include "mm.h"

void vmm_init(void);
void vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags);
void vmm_unmap_page(uint64_t virt);
uint64_t vmm_virt_to_phys(uint64_t virt);
void vmm_switch_page_directory(uint64_t *pml4);

#endif
