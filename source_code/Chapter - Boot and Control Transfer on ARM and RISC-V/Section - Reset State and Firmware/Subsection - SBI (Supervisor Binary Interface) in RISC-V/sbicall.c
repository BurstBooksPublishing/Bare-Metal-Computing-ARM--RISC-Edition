RISC-V SBI Call Implementation

#include <stdint.h>

/* SBI call primitive: extension in ext, function in fid, up to 6 args */
static inline long sbi_call(uint64_t ext, uint64_t fid,
                            uint64_t arg0, uint64_t arg1, uint64_t arg2,
                            uint64_t arg3, uint64_t arg4, uint64_t arg5,
                            uint64_t *out_val)
{
    register uint64_t a0 asm("a0") = arg0;
    register uint64_t a1 asm("a1") = arg1;
    register uint64_t a2 asm("a2") = arg2;
    register uint64_t a3 asm("a3") = arg3;
    register uint64_t a4 asm("a4") = arg4;
    register uint64_t a5 asm("a5") = arg5;
    register uint64_t a6 asm("a6") = fid;  /* function id */
    register uint64_t a7 asm("a7") = ext;  /* extension id */

    asm volatile (
        "ecall"
        : "+r"(a0), "+r"(a1)
        : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
        : "memory"
    );

    if (out_val) *out_val = a1; /* return value (if any) */
    return (long)a0;            /* error code */
}

/* Example extension/function identifiers (replace with platform headers) */
enum {
    SBI_EXT_BASE = 0x10,   /* placeholder: base extension */
    SBI_EXT_CONSOLE = 0x1, /* placeholder */
    SBI_EXT_HSM = 0x2      /* hart state management placeholder */
};

/* High-level helpers */
static inline int sbi_console_putchar(int ch)
{
    uint64_t val;
    long err = sbi_call(SBI_EXT_CONSOLE, 0 /* console_putchar */,
                        (uint64_t)ch, 0, 0, 0, 0, 0, &val);
    return (err == 0) ? 0 : -1;
}

static inline int sbi_hart_start(uint64_t hartid, uint64_t start_addr, uint64_t opaque)
{
    long err = sbi_call(SBI_EXT_HSM, 0 /* hart_start */,
                        hartid, start_addr, opaque, 0, 0, 0, NULL);
    return (err == 0) ? 0 : -1;
}