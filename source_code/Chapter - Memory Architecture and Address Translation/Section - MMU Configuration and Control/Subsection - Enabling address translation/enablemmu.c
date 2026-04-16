//AArch64: Enable MMU and Set System Registers for EL1


#include <stdint.h>

void enable_mmu_aarch64(uint64_t ttbr0_phys)
{
    uint64_t mair = (0xFFUL << 0) | (0x04UL << 8);
    asm volatile("msr mair_el1, %0" :: "r"(mair) : "memory");

    uint64_t tcr = (16 & 0x3FUL) |
                   ((1ULL << 8) | (1ULL << 10)) |
                   ((3ULL << 12)) |
                   ((0ULL << 14));
    asm volatile("msr tcr_el1, %0" :: "r"(tcr) : "memory");

    asm volatile("msr ttbr0_el1, %0" :: "r"(ttbr0_phys) : "memory");

    asm volatile("dsb ish" ::: "memory");
    asm volatile("tlbi alle1" ::: "memory");
    asm volatile("dsb ish" ::: "memory");
    asm volatile("isb" ::: "memory");

    uint64_t sctlr;
    asm volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
    sctlr |= (1UL << 0) | (1UL << 2) | (1UL << 12);
    asm volatile("msr sctlr_el1, %0" :: "r"(sctlr) : "memory");
    asm volatile("isb" ::: "memory");
}


//RISC-V RV64: Enable Paging via SATP and Flush TLBs (Sv39)


#include <stdint.h>

void enable_paging_riscv(uint64_t root_table_phys, uint64_t asid)
{
    const uint64_t MODE_SV39 = 8ULL;
    uint64_t ppn = root_table_phys >> 12;
    uint64_t satp = (MODE_SV39 << 60) |
                    ((asid & 0xFFFFULL) << 44) |
                    (ppn & 0xFFFFFFFFFFFULL);

    asm volatile("csrw satp, %0" :: "r"(satp) : "memory");
    asm volatile("sfence.vma zero, zero" ::: "memory");
}