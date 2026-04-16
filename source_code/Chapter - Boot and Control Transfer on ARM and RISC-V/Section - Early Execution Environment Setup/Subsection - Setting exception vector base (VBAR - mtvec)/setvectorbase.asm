AArch64 and RISC-V Exception Vector Setup and Handler Example


    .global setup_exception_vector_aarch64
    .global setup_exception_vector_riscv

setup_exception_vector_aarch64:
    // x0 contains the base address of the vector table
    msr VBAR_EL1, x0         // Set the vector base address register
    dsb sy                   // Ensure memory writes complete before continuing
    isb                      // Flush the pipeline to use updated vectors immediately
    ret

setup_exception_vector_riscv:
    // t0 = base address (must be 4-byte aligned), t1 = mode (0 or 1)
    or t2, t0, t1            // Combine base and mode into mtvec format
    csrw mtvec, t2           // Write to the machine trap-vector base register
    fence.i                  // Ensure instruction stream sees updated handlers
    ret

// Minimal RISC-V trap handler stub (placed at vector base in direct mode)
trap_handler_stub:
    addi sp, sp, -16         // Allocate stack space
    sd ra, 8(sp)             // Save return address
    sd t0, 0(sp)             // Save temporary register

    csrr t0, mcause          // Read the cause of the trap
    // Dispatch logic would go here based on mcause value

    ld t0, 0(sp)             // Restore temporary register
    ld ra, 8(sp)             // Restore return address
    addi sp, sp, 16          // Deallocate stack space
    mret                     // Return from trap handler