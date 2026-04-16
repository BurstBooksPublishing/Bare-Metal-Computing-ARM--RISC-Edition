.global max_aarch64
.global max_rv64_branch
.global max_rv64_cmov

// ARM AArch64: X0 = max(X0, X1) using flags and conditional select
// Assumes signed 64-bit integers in X0, X1.
max_aarch64:
    subs    x2, x0, x1       // X2 = X0 - X1, updates NZCV
    csel    x0, x0, x1, ge   // if (X0 >= X1) X0=X0 else X0=X1

    // Read NZCV into a general register (if needed)
    mrs     x3, nzcv         // X3 receives NZCV bits for diagnostics
    ret

// RISC-V RV64: x10 = max(x10, x11) without flags
// Assumes signed 64-bit integers in x10, x11.
max_rv64_branch:
    blt     x10, x11, 1f     # if x10 < x11 jump
    j       2f
1:  mv      x10, x11         # x10 = x11
2:  ret

// RISC-V alternative using slt and conditional move idiom
max_rv64_cmov:
    slt     x12, x10, x11    # x12 = (x10 < x11) ? 1 : 0
    beq     x12, x0, 3f      # if not less, skip
    mv      x10, x11
3:  ret