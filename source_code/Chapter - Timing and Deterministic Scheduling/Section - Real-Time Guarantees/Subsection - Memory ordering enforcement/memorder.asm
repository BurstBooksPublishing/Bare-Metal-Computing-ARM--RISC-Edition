AArch64 Memory Ordering for Device Doorbell Synchronization


.global producer_aarch64
.global consumer_aarch64

producer_aarch64:
    // X0 = address of data, X1 = data value, X2 = doorbell MMIO
    str     x1, [x0]           // regular store of data
    stlr    x1, [x0]           // store-release alternative for normal memory
    dsb     ish                // ensure previous stores complete globally
    str     x1, [x2]           // MMIO doorbell write (device expects ordered write)
    ret

consumer_aarch64:
    ldr     x3, [x2]           // read doorbell (MMIO)
    dsb     ish                // ensure subsequent loads see ordered data
    ldr     x4, [x0]           // read data, now synchronized
    ret


RISC-V Memory Ordering for Device Doorbell Synchronization


.section .text
.globl producer_riscv
.globl consumer_riscv

producer_riscv:
    # x10 = data addr, x11 = data, x12 = doorbell addr
    sw      x11, 0(x10)        # store data
    fence   w, w               # ensure prior stores complete before subsequent writes
    sw      x11, 0(x12)        # MMIO doorbell write

    # OR using AMO for ordered release (if available)
    # amoadd.w.aq x0, x11, (x10) # atomic add with acquire semantics (example of aq/rl use)
    ret

consumer_riscv:
    lw      x13, 0(x12)        # read doorbell
    fence   r, r               # ensure subsequent reads see ordered data
    lw      x14, 0(x10)        # read data
    ret