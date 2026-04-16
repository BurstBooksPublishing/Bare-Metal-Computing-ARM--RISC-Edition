ARM (AArch64) and RISC-V Interrupt Entry Assembly Code


.global irq_entry_aarch64
irq_entry_aarch64:
    STP     X29, X30, [SP, #-16]!       // Push FP (X29) and LR (X30)
    MOV     X29, SP                     // Set up new frame pointer
    STP     X19, X20, [SP, #-16]!       // Save callee-saved registers in pairs
    STP     X21, X22, [SP, #-16]!
    STP     X23, X24, [SP, #-16]!
    STP     X25, X26, [SP, #-16]!
    STP     X27, X28, [SP, #-16]!
    // ... handle interrupt ...
    LDP     X27, X28, [SP], #16         // Restore callee-saved registers in reverse order
    LDP     X25, X26, [SP], #16
    LDP     X23, X24, [SP], #16
    LDP     X21, X22, [SP], #16
    LDP     X19, X20, [SP], #16
    LDP     X29, X30, [SP], #16         // Restore FP and LR
    RET                                 // Return via LR

.global irq_entry_rv64
irq_entry_rv64:
    addi    sp, sp, -208                // Allocate stack frame (16-byte aligned)
    sd      ra, 200(sp)                 // Save return address (x1)
    sd      s0, 192(sp)                 // Save s0 (frame pointer)
    sd      s1, 184(sp)
    sd      s2, 176(sp)
    sd      s3, 168(sp)
    sd      s4, 160(sp)
    sd      s5, 152(sp)
    sd      s6, 144(sp)
    sd      s7, 136(sp)
    sd      s8, 128(sp)
    sd      s9, 120(sp)
    sd      s10, 112(sp)
    sd      s11, 104(sp)
    addi    s0, sp, 200                 // Set up new frame pointer
    // ... handle interrupt ...
    ld      s11, 104(sp)                // Restore saved registers in reverse order
    ld      s10, 112(sp)
    ld      s9, 120(sp)
    ld      s8, 128(sp)
    ld      s7, 136(sp)
    ld      s6, 144(sp)
    ld      s5, 152(sp)
    ld      s4, 160(sp)
    ld      s3, 168(sp)
    ld      s2, 176(sp)
    ld      s1, 184(sp)
    ld      s0, 192(sp)
    ld      ra, 200(sp)
    addi    sp, sp, 208                 // Deallocate stack frame
    ret                                 // Return to saved ra