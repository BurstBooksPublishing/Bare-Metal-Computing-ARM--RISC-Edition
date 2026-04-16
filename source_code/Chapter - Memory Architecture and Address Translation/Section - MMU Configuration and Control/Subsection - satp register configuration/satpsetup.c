//RISC-V SV39/SV48 Page Table Activation with TLB Flush


#include <stdint.h>
#include <stdbool.h>

#define SATP_MODE_SV39 8U
#define SATP_MODE_SV48 9U

// Write satp CSR and perform a full TLB flush.
// root_pa must be 4 KiB aligned; asid is 16-bit; mode is SATP_MODE_SV39 or SATP_MODE_SV48.
static inline bool set_satp(uint64_t root_pa, uint16_t asid, uint8_t mode)
{
    if ((root_pa & 0xFFFULL) != 0) 
        return false;            // alignment check
    if (!(mode == SATP_MODE_SV39 || mode == SATP_MODE_SV48)) 
        return false;            // mode check

    uint64_t ppn = root_pa >> 12;                           // eq. (2)
    uint64_t satp = ((uint64_t)mode << 60) | 
                    ((uint64_t)asid << 44) | 
                    (ppn & 0xFFFFFFFFFFFULL);

    // CSR write: atomic assignment visible to hardware; 'memory' clobber prevents reorder.
    asm volatile("csrw satp, %0" :: "r"(satp) : "memory");
    // Ensure previous memory writes (page-table initialization) are visible before enabling SATP.
    asm volatile("sfence.vma x0, x0" ::: "memory");         // full TLB flush after change
    return true;
}