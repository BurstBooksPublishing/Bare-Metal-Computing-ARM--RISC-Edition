\begin{figure}[h]
\centering
\begin{lstlisting}[language={RISC-V}]
    # Assume t0 holds user return PC, t1 holds saved sstatus
    ld      ra, 0(sp)           # restore return address
    ld      s0, 8(sp)           # restore callee-saved
    addi    sp, sp, 16          # adjust SP
    csrw    sepc, t0            # program user PC
    csrw    sstatus, t1         # restore sstatus (SPIE->SIE on SRET)
    sfence.vma zero, zero       # ensure TLB coherence
    sret                        # return to user mode