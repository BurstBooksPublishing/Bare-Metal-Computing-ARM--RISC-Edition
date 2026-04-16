\title{Production-Ready Crash Report Finalization for ARM (AArch64) and RISC-V}
\caption{Finalizes a crash report by populating header fields, computing CRC32, and ensuring memory visibility across architectures.}

#include <stddef.h>
#include <stdint.h>

#define CRASH_MAGIC 0x43525348u  /* "CRSH" */
#define CRASH_HDR_OFF_CRC 16     /* CRC32 offset in header (bytes) */

static uint32_t crc32_bitwise(const uint8_t *data, size_t len)
{
    /* CRC-32/IEEE 802.3 bitwise implementation (small, robust in panic state). */
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint32_t)data[i];
        for (int b = 0; b < 8; ++b) {
            crc = (crc & 1u) ? (0xEDB88320u ^ (crc >> 1)) : (crc >> 1);
        }
    }
    return ~crc;
}

/* finalize_crash_report:
 *  - buf: pointer to reserved buffer populated by exception entry.
 *  - buf_size: total buffer capacity in bytes.
 *  - record_len: actual bytes used by this record (<= buf_size).
 *  - emit: optional platform emitter callback (may be NULL).
 */
void finalize_crash_report(uint8_t *buf, size_t buf_size,
                           size_t record_len,
                           void (*emit)(const uint8_t*, size_t))
{
    if (!buf || record_len < (CRASH_HDR_OFF_CRC + 4) || record_len > buf_size)
        return; /* defensive, cannot report */

    /* Populate header magic and length (assumes caller reserved header area). */
    uint32_t *magic = (uint32_t*)(void*)(buf + 0);
    uint32_t *length = (uint32_t*)(void*)(buf + 8);
    *magic = CRASH_MAGIC;
    *length = (uint32_t)record_len;

    /* Zero CRC field prior to computation. */
    uint32_t *crc_field = (uint32_t*)(void*)(buf + CRASH_HDR_OFF_CRC);
    *crc_field = 0;

    /* Compute CRC over record */
    uint32_t crc = crc32_bitwise(buf, record_len);
    *crc_field = crc;

    /* Ensure visibility to other agents / after reset.
       Use architecture-specific barriers to make stores durable/ordered. */
#if defined(__aarch64__)
    __asm__ volatile("dsb sy\n" "isb\n" ::: "memory");
#elif defined(__riscv)
    __asm__ volatile("fence" ::: "memory");
#else
    __asm__ volatile("" ::: "memory");
#endif

    /* Optionally emit the report via platform transport for immediate telemetry. */
    if (emit) emit(buf, record_len);
}