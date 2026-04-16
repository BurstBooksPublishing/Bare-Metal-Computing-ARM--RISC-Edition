ARM (AArch64) Exception Entry and Return with MMU Enable


.global trap_entry_aarch64
trap_entry_aarch64:
    mrs x0, spsr_el1            // save processor state
    mrs x1, elr_el1             // save return PC
    mrs x2, esr_el1             // read exception syndrome
    mrs x3, far_el1             // read faulting address (FAR)
    stp x0, x1, [sp, #-16]!     // push spsr_el1, elr_el1

    // ... handle exception, possibly modify sctlr_el1/mmu bits ...

    ldr x0, =0x400000           // example: new TTBR0 base (physical)
    msr ttbr0_el1, x0           // set page-table root
    dsb ish                     // ensure translation state visible
    isb                         // synchronize instruction stream

    mrs x4, sctlr_el1
    orr x4, x4, #1              // set M bit to enable MMU
    msr sctlr_el1, x4

    // ... return: restore state
    ldp x0, x1, [sp], #16
    msr elr_el1, x1
    msr spsr_el1, x0
    eret                        // return from exception


RISC-V Trap Entry and Return with Sv39 MMU Enable


.section .text
.global trap_entry_riscv
trap_entry_riscv:
    csrr t0, mstatus           # save mstatus
    csrr t1, mepc              # save return PC
    csrr t2, mcause            # syndrome
    csrr t3, mtval             # faulting address or additional info
    addi sp, sp, -16
    sd t0, 0(sp)               # push mstatus
    sd t1, 8(sp)               # push mepc

    // Example: set satp to enable Sv39 (mode=8), with ASID=0 and PPN in a0
    // a0 must contain physical root page frame number >> 12
    li t4, 0x8000000000000000  # compose satp value: (mode<<60) | ppn
    or t4, t4, a0
    csrw satp, t4
    fence.i                    # ensure TLB/ICache coherence if required

    // ... handle trap, restore
    ld t0, 0(sp)
    ld t1, 8(sp)
    addi sp, sp, 16
    csrw mepc, t1
    csrw mstatus, t0
    mret                       # return from trap