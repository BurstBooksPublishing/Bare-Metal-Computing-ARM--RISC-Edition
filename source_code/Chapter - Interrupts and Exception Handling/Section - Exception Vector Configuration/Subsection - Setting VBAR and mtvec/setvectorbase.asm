/* ARM (AArch64) */
    .global set_vbar_el1
    .type   set_vbar_el1, @function

set_vbar_el1:
    // caller guarantees alignment and EL privileges
    msr     VBAR_EL1, x0        // write vector base
    dsb     sy                  // ensure prior memory accesses complete
    isb                         // synchronize instruction stream
    ret

/* RISC-V */
    .global set_mtvec_direct
    .type   set_mtvec_direct, @function

set_mtvec_direct:
    // clear low two bits to ensure MODE=0 (direct)
    li      t0, -4              // mask for low two bits
    and     t0, a0, t0          // BASE = a0 & ~0x3
    csrw    mtvec, t0           // write mtvec = BASE | MODE(0)
    fence.i                     // ensure I-cache sees new handler code
    ret