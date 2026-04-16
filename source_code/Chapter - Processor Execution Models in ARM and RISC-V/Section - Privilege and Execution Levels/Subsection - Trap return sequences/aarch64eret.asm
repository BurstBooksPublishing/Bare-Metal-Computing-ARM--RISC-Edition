AArch64 Exception Return Handler


.global exception_return
exception_return:
    // SP points to saved x19..x30, saved LR in x30_save, return address in x0
    // Restore callee-saved registers (example)
    ldp     x19, x20, [sp], #16        // pop x19, x20
    ldp     x21, x22, [sp], #16        // pop x21, x22
    ldp     x29, x30, [sp], #16        // pop frame pointer, lr
    msr     elr_el1, x0                // set return PC
    // Optionally program a prepared SPSR_EL1 in x1 if altering masks
    // msr     spsr_el1, x1            // (uncomment when handler computes SPSR)
    dsb     sy                         // complete any memory writes (address-space changes)
    isb                                // ensure SPSR/ELR visible to following ERET
    eret                               // return to EL0 using ELR_EL1 / SPSR_EL1