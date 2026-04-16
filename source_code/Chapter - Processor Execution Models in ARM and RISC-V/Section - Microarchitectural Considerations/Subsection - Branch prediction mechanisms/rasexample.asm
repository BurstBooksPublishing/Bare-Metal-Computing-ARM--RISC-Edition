ARM (AArch64) and RISC-V Function Call Conventions


    // AArch64: use BL / RET to keep RAS coherent
    // Caller:
    .text
    .global caller_aarch64
caller_aarch64:
    // prepare args in x0, x1...
    bl callee_aarch64      // link register x30 pushed to RAS implicitly
    // continue...
    ret

    // Callee:
callee_aarch64:
    stp x29, x30, [sp, #-16]! // standard frame (optional)
    mov x29, sp
    // body...
    ldp x29, x30, [sp], #16
    ret                      // RAS pop provides predicted target

    // RISC-V (RV64): use jal / jalr (ret = jalr x0, x1, 0) with ra in x1
    .text
    .global caller_rv64
caller_rv64:
    // set up arguments in a0, a1...
    jal ra, callee_rv64      // saves return PC to ra
    // continue...
    ret                      // glibc uses jalr x0, ra, 0

callee_rv64:
    addi sp, sp, -16         // optional frame
    sd ra, 8(sp)
    // body...
    ld ra, 8(sp)
    addi sp, sp, 16
    ret