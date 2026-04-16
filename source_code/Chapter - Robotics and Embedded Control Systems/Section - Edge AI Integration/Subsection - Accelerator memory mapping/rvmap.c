#include <stdint.h>
#include <stddef.h>

#define PTE_V   (1ULL << 0)
#define PTE_R   (1ULL << 1)
#define PTE_W   (1ULL << 2)
#define PTE_X   (1ULL << 3)
#define PTE_U   (1ULL << 4)
#define PTE_G   (1ULL << 5)
#define PTE_A   (1ULL << 6)
#define PTE_D   (1ULL << 7)

static inline int vpn_index(uint64_t va, int level) {
    return (va >> (12 + 9 * level)) & 0x1FF;
}

/* Map a physical range [pa, pa+size) to virtual [va, va+size).
   flags: combination of PTE_* bits (excluding V). root is pointer to level-2 page table. */
int map_region_sv39(uint64_t *root, uint64_t va, uint64_t pa, size_t size, uint64_t flags) {
    if (size == 0) return -1;

    while (size) {
        /* Try 2MiB superpage when both va and pa aligned and remaining >= 2MiB */
        if ((va & ((1ULL << 21) - 1)) == 0 && 
            (pa & ((1ULL << 21) - 1)) == 0 && 
            size >= (1ULL << 21)) {

            uint64_t *tab = root;
            /* Descend to level-1 */
            for (int l = 2; l > 1; --l) {
                int idx = vpn_index(va, l);
                uint64_t e = tab[idx];
                if (!(e & PTE_V)) {
                    return -2; /* Allocation required */
                }
                tab = (uint64_t*)((e >> 10) << 12);
            }

            int idx1 = vpn_index(va, 1);
            tab[idx1] = ((pa >> 12) << 10) | PTE_V | flags | PTE_A | PTE_D;
            va += (1ULL << 21);
            pa += (1ULL << 21);
            size -= (1ULL << 21);
            continue;
        }

        /* Fallback to 4KiB mapping */
        uint64_t *tab = root;
        for (int l = 2; l >= 0; --l) {
            int idx = vpn_index(va, l);
            if (l == 0) {
                tab[idx] = ((pa >> 12) << 10) | PTE_V | flags | PTE_A | PTE_D;
                va += 4096;
                pa += 4096;
                size -= 4096;
            } else {
                uint64_t e = tab[idx];
                if (!(e & PTE_V)) {
                    return -2; /* Allocation required */
                }
                tab = (uint64_t*)((e >> 10) << 12);
            }
        }
    }

    return 0;
}