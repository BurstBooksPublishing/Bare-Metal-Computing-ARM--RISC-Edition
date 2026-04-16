\begin{figure}[h]
\centering
\caption{RISC-V 64-bit Critical Section Implementation}
\end{figure}

.text
.global critical_section
critical_section:
    li      t2, 8                   # mask for MIE (bit 3)
    csrrc   t1, mstatus, t2         # t1 := old mstatus; clear MIE atomically
    fence   iorw, iorw              # order memory/device accesses
    // -- critical protected work --
    call    do_critical_work
    // -- end protected work --
    andi    t3, t1, 8               # extract saved MIE bit
    beqz    t3, 1f                  # if previously cleared, skip restore
    csrs    mstatus, t2             # restore MIE
1:
    ret