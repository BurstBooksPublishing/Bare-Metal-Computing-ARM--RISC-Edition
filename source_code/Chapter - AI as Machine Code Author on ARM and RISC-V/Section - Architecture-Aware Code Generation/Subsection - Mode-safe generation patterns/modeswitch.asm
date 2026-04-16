\begin{figure}[ht]
\centering

/* ARM (AArch64): EL1 context prepares a drop to EL0.
       Preconditions: x0=user_sp (aligned), x1=user_pc. Running at EL1. */
    msr     sp_el0, x0          // set user-mode stack pointer bank
    mov     x2, #0              // desired SPSR_EL1 value (PSTATE mask as configured)
    msr     spsr_el1, x2        // set PSTATE to restore on eret
    msr     elr_el1, x1         // return PC
    eret                        // return to EL0 (restores PSTATE from SPSR_EL1)

/* RISC-V: M-mode prepares return to S-mode.
       Preconditions: a0=user_sp (not written to CSR here), a1=user_pc. Running at M-mode.
       Note: MPP shift per spec is 11. */
    csrw    mepc, a1            // set return PC
    csrr    t0, mstatus         // read mstatus
    li      t1, 1               // S-mode encoding for MPP (01)
    li      t2, 11              // MPP shift
    sll     t1, t1, t2          // t1 = (1 << MPP_SHIFT)
    li      t3, 3               // mask width for MPP (two bits)
    sll     t3, t3, t2          // t3 = mask << MPP_SHIFT
    not     t3, t3              // invert mask to clear MPP bits
    and     t0, t0, t3          // clear old MPP
    or      t0, t0, t1          // set MPP to S-mode
    csrw    mstatus, t0         // write back mstatus
    mret                        // return to S-mode (mepc -> pc, mstatus.MPP -> new mode)

\end{figure}