\title{Production-Ready Privilege Verification for ARM (AArch64) and RISC-V (RV64)}

#include <stdint.h>

/* Return codes: bit set indicates specific invariant failure. */

/* ----- AArch64 verification ----- */
int verify_priv_aarch64(uint64_t expect_currentel,
                        uint64_t sctlr_mask, uint64_t sctlr_expect,
                        uint64_t hcr_mask, uint64_t hcr_expect) {
    uint64_t currentel, sctlr, hcr;

    /* Read CurrentEL, SCTLR_EL1, HCR_EL2 using system register access. */
    asm volatile("mrs %0, CurrentEL" : "=r"(currentel));
    asm volatile("mrs %0, SCTLR_EL1" : "=r"(sctlr));
    asm volatile("mrs %0, HCR_EL2"   : "=r"(hcr));

    int rc = 0;
    if ((currentel >> 2) != expect_currentel) rc |= 1; /* Current EL mismatch */
    if ((sctlr & sctlr_mask) != sctlr_expect) rc |= 2; /* SCTLR mismatch */
    if ((hcr & hcr_mask) != hcr_expect)       rc |= 4; /* HCR mismatch */

    return rc;
}

/* ----- RISC-V verification (RV64) ----- */
static inline uint64_t csr_read_mstatus(void) {
    uint64_t v;
    asm volatile("csrr %0, mstatus" : "=r"(v));
    return v;
}

static inline uint64_t csr_read_medeleg(void) {
    uint64_t v;
    asm volatile("csrr %0, medeleg" : "=r"(v));
    return v;
}

static inline uint64_t csr_read_mideleg(void) {
    uint64_t v;
    asm volatile("csrr %0, mideleg" : "=r"(v));
    return v;
}

static inline uint64_t csr_read_satp(void) {
    uint64_t v;
    asm volatile("csrr %0, satp" : "=r"(v));
    return v;
}

/* Concrete reads for portability across toolchains: */
int verify_priv_rv64(uint64_t expect_mstatus_mpp,
                     uint64_t medeleg_mask, uint64_t medeleg_expect,
                     uint64_t mideleg_mask, uint64_t mideleg_expect) {
    uint64_t mstatus = csr_read_mstatus();
    uint64_t medeleg = csr_read_medeleg();
    uint64_t mideleg = csr_read_mideleg();
    uint64_t satp    = csr_read_satp();

    int rc = 0;
    if (((mstatus >> 11) & 0x3) != expect_mstatus_mpp) rc |= 1; /* MPP check */
    if ((medeleg & medeleg_mask) != medeleg_expect)     rc |= 2;
    if ((mideleg & mideleg_mask) != mideleg_expect)     rc |= 4;
    /* If translation expected off, ensure satp==0 */
    if ((medeleg_mask == 0) && (satp != 0))             rc |= 8;

    return rc;
}