\title{RISC-V PMP Configuration Code}
\caption{Production-ready RISC-V PMP setup for NAPOT region with memory ordering}

#include <stdint.h>
#include <assert.h>

#define PMP_R        0x01
#define PMP_W        0x02
#define PMP_X        0x04
#define PMP_A_NAPOT  (0x3UL << 3)  // A=3 selects NAPOT addressing

static inline void pmp_install_napot_entry0(uint64_t addr, uint64_t size, uint8_t perms)
{
    assert(size >= 8 && (size & (size - 1)) == 0);
    assert((addr & (size - 1)) == 0);

    uint64_t pmpaddr = (addr | (size - 1)) >> 2;
    uint8_t cfg = perms | PMP_A_NAPOT;

    asm volatile ("csrw pmpaddr0, %0" :: "r"(pmpaddr) : "memory");
    uint64_t cfg64 = (uint64_t)cfg;
    asm volatile ("csrw pmpcfg0, %0" :: "r"(cfg64) : "memory");

    asm volatile ("fence.i" ::: "memory");
}

void init_pmp_for_ram(void)
{
    uint64_t ram_base = 0x80000000UL;
    uint64_t ram_size = 64 * 1024UL;
    uint8_t perms = PMP_R | PMP_W;
    pmp_install_napot_entry0(ram_base, ram_size, perms);
}