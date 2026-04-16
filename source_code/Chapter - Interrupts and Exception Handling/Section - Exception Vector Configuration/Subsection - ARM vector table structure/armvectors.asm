//AArch64 Exception Vector Table and Common Handler


.section .vectors, "ax"
    .globl vector_table
    .align 7                  // 2^7 = 128-byte alignment
vector_table:
    // Group 0: current EL, using SP_EL0
    b el_sync_curr_sp0        // 0x000
    .balign 0x20
    b el_irq_curr_sp0         // 0x020
    .balign 0x20
    b el_fiq_curr_sp0         // 0x040
    .balign 0x20
    b el_serr_curr_sp0        // 0x060
    .balign 128               // advance to next 0x80 boundary

    // Group 1: current EL, using SP_ELx
    b el_sync_curr_spx        // 0x080
    .balign 0x20
    b el_irq_curr_spx         // 0x0A0
    .balign 0x20
    b el_fiq_curr_spx         // 0x0C0
    .balign 0x20
    b el_serr_curr_spx        // 0x0E0
    .balign 128

    // Group 2: lower EL, AArch64 state
    b el_sync_lower_a64       // 0x100
    .balign 0x20
    b el_irq_lower_a64        // 0x120
    .balign 0x20
    b el_fiq_lower_a64        // 0x140
    .balign 0x20
    b el_serr_lower_a64       // 0x160
    .balign 128

    // Group 3: lower EL, AArch32 state
    b el_sync_lower_a32       // 0x180
    .balign 0x20
    b el_irq_lower_a32        // 0x1A0
    .balign 0x20
    b el_fiq_lower_a32        // 0x1C0
    .balign 0x20
    b el_serr_lower_a32       // 0x1E0
    .balign 0x20

    // Per-vector labels branch to a common prologue.
el_sync_curr_sp0:    b exception_common
el_irq_curr_sp0:     b exception_common
el_fiq_curr_sp0:     b exception_common
el_serr_curr_sp0:    b exception_common
el_sync_curr_spx:    b exception_common
el_irq_curr_spx:     b exception_common
el_fiq_curr_spx:     b exception_common
el_serr_curr_spx:    b exception_common
el_sync_lower_a64:   b exception_common
el_irq_lower_a64:    b exception_common
el_fiq_lower_a64:    b exception_common
el_serr_lower_a64:   b exception_common
el_sync_lower_a32:   b exception_common
el_irq_lower_a32:    b exception_common
el_fiq_lower_a32:    b exception_common
el_serr_lower_a32:   b exception_common

    .align 4
exception_common:
    // Minimal, safe prologue: save frame pointer and LR, set up frame.
    stp x29, x30, [sp, #-16]!    // push x29/x30
    mov x29, sp
    // Save additional registers here as required by your context-save policy.
    bl c_exception_dispatch     // call C handler (must preserve ABI)
    // Restore and return to exception return instruction sequence.
    ldp x29, x30, [sp], #16     // pop x29/x30
    // c_exception_dispatch should prepare ELR and SPSR appropriately,
    // then execute 'eret' to return from exception.
    eret