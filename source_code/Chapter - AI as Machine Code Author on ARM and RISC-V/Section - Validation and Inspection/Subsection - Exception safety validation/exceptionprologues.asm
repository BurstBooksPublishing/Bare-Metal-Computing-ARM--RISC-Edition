ARM (AArch64) Exception Entry and RISC-V Trap Handler Assembly Code


    .section .text
    .global exception_entry_aarch64
exception_entry_aarch64:
    // allocate aligned frame and save callee-saved regs
    sub     sp, sp, #48              // 16-byte aligned frame
    stp     x19, x20, [sp, #0]
    stp     x21, x22, [sp, #16]
    stp     x29, x30, [sp, #32]      // frame pointer and lr

    // capture syndrome registers into x0..x2 (C ABI: args)
    mrs     x0, esr_el1              // syndrome
    mrs     x1, elr_el1              // return address
    mrs     x2, spsr_el1             // saved pstate

    // call C validator: handle_exception_aarch64(esr, elr, spsr)
    bl      handle_exception_aarch64

    // restore regs and return to saved EL
    ldp     x29, x30, [sp, #32]
    ldp     x21, x22, [sp, #16]
    ldp     x19, x20, [sp, #0]
    add     sp, sp, #48
    eret                             // return to interrupted context

    .section .text
    .global trap_handler_riscv
trap_handler_riscv:
    addi    sp, sp, -64              # allocate stack frame
    sd      ra, 56(sp)               # save caller-saved return address
    sd      s0, 48(sp)               # save callee-saved registers
    sd      s1, 40(sp)

    csrr    t0, mcause               # cause -> t0
    csrr    t1, mepc                 # exception PC -> t1
    csrr    t2, mtval                # faulting address/val -> t2

    // call C validator: handle_trap_riscv(mcause, mepc, mtval)
    call    handle_trap_riscv

    ld      s1, 40(sp)               # restore callee-saved regs
    ld      s0, 48(sp)
    ld      ra, 56(sp)
    addi    sp, sp, 64               # free frame
    mret                             # resume interrupted context