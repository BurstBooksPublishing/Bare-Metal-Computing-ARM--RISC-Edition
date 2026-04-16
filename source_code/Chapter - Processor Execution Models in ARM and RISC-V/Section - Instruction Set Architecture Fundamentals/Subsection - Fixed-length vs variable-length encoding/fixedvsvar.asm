AArch64 Assembly Implementation


    .text
    .global sum_a64
    .type sum_a64, %function
sum_a64:
    // x0 = ptr, x1 = len
    mov     x2, x0              // x2 <- ptr
    mov     w0, wzr             // sum = 0 (lower 32 bits)
    cbz     x1, .LendA
.LloopA:
    ldr     w3, [x2], #4        // load 32-bit, post-increment ptr
    add     w0, w0, w3          // sum += w3
    subs    x1, x1, #1
    bne     .LloopA
.LendA:
    ret


RISC-V Assembly Implementation


    .text
    .global sum_rv
    .type sum_rv, @function
sum_rv:
    // a0 = ptr, a1 = len
    mv      t0, a0              # t0 <- ptr
    li      a0, 0               # sum = 0
    beqz    a1, 1f
0:
    lw      t1, 0(t0)           # load word
    add     a0, a0, t1          # sum += t1
    addi    t0, t0, 4           # ptr += 4
    addi    a1, a1, -1
    bnez    a1, 0b
1:
    ret