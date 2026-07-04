#include "vmm.h"
#include "mm.h"
#include "string.h"
#include "vga.h"

#define PML4_INDEX(x) (((x) >> 39) & 0x1FF)
#define PDPT_INDEX(x) (((x) >> 30) & 0x1FF)
#define PD_INDEX(x)   (((x) >> 21) & 0x1FF)
#define PT_INDEX(x)   (((x) >> 12) & 0x1FF)

#define PDE_PRESENT  0x01
#define PDE_WRITABLE 0x02
#define PDE_USER     0x04
#define PDE_2MB      0x80
#define PDE_PRESENT_BIT 0
#define PDE_WRITABLE_BIT 1
#define PDE_USER_BIT 2

static uint64_t *kernel_pml4 = NULL;

static uint64_t *get_or_create_entry(uint64_t *table, uint64_t index, uint64_t flags) {
    if (!(table[index] & PDE_PRESENT)) {
        uint64_t *new_table = (uint64_t*)pmm_alloc_page();
        if (!new_table) return NULL;
        memset(new_table, 0, PAGE_SIZE);
        table[index] = (uint64_t)new_table | flags;
    }
    return (uint64_t*)(table[index] & ~0xFFFULL);
}

void vmm_init(void) {
    // Get current PML4 from CR3
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    kernel_pml4 = (uint64_t*)(cr3 & ~0xFFFULL);

    vga_printf("VMM: Initialized with PML4 at 0x%x\n", kernel_pml4);
}

void vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t pml4_idx = PML4_INDEX(virt);
    uint64_t pdpt_idx = PDPT_INDEX(virt);
    uint64_t pd_idx   = PD_INDEX(virt);
    uint64_t pt_idx   = PT_INDEX(virt);

    uint64_t *pdpt = get_or_create_entry(kernel_pml4, pml4_idx, flags | PDE_PRESENT | PDE_WRITABLE);
    if (!pdpt) return;

    uint64_t *pd = get_or_create_entry(pdpt, pdpt_idx, flags | PDE_PRESENT | PDE_WRITABLE);
    if (!pd) return;

    uint64_t *pt = get_or_create_entry(pd, pd_idx, flags | PDE_PRESENT | PDE_WRITABLE);
    if (!pt) return;

    pt[pt_idx] = (phys & ~0xFFFULL) | (flags | PDE_PRESENT);
}

void vmm_unmap_page(uint64_t virt) {
    uint64_t pml4_idx = PML4_INDEX(virt);
    uint64_t pdpt_idx = PDPT_INDEX(virt);
    uint64_t pd_idx   = PD_INDEX(virt);
    uint64_t pt_idx   = PT_INDEX(virt);

    uint64_t *pdpt = (uint64_t*)(kernel_pml4[pml4_idx] & ~0xFFFULL);
    if (!pdpt) return;

    uint64_t *pd = (uint64_t*)(pdpt[pdpt_idx] & ~0xFFFULL);
    if (!pd) return;

    uint64_t *pt = (uint64_t*)(pd[pd_idx] & ~0xFFFULL);
    if (!pt) return;

    pt[pt_idx] = 0;

    // Flush TLB
    __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");
}

uint64_t vmm_virt_to_phys(uint64_t virt) {
    uint64_t pml4_idx = PML4_INDEX(virt);
    uint64_t pdpt_idx = PDPT_INDEX(virt);
    uint64_t pd_idx   = PD_INDEX(virt);
    uint64_t pt_idx   = PT_INDEX(virt);

    uint64_t *pdpt = (uint64_t*)(kernel_pml4[pml4_idx] & ~0xFFFULL);
    if (!pdpt) return 0;

    uint64_t *pd = (uint64_t*)(pdpt[pdpt_idx] & ~0xFFFULL);
    if (!pd) return 0;

    uint64_t *pt = (uint64_t*)(pd[pd_idx] & ~0xFFFULL);
    if (!pt) return 0;

    return pt[pt_idx] & ~0xFFFULL;
}

void vmm_switch_page_directory(uint64_t *pml4) {
    __asm__ volatile ("mov %0, %%cr3" : : "r"(pml4) : "memory");
}
