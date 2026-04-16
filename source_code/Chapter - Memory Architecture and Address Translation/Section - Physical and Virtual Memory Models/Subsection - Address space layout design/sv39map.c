\title{RISC-V Sv39 Page Table Mapping Implementation}

#include <stdint.h>
#include <stddef.h>

/* Sv39 constants */
#define PAGE_SIZE       4096UL
#define PTE_PER_PT      512UL    /* 9 bits */
#define PPN_SHIFT       10       /* PTE[53:10] holds PPN */
#define PTE_V           (1UL << 0)
#define PTE_R           (1UL << 1)
#define PTE_W           (1UL << 2)
#define PTE_X           (1UL << 3)
#define PTE_U           (1UL << 4)
#define PTE_G           (1UL << 5)
#define PTE_A           (1UL << 6)
#define PTE_D           (1UL << 7)

/* A simple page-table allocator provided by boot code */
extern uint64_t *alloc_page_table(void);

/* Insert a mapping of 'size' bytes at virt->phys with flags. */
void map_range_sv39(uint64_t *root, uint64_t virt, uint64_t phys,
                    size_t size, uint64_t flags) {
    while (size) {
        /* Prefer 1GiB superpage if aligned and size>=1GiB */
        if ((virt & ((1UL << 30) - 1)) == 0 && 
            (phys & ((1UL << 30) - 1)) == 0 && 
            size >= (1UL << 30)) {
            /* index at level 2 (root) */
            size_t idx = (virt >> 30) & 0x1FF;
            uint64_t pte = (phys >> 12) << PPN_SHIFT;
            pte |= flags | PTE_V;
            root[idx] = pte;              /* block entry at level 0 in Sv39 */
            virt += (1UL << 30);
            phys += (1UL << 30);
            size -= (1UL << 30);
            continue;
        }
        /* Else, try 2MiB block at level 1 */
        if ((virt & ((1UL << 21) - 1)) == 0 && 
            (phys & ((1UL << 21) - 1)) == 0 && 
            size >= (1UL << 21)) {
            uint64_t *l1 = root;
            size_t idx2 = (virt >> 30) & 0x1FF;
            if (!(l1[idx2] & PTE_V)) {
                l1[idx2] = (uint64_t)alloc_page_table() | PTE_V;
            }
            uint64_t *l1pt = (uint64_t *)(l1[idx2] & ~0xFFFUL);
            size_t idx1 = (virt >> 21) & 0x1FF;
            uint64_t pte = (phys >> 12) << PPN_SHIFT;
            pte |= flags | PTE_V;
            l1pt[idx1] = pte;            /* 2MiB block at level 1 */
            virt += (1UL << 21);
            phys += (1UL << 21);
            size -= (1UL << 21);
            continue;
        }
        /* Fallback to 4KiB page: walk/create down to level 0 */
        uint64_t *pt = root;
        for (int level = 2; level > 0; --level) {
            size_t idx = (virt >> (12 + level * 9)) & 0x1FF;
            if (!(pt[idx] & PTE_V)) {
                pt[idx] = (uint64_t)alloc_page_table() | PTE_V;
            }
            pt = (uint64_t *)(pt[idx] & ~0xFFFUL);
        }
        size_t idx0 = (virt >> 12) & 0x1FF;
        uint64_t pte = (phys >> 12) << PPN_SHIFT;
        pte |= flags | PTE_V;
        pt[idx0] = pte;
        virt += PAGE_SIZE;
        phys += PAGE_SIZE;
        size -= PAGE_SIZE;
    }
}