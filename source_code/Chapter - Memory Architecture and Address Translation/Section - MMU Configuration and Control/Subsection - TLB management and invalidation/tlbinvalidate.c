/*
 * ARM AArch64 Translation Table Base Register Switch
 */

#include <stdint.h>

void arm_switch_ttbr(uint64_t ttbr_val) {
    // write TTBR0_EL1 then ensure old mappings cannot be used
    asm volatile(
        "msr    TTBR0_EL1, %0\n"    // update translation base
        "dsb    ish\n"              // ensure page-table stores visible
        "tlbi   vmalle1\n"          // invalidate all EL1/EL0 TLB entries
        "dsb    ish\n"              // wait for invalidation to complete
        "isb\n"                     // synchronize execution
        :
        : "r"(ttbr_val)
        : "memory");
}