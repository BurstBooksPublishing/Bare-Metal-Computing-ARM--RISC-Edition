/* ARM (AArch64): enable EL1 MMU (example); registers in x0..x3 */
/* set TTBR0_EL1, TCR_EL1, MAIR_EL1, then enable in SCTLR_EL1 */
    msr     ttbr0_el1, x0           // set level-1 translation base
    msr     tcr_el1, x1             // set granule, ASID size, etc.
    msr     mair_el1, x2            // memory attributes
    msr     sctlr_el1, x3           // update control (enable bit set)
    dsb     sy                      // wait for previous stores to complete globally
    isb                             // ensure new translations apply to following fetches

/* RISC-V: switch page table root (satp) and flush TLB/ICache */
/* assume new root in a0, use zero to flush all entries */
    csrw    satp, a0                // install new page-table root
    sfence.vma zero, zero           // invalidate all entries in TLB
    fence.i                         // synchronize I-cache with any new mappings