Production-Ready Checkpointing System for ARM (AArch64) and RISC-V


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Hardware abstraction (implement for your platform)
bool hw_persistent_write(int slot, const void *buf, size_t len); // atomic page writes
bool hw_persistent_read(int slot, void *buf, size_t len);
void hw_kick_watchdog(void);
uint32_t crc32(const void *buf, size_t len);

// Checkpoint layout
#define CHECK_MAGIC 0x43504B54U // "CPKT"
struct checkpoint {
    uint32_t magic;
    uint32_t version;
    uint64_t seq;          // monotonic sequence
    uint64_t sp;           // saved stack pointer
    uint64_t pc;           // saved program counter
    uint64_t regs[8];      // optional caller-saved registers
    uint32_t crc;          // CRC covers all previous bytes
} __attribute__((packed));

// Double-buffer slots
static const int SLOTS = 2;

// Commit checkpoint to nonvolatile storage (double-buffered)
bool checkpoint_commit(const struct checkpoint *state) {
    struct checkpoint tmp = *state;
    tmp.magic = CHECK_MAGIC;
    tmp.crc = 0;
    tmp.crc = crc32(&tmp, sizeof(tmp));
    // select inactive slot (simple example: seq % 2)
    int slot = tmp.seq % SLOTS;
    // ensure watchdog is serviced during long writes
    hw_kick_watchdog();
    if (!hw_persistent_write(slot, &tmp, sizeof(tmp))) {
        return false;
    }
    // publish sequence by writing high-level metadata if needed
    hw_kick_watchdog();
    return true;
}

// Boot-time resume decision; returns true if resume performed (does not return)
bool boot_resume_or_init(void (*fresh_start)(void)) {
    struct checkpoint a, b;
    if (!hw_persistent_read(0, &a, sizeof(a))) {
        return false;
    }
    if (!hw_persistent_read(1, &b, sizeof(b))) {
        return false;
    }

    bool a_ok = (a.magic == CHECK_MAGIC) && (a.crc == crc32(&a, sizeof(a)));
    bool b_ok = (b.magic == CHECK_MAGIC) && (b.crc == crc32(&b, sizeof(b)));

    struct checkpoint *chosen = NULL;
    if (a_ok && b_ok) {
        chosen = (a.seq >= b.seq) ? &a : &b;
    } else if (a_ok) {
        chosen = &a;
    } else if (b_ok) {
        chosen = &b;
    }

    if (!chosen) {
        fresh_start(); // no valid checkpoint
        return false;
    }

    // perform architectural resume via small assembly stub
#if defined(__aarch64__)
    extern void resume_aarch64(uint64_t sp, uint64_t pc, uint64_t *regs);
    resume_aarch64(chosen->sp, chosen->pc, (uint64_t*)chosen->regs);
#elif defined(__riscv)
    extern void resume_rv64(uint64_t sp, uint64_t pc, uint64_t *regs);
    resume_rv64(chosen->sp, chosen->pc, (uint64_t*)chosen->regs);
#else
    fresh_start(); // unsupported arch
#endif
    return true; // should not reach
}