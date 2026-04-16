ARM (AArch64) Exception Syndrome Decoder and Handler


#include <stdint.h>
#include <stdbool.h>

// Minimal diagnostics structure.
struct esr_info {
    uint8_t ec;      // Exception Class
    bool il;         // Instruction length (0 = 16-bit, 1 = 32-bit)
    uint32_t iss;    // Instruction Specific Syndrome (25 bits)
    uint8_t dfsc;    // Data/Instruction Fault Status Code (low 6 bits of ISS)
    uint64_t far;    // Fault Address Register (valid for aborts)
};

// Read ESR_EL1 and FAR_EL1 (valid at EL1). Caller runs in exception context.
static inline uint64_t read_esr_el1(void) {
    uint64_t val;
    asm volatile("mrs %0, esr_el1" : "=r"(val));
    return val;
}

static inline uint64_t read_far_el1(void) {
    uint64_t val;
    asm volatile("mrs %0, far_el1" : "=r"(val));
    return val;
}

// Decode ESR into esr_info. Safe shifts/masks for 64-bit values.
static inline struct esr_info decode_esr(uint64_t esr) {
    struct esr_info r;
    r.ec  = (esr >> 26) & 0x3F;                 // 6-bit EC
    r.il  = ((esr >> 25) & 0x1) != 0;           // IL bit
    r.iss = (uint32_t)(esr & ((1ULL << 25) - 1)); // ISS 25 bits
    r.dfsc = (uint8_t)(r.iss & 0x3F);           // low 6 bits common DFSC/IFSC
    r.far = 0;
    return r;
}

// Example handler fragment: classify and act.
void handle_sync_exception_el1(void) {
    uint64_t esr = read_esr_el1();
    struct esr_info info = decode_esr(esr);

    // For data/instruction aborts, read FAR_EL1.
    // EC values for aborts are class-dependent; test generically here.
    // Production code should match EC to abort classes per ARM ARM.
    const uint8_t EC_DATA_ABORT_LOWER_EL = 0x24; // example symbolic use
    const uint8_t EC_INST_ABORT_LOWER_EL = 0x20; // example symbolic use

    if (info.ec == EC_DATA_ABORT_LOWER_EL || info.ec == EC_INST_ABORT_LOWER_EL) {
        info.far = read_far_el1();
        // Dispatch on DFSC: translation vs permission vs alignment.
        switch (info.dfsc) {
        case 0x04: // Example: level 0 translation fault (platform-specific)
        case 0x05: // Example: level 1 translation fault
            // Attempt to populate page tables, then return to faulting PC.
            // (Platform-specific page mapping routine)
            break;
        case 0x0D: // Example: permission fault
            // Log and stop or escalate to higher-level fault manager.
            break;
        default:
            // Unhandled DFSC: produce register dump and halt.
            while (1) asm volatile("wfe");
        }
    } else {
        // Non-abort exceptions: SVC, illegal instruction, etc.
        // Handle or escalate per policy.
    }
}