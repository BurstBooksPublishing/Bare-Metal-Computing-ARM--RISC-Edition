\title{Production-Ready Hand-off Routines for ARM AArch64 and RISC-V}


\InsertInfographic{Images/output/Image31.png}
\section*{AArch64 Hand-off Routine (UEFI)}

#include <stdint.h>

#define EFI_SUCCESS 0
typedef uint64_t efi_status_t;

typedef struct {
    void* map;
    uint64_t map_size;
    uint64_t map_key;
} memmap_t;

/* Platform-specific primitives (implemented elsewhere) */
extern efi_status_t uefi_get_memory_map(memmap_t *mm);
extern efi_status_t uefi_exit_boot_services(uint64_t map_key);
extern void reinit_interrupt_controller(void);
extern void install_exception_vectors(void);
extern void setup_page_tables_and_enable_mmu(void);
extern void platform_cache_flush_and_tlb_invalidate(void);
extern void take_console_from_firmware(void);

/* Hand-off for UEFI on AArch64 */
efi_status_t handoff_from_uefi(void) {
    memmap_t mm;
    efi_status_t st;

    /* loop until ExitBootServices succeeds with a stable map key */
    for (int attempts = 0; attempts < 4; ++attempts) {
        st = uefi_get_memory_map(&mm);
        if (st != EFI_SUCCESS) {
            return st;
        }
        st = uefi_exit_boot_services(mm.map_key);
        if (st == EFI_SUCCESS) {
            break; /* success */
        }
        /* EFI_INVALID_PARAMETER: retry because memory map changed */
    }

    if (st != EFI_SUCCESS) {
        return st;
    }

    /* CPU-level quiesce: disable IRQ/FIQ/SError via DAIF set */
    __asm__ volatile("msr daifset, #0xf" ::: "memory");

    platform_cache_flush_and_tlb_invalidate(); /* sequence-safe */
    install_exception_vectors();
    setup_page_tables_and_enable_mmu(); /* set TTBR */
    reinit_interrupt_controller();
    take_console_from_firmware();

    /* enable interrupts selectively (clear DAIF IRQ/FIQ bits as needed) */
    __asm__ volatile("msr daifclr, #0x6" ::: "memory"); /* enable IRQ+FIQ as chosen */

    return EFI_SUCCESS;
}

\section*{RISC-V Hand-off Routine (OpenSBI)}

/* Simplified RISC-V hand-off: stop using SBI, set stvec and paging, flush */
void handoff_from_sbi(void) {
    /* disable machine interrupts */
    __asm__ volatile("csrci mstatus, 8" ::: "memory");

    install_exception_vectors(); /* writes stvec */
    platform_cache_flush_and_tlb_invalidate(); /* platform-specific cache flush */
    setup_page_tables_and_enable_mmu(); /* write satp and sfence.vma */
    reinit_interrupt_controller(); /* program PLIC/CLINT for runtime */

    /* enable supervisor interrupts if desired */
    __asm__ volatile("csrsi sstatus, 1" ::: "memory"); /* set SIE */
}