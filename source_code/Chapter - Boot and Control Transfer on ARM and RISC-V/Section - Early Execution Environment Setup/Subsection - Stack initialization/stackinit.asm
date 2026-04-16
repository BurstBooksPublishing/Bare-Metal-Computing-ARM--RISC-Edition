ARM AArch64 and RISC-V Boot Stack Initialization


    .globl _start
_start:
    adrp    x0, __stack_top
    add     x0, x0, :lo12:__stack_top
    add     x0, x0, #15
    bic     x0, x0, #15
    mov     sp, x0

    b       1f

    .section .text.init
    .globl _start_riscv
_start_riscv:
    csrr    a0, mhartid
    la      a1, stack_region_base
    li      a2, 65536
    mul     a3, a0, a2
    add     a1, a1, a3
    addi    a1, a1, 65536
    addi    a1, a1, -16
    andi    sp, a1, -16
1: