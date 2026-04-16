AArch64 EL1 Vector Handler Prologue


.global vector\_el1
vector\_el1:
    // Create stack frame and save FP/LR for ABI compliance
    stp     x29, x30, [sp, #-16]!
    mov     x29, sp

    // Save a minimal set of callee-saved registers
    stp     x19, x20, [sp, #-16]!
    stp     x21, x22, [sp, #-16]!
    stp     x23, x24, [sp, #-16]!

    // Read syndrome and fault address for the handler
    mrs     x0, ESR\_EL1               // Exception Syndrome Register
    mrs     x1, FAR\_EL1               // Fault Address Register

    // Optionally pass ELR/SPSR to the handler
    mrs     x2, ELR\_EL1               // Exception Link Register (saved PC)
    mrs     x3, SPSR\_EL1              // Saved Processor State

    bl      exception\_c\_handler      // Call C-level handler

    // Restore registers and return from exception
    ldp     x23, x24, [sp], #16
    ldp     x21, x22, [sp], #16
    ldp     x19, x20, [sp], #16
    ldp     x29, x30, [sp], #16
    eret                              // Exception return


RISC-V M-Mode Trap Entry Prologue


.section .text
.global trap\_entry
trap\_entry:
    // Allocate stack frame (128 bytes; adjust based on saved register count)
    addi    sp, sp, -128

    // Save general-purpose callee-saved registers
    sd      ra, 120(sp)     // Return address
    sd      s0, 112(sp)     // Frame pointer
    sd      s1, 104(sp)
    sd      s2, 96(sp)
    sd      s3, 88(sp)
    sd      s4, 80(sp)
    sd      s5, 72(sp)
    sd      s6, 64(sp)
    sd      s7, 56(sp)
    sd      s8, 48(sp)
    sd      s9, 40(sp)
    sd      s10, 32(sp)
    sd      s11, 24(sp)

    // Capture trap-related CSRs
    csrr    t0, mcause      // Trap cause
    csrr    t1, mepc        // Machine exception program counter
    csrr    t2, mtval       // Trap value (e.g., faulting address)

    // Pass trap info to high-level handler via argument registers
    mv      a0, t1          // mepc -> a0
    mv      a1, t0          // mcause -> a1
    mv      a2, t2          // mtval -> a2

    call    handle\_trap

    // Restore saved registers
    ld      ra, 120(sp)
    ld      s0, 112(sp)
    ld      s1, 104(sp)
    ld      s2, 96(sp)
    ld      s3, 88(sp)
    ld      s4, 80(sp)
    ld      s5, 72(sp)
    ld      s6, 64(sp)
    ld      s7, 56(sp)
    ld      s8, 48(sp)
    ld      s9, 40(sp)
    ld      s10, 32(sp)
    ld      s11, 24(sp)

    addi    sp, sp, 128     // Deallocate stack frame
    mret                    // Return from machine-mode trap