High-Performance Timer Access for ARM AArch64 and RISC-V Architectures


#include <stdint.h>

// Read AArch64 generic timer virtual count (CNTVCT\_EL0).
static inline uint64_t arm_read_cntvct(void) {
    uint64_t val;
    __asm__ volatile (
        "isb\n"                     // ensure prior instructions complete
        "mrs %0, cntvct_el0\n"      // read 64-bit virtual count
        : "=r" (val)
        :
        : "memory"
    );
    return val;
}

// Read RISC-V cycle CSR (RV64). Requires mcounteren permitting user access.
static inline uint64_t riscv_read_cycle(void) {
    uint64_t val;
    __asm__ volatile (
        "fence\n"                   // order memory prior to timestamp
        "rdcycle %0\n"              // RV64 instruction to read cycle CSR
        : "=r" (val)
        :
        : "memory"
    );
    return val;
}