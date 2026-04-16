ARM (AArch64) Memory Barrier Implementation



Core synchronization barrier using atomic operations


.global core\_barrier
.type core\_barrier, %function

core\_barrier:
    // x0 = num\_cores (input parameter)
    // Use x1-x3 as temporary registers
    // Use x4 to store local\_sense
    
    // Load current barrier sense value
    adrp x1, \_barrier\_sense
    add x1, x1, :lo12:\_barrier\_sense
    ldar w2, [x1]           // atomic load of barrier\_sense
    and x4, x2, #1          // local\_sense = barrier\_sense & 1
    
    // Increment barrier count atomically
    adrp x1, \_barrier\_count
    add x1, x1, :lo12:\_barrier\_count
    mov w2, #1
    ldaddal w2, w3, [x1]    // atomic fetch add, result in w3
    
    // Check if this is the last core to arrive
    sub w2, w0, #1          // w2 = num\_cores - 1
    cmp w3, w2              // compare previous count with num\_cores - 1
    b.ne 1f                 // branch if not the last core
    
    // Last core: reset count and flip sense
    mov w2, #0
    stlr w2, [x1]           // atomic store of barrier\_count = 0
    
    adrp x1, \_barrier\_sense
    add x1, x1, :lo12:\_barrier\_sense
    eor x2, x4, #1          // flip the sense bit
    stlr w2, [x1]           // atomic store of new barrier\_sense
    b 2f                    // exit
    
1:  // Spin until barrier is released
    adrp x1, \_barrier\_sense
    add x1, x1, :lo12:\_barrier\_sense
    ldar w2, [x1]           // load current barrier\_sense
    and x2, x2, #1
    cmp x2, x4              // compare with local\_sense
    b.eq 1b                 // spin if still equal
    
2:  // Exit
    ret

.size core\_barrier, .-core\_barrier

.global \_barrier\_count
.type \_barrier\_count, %object
\_barrier\_count:
    .word 0
.size \_barrier\_count, 4

.global \_barrier\_sense
.type \_barrier\_sense, %object
\_barrier\_sense:
    .word 0
.size \_barrier\_sense, 4