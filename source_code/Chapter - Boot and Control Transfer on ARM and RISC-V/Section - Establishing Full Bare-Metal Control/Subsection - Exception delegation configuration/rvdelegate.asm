\begin{figure}[ht]
\centering
\caption{RISC-V Supervisor Mode Initialization and Exception Delegation Setup}
\end{figure}

.section .text
    .globl _start
_start:
    # -- Build a medeleg mask: delegate selected synchronous traps.
    # Constants chosen: illegal(2), breakpoint(3), instr-page-fault(12),
    # load-page-fault(13), store-page-fault(15)
    li      t0, (1 << 2) | (1 << 3) | (1 << 12) | (1 << 13) | (1 << 15)
    csrw    medeleg, t0                # write delegation mask for synchronous exceptions

    # Optionally verify write
    csrr    t1, medeleg
    bne     t1, t0, .medeleg_failed    # fault if mismatch

    # Prepare supervisor entry point address
    la      t0, supervisor_entry
    csrw    mepc, t0                   # mret will jump to supervisor_entry

    # Clear MPP bits then set MPP = S (01)
    li      t1, (0x3 << 11)            # mask for MPP (bits 12:11)
    csrrc   x0, mstatus, t1            # clear MPP
    li      t1, (0x1 << 11)            # MPP = 01 (S-mode)
    csrrs   x0, mstatus, t1            # set MPP=1

    # Ensure interrupts are masked while transitioning (optional)
    csrci   mstatus, (1 << 3)          # clear MIE

    mret                                # enter S-mode at supervisor_entry

.medeleg_failed:
    # Handle unexpected failure: hang or fall back to diagnostic handler.
    j       .medeleg_failed

    .align  4
supervisor_entry:
    # This code runs in S-mode after delegation. Set up sepc/stvec etc. here.
    # Minimal example: spin forever (replace with real supervisor init).
1:  wfi
    j       1b