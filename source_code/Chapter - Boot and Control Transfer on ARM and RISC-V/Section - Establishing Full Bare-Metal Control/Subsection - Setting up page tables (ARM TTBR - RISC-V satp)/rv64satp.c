\begin{figure}[ht]
\centering
\begin{lstlisting}[language={[RISC-V]Assembler}]
#include <stdint.h>

#define PTE_V (1ULL << 0)
#define PTE_R (1ULL << 1)
#define PTE_W (1ULL << 2)
#define PTE_X (1ULL << 3)
#define PTE_A (1ULL << 6)

static uint64_t root_table[512] __attribute__((aligned(4096)));

/* Map N 1GiB blocks identity-mapped starting at PA=0 */
void enable_sv39_identity(uint32_t blocks) {
    uint64_t flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_A; // kernel RWX
    for (uint32_t i = 0; i < blocks && i < 512; ++i) {
        uint64_t phys_base = ((uint64_t)i) << 30;            // i * 1GiB
        uint64_t ppn = phys_base >> 12;                      // physical page number
        root_table[i] = (ppn << 10) | flags;                 // PTE: [63:10]=PPN, [9:0]=flags
    }
    uint64_t root_ppn = ((uint64_t)root_table) >> 12;
    uint64_t satp_val = (8ULL << 60) | (0ULL << 44) | root_ppn; // mode=8(Sv39), ASID=0

    __asm__ volatile("sfence.vma zero, zero" ::: "memory");      // ensure clean state
    __asm__ volatile("csrw satp, %0" :: "r"(satp_val));         // write satp
    __asm__ volatile("sfence.vma zero, zero" ::: "memory");     // sync new translations
    __asm__ volatile("fence.i" ::: "memory");                   // conservative sync
}