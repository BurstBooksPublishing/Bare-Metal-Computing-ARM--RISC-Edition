\title{RISC-V SV39 2MiB Block Mapping Implementation}

#include <stdint.h>
#include <stddef.h>

// PTE bit masks (canonical names)
#define PTE\_V   (1ULL << 0)
#define PTE\_R   (1ULL << 1)
#define PTE\_W   (1ULL << 2)
#define PTE\_X   (1ULL << 3)
#define PTE\_U   (1ULL << 4)
#define PTE\_G   (1ULL << 5)
#define PTE\_A   (1ULL << 6)
#define PTE\_D   (1ULL << 7)

#define PAGE\_SIZE       4096ULL
#define ENTRIES\_PER\_PT  512ULL
#define BLOCK\_2MB\_SIZE  (PAGE\_SIZE * ENTRIES\_PER\_PT) // 2MiB

// Build a level-1 block PTE for a 2MiB mapping.
// pa must be 2MiB-aligned. attrs contains PTE\_R|PTE\_W|PTE\_X etc.
static inline uint64_t build\_sv39\_block\_pte(uint64_t pa, uint64_t attrs) {
    // PPN for block: bits [53:12] of physical address; for a 2MiB block
    uint64_t ppn = (pa >> 12) & ((1ULL << 42) - 1); // mask sufficiently wide
    // For block PTE at level-1, low PPN bits that would index lower pages must be zero.
    // Compose pte: PPN << 10 | flags (as per RISC-V privileged spec)
    return (ppn << 10) | (attrs & 0x3FFULL);
}

// Install the PTE into a level-2 table at index 'idx'.
// lvl2\_table must be 4KiB aligned and contain 512 uint64\_t entries.
void map\_2mb\_block(uint64\_t *lvl2\_table, size\_t idx, uint64\_t phys\_base, uint64\_t attrs) {
    // Validate alignment and index range in production code (omitted for brevity).
    uint64\_t pte = build\_sv39\_block\_pte(phys\_base, attrs | PTE\_V | PTE\_A | PTE\_D);
    lvl2\_table[idx] = pte;
    // Caller must flush data cache lines for lvl2\_table and sfence.vma as needed.
}