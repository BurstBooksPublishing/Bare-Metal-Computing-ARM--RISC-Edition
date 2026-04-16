.section .text
.global riscv_switch_satp
.func riscv_switch_satp

riscv_switch_satp:
    // write satp then flush TLB on this hart
    csrw satp, a0           // set new page-table base and ASID
    sfence.vma x0, x0       // flush entire TLB on this hart
    fence                   // ensure ordering if required by SW model
    ret

.endfunc