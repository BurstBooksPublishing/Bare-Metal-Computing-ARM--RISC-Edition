ARM (AArch64) and RISC-V Memory Ordering Examples


.global thread0_start
.global thread1_start

// ARMv8-A: two threads, shared x,y in memory.
thread0_start:
    mov     w0, #1                 // prepare value
    str     w0, [x0]               // store x := 1
    dmb     ish                    // full barrier to ensure visibility
    ldr     w1, [x1]               // r1 := y
    ret

thread1_start:
    mov     w0, #1
    str     w0, [x1]               // store y := 1
    dmb     ish
    ldr     w1, [x0]               // r1 := x
    ret

// Alternate: use release/store and acquire/load
.global thread0_alt_start
.global thread1_alt_start

thread0_alt_start:
    mov     w0, #1
    stlr    w0, [x0]               // store-release x := 1
    ldar    w1, [x1]               // acquire load of y
    ret

thread1_alt_start:
    mov     w0, #1
    stlr    w0, [x1]               // store-release y := 1
    ldar    w1, [x0]               // acquire load of x
    ret

.section .text
.global thread0_rv_start
.global thread1_rv_start

// RISC-V RV64:
thread0_rv_start:
    li      t0, 1
    sd      t0, 0(a0)              // store x := 1
    fence   rw, rw                 // full fence to enforce ordering
    ld      t1, 0(a1)              // t1 := y
    ret

thread1_rv_start:
    li      t0, 1
    sd      t0, 0(a1)              // store y := 1
    fence   rw, rw
    ld      t1, 0(a0)              // t1 := x
    ret

// Alternate: use AMO with aq/rl for release-acquire ordering:
.global thread0_rv_amo_start
.global thread1_rv_amo_start

thread0_rv_amo_start:
    li      t0, 1
    amoswap.d.aq.rl zero, t0, (a0) // atomic store-release via amoswap
    ld      t1, 0(a1)              // load with acquire semantics
    ret

thread1_rv_amo_start:
    li      t0, 1
    amoswap.d.aq.rl zero, t0, (a1)
    ld      t1, 0(a0)
    ret