//RISC-V SV39 Virtual Memory Configuration


.section .text
.global configure_sv39_paging

configure_sv39_paging:
    // Construct SATP value: MODE_SV39 in upper 4 bits, ASID in bits 59:44, PPN in lower 44 bits
    li      t0, 8           // MODE_SV39 = 8 (b1000)
    slli    t0, t0, 60      // Position mode field in bits 63:60
    slli    a1, a1, 44      // Position ASID in bits 59:44
    or      t0, t0, a1      // Combine mode and ASID
    or      t0, t0, a2      // Combine with page root PPN (bits 43:0)

    // Install new page table and ASID
    csrw    satp, t0

    // Flush all TLB entries globally
    sfence.vma x0, x0

    // Ensure instruction stream coherence
    fence.i

    ret