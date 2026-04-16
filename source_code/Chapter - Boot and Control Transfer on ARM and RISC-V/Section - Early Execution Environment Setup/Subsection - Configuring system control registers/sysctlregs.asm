AArch64: EL3 to EL1 Handoff and RISC-V M-Mode Early Configuration


    // AArch64: EL3 -> EL1 handoff (GNU as / aarch64)
    // symbolic constants provided by platform headers
    // SCR\_NS: enable Non-secure; SCR\_HCE: allow HVC from NS; CPACR\_FPEN: allow FP/SIMD
    // SCTLR\_MM\_ENABLE is intentionally not set here until page tables are active

    // Set CPACR\_EL1 to enable FP/SIMD for EL1
    mrs     x0, CPACR_EL1                // read
    and     x0, x0, #~CPACR_RESERVED_MASK // clear reserved bits
    orr     x0, x0, #CPACR_FPEN           // set FP enable
    msr     CPACR_EL1, x0                 // write
    isb                                  // ensure subsequent instructions see CPACR change

    // Configure SCR\_EL3 for non-secure handoff semantics
    mrs     x1, SCR_EL3
    and     x1, x1, #~SCR_RESERVED_MASK
    orr     x1, x1, #SCR_NS | SCR_HCE
    msr     SCR_EL3, x1
    isb

    // Prepare SCTLR\_EL1: keep reserved fields, clear MMU enable bit for now
    mrs     x2, SCTLR_EL1
    and     x2, x2, #~SCTLR_RESERVED_MASK
    // do not set SCTLR\_M (MMU) until TTBR/TCR/MAIR are configured
    msr     SCTLR_EL1, x2
    isb

// RISC-V: M-mode early configuration (GCC gas syntax)
    // Define symbolic values: S\_MODE, MSTATUS\_MPP\_MASK, MEDELEG\_ALL\_TRAPS
    // Delegate synchronous exceptions and interrupts to S-mode
    li      t0, MEDELEG_ALL_TRAPS
    csrw    medeleg, t0                   // delegate exceptions
    li      t0, MIDELEG_ALL_IRQS
    csrw    mideleg, t0                   // delegate interrupts

    // Set PMP to allow full memory access for the firmware region (example)
    li      t0, PMP_RWX                  // pmpcfg0 entry
    csrw    pmpcfg0, t0
    li      t0, PMP_ADDR_FULL
    csrw    pmpaddr0, t0

    // Prepare mstatus: set MPP = S (symbolic), clear FS to initial state
    csrr    t1, mstatus
    and     t1, t1, ~MSTATUS_MPP_MASK
    or      t1, t1, (S_MODE << MSTATUS_MPP_SHIFT)
    csrw    mstatus, t1
    csrw    mie, zero                     // mask machine interrupts during setup