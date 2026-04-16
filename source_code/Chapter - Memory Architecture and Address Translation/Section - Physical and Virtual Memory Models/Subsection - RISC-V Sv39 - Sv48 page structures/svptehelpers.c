RISC-V Page Table Entry Utilities (RV64)


#include <stdint.h>
#include <stdbool.h>

#define PTE_V    (1ULL << 0)
#define PTE_R    (1ULL << 1)
#define PTE_W    (1ULL << 2)
#define PTE_X    (1ULL << 3)
#define PTE_U    (1ULL << 4)
#define PTE_G    (1ULL << 5)
#define PTE_A    (1ULL << 6)
#define PTE_D    (1ULL << 7)

static inline unsigned vpn_index(uint64_t va, unsigned level) {
    // level 0 => VPN[0], level 1 => VPN[1], etc.
    return (va >> (12 + 9 * level)) & 0x1FFU;
}

static inline uint64_t make_pte_leaf(uint64_t ppn, uint64_t flags) {
    // PPN already shifted right by 12; flags include V/R/W/X/U/G/A/D
    return (ppn << 10) | (flags & 0xFF);
}

static inline bool pte_is_leaf(uint64_t pte) {
    return (pte & (PTE_R | PTE_W | PTE_X)) != 0;
}

// Example: compute mapping size for level
static inline uint64_t mapping_size_for_level(unsigned level) {
    return 1ULL << (12 + 9 * level);
}