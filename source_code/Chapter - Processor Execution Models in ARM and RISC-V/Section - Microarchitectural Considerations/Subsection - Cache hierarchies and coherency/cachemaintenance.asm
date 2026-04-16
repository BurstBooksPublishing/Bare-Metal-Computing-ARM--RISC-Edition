ARM (AArch64) Cache Line Cleaning Routine


.text
    .global cache_clean_range

cache_clean_range:
    // x0 = start address, x1 = size in bytes
    cbz     x1, .done                 // nothing to do

    // compute end = start + size - 1
    add     x2, x0, x1
    sub     x2, x2, #1

    // get cache line size into x3 (typically 64 bytes on modern ARM)
    mov     x3, #64

    // align start address down to nearest cache line boundary
    and     x0, x0, #(~63)

.align_loop:
    // if current address > end address, exit loop
    cmp     x0, x2
    b.gt    .fence_and_exit

    // clean data cache line by virtual address to PoC
    dc      cvac, x0

    // advance to next cache line
    add     x0, x0, x3
    b       .align_loop

.fence_and_exit:
    dsb     sy                        // ensure all clean operations complete
    isb                           // instruction synchronization barrier
    ret

.done:
    ret