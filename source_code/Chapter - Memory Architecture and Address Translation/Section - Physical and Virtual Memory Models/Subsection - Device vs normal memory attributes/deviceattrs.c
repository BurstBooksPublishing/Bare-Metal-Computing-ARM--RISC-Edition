ARM (AArch64) and RISC-V Memory Mapping Configuration


.global write_mair_el1
write_mair_el1:
    msr mair_el1, x0
    isb
    ret

.global arm_setup_device_mapping
arm_setup_device_mapping:
    // x0 = ttb_l1, x1 = va, x2 = pa

    // Compose MAIR: byte0 = normal, byte1 = device
    mov x3, #0xff
    mov x4, #0x04
    orr x3, x3, x4, lsl #8
    bl write_mair_el1

    // Create level-1 block descriptor
    and x5, x2, #0xFFFFF000        // output address bits
    mov x6, #1
    orr x5, x5, x6, lsl #2         // AttrIndx = 1
    orr x5, x5, x6                 // descriptor type: block
    // AP field assumed zeroed; set as required in real usage

    // Index into L1 table
    lsr x7, x1, #20
    and x7, x7, #0xFFF
    add x7, x0, x7, lsl #3
    str x5, [x7]

    ret

.global riscv_program_pmp_and_map
riscv_program_pmp_and_map:
    // x0 = paddr, x1 = size, x2 = pt_root, x3 = vaddr

    // 1) Program PMP entry 0 using NAPOT encoding
    add x4, x0, x1, lsr #1         // paddr + size/2
    lsr x4, x4, #2                 // napot_addr
    csrw pmpaddr0, x4

    // PMP config: R/W=1, X=0, A=NAPOT(0b10), L=0
    li x5, (0b10 << 3) | (1 << 1) | (1 << 0)
    csrw pmpcfg0, x5

    // 2) Create SV39 leaf PTEs
    add x6, x1, #0xFFF
    lsr x6, x6, #12                // npages = (size + 0xFFF) >> 12
    li x7, #0                      // i = 0

loop:
    bge x7, x6, done

    // pfn = (paddr >> 12) + i
    lsr x8, x0, #12
    add x8, x8, x7

    // pte = (pfn << 10) | R | W | A
    sll x9, x8, #10
    ori x9, x9, (1 << 1) | (1 << 2) | (1 << 6)

    // pt_root[(vaddr >> 12 & 0x1FF) + i] = pte
    lsr x10, x3, #12
    and x10, x10, #0x1FF
    add x10, x10, x7
    sll x10, x10, #3               // multiply by 8 for 64-bit entries
    add x11, x2, x10
    sd x9, (x11)

    addi x7, x7, #1
    j loop

done:
    sfence.vma
    ret