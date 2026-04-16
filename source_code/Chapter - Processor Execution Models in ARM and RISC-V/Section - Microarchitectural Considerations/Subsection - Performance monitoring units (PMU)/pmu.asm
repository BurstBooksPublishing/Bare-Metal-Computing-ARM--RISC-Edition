ARM (AArch64) and RISC-V Assembly Code for Performance Counter Access


    .global arm_enable_and_read_cycle
arm_enable_and_read_cycle:
    // Enable user-mode access to PMU if desired: write 0x1 to PMUSERENR_EL0
    mov     x0, #1
    msr     PMUSERENR_EL0, x0       // allow EL0 reads if kernel grants

    // Reset and enable PMU: PMCR_EL0.E (enable) = 1
    mov     x0, #1
    msr     PMCR_EL0, x0            // enable counters, default div and reset clear

    // Enable cycle counter specifically: set bit 31 in PMCNTENSET_EL0
    mov     x0, #0x80000000
    msr     PMCNTENSET_EL0, x0      // enable cycle counter

    // Read cycle counter into x1 and return
    mrs     x1, PMCCNTR_EL0
    ret

    .global riscv_config_and_read_hpm3
riscv_config_and_read_hpm3:
    // Select event code in mhpmevent3 (example event code 0x03; platform-specific)
    li      t0, 0x03                // event ID (platform defined)
    csrw    mhpmevent3, t0

    // Ensure counter is enabled (implementation dependent); read mhpmcounter3
    csrr    a0, mhpmcounter3        // read low 64-bit value on RV64
    ret