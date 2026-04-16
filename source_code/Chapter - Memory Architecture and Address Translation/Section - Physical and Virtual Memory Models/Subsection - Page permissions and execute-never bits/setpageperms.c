\title{Page Table Entry Permission Management for RISC-V and ARM (AArch64)}
\caption{Functions to modify page table entry permissions and flush TLBs on RISC-V and AArch64 architectures.}

#include <stdint.h>

/* RISC-V: set PTE to R/W/X/U bits and flush TLB for single page */
int set_page_permissions_riscv(volatile uint64_t *ptep, uintptr_t va, int R, int W, int X, int U) {
    uint64_t pte = *ptep;
    /* preserve physical ppn and flags except permission bits */
    uint64_t ppn_mask = (~0ULL) << 10;
    uint64_t new_pte = (pte & ppn_mask)
                     | ((uint64_t)R << 1)
                     | ((uint64_t)W << 2)
                     | ((uint64_t)X << 3)
                     | ((uint64_t)U << 4)
                     | (1ULL << 6); /* keep A bit set */
    *ptep = new_pte; /* store PTE */
    asm volatile("sfence.vma %0, x0" :: "r"(va) : "memory"); /* flush this VA */
    return 0;
}

/* AArch64: set PTE permissions (R/W) and UXN/PXN control; flush entire TLB */
int set_page_permissions_aarch64(volatile uint64_t *ptep, int read, int write, int uxn, int pxn) {
    uint64_t pte = *ptep;
    /* PTE format: preserve output addr and attrs, replace AP and UXN/PXN bits */
    /* Construct minimal example: set AP to read-only or read-write */
    uint64_t ap = (read && !write) ? 0x5 : (read && write ? 0x3 : 0x7); /* simplified AP encoding */
    uint64_t new_pte = (pte & ~((uint64_t)0xFFF))
                     | (ap << 6)
                     | ((uint64_t)uxn << 54)
                     | ((uint64_t)pxn << 53);
    *ptep = new_pte; /* store PTE */
    asm volatile(
        "dsb ish\n"             /* ensure PTE reaches memory */
        "tlbi vmalle1\n"        /* invalidate stage-1 TLBs on all EL1/EL0 */
        "dsb ish\n"
        "isb\n" ::: "memory");
    return 0;
}