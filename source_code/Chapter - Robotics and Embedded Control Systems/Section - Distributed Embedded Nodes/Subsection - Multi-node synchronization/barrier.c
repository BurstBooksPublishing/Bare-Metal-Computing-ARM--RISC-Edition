Distributed Barrier Implementation for ARM (AArch64) and RISC-V


#include <stdint.h>
#include <stdbool.h>

#define MAX_NODES       16
#define TIMEOUT_CYCLES  (1000000ULL) // adjust for network

typedef struct {
    uint8_t  src;
    uint8_t  type;
    uint32_t epoch;
} msg_t;

enum {
    MSG_ANN,
    MSG_ACK
};

static inline uint64_t read_cycles(void) {
#if defined(__aarch64__)
    uint64_t v;
    __asm__ volatile("mrs %0, CNTVCT_EL0" : "=r"(v));
    return v;
#elif defined(__riscv)
    uint64_t v;
    __asm__ volatile("rdcycle %0" : "=r"(v));
    return v;
#else
#error "Unsupported architecture"
#endif
}

/* Wait for messages, collecting types for current epoch. Non-blocking poll wrapper. */
static bool collect_epoch(uint8_t myid, uint32_t epoch, uint8_t n_nodes,
                          bool want_ack, uint8_t seen[MAX_NODES],
                          uint64_t timeout_cycles) {
    uint64_t start = read_cycles();
    for (;;) {
        msg_t m;
        if (poll_msg(&m)) { // platform-provided non-blocking poll
            if (m.epoch == epoch) {
                seen[m.src] = 1;
            }
        }

        // Memory fence to ensure visibility before checking seen[]
#if defined(__aarch64__)
        __asm__ volatile("dmb ish" ::: "memory");
#elif defined(__riscv)
        __asm__ volatile("fence rw, rw" ::: "memory");
#endif

        bool complete = true;
        for (uint8_t i = 0; i < n_nodes; ++i) {
            if (!seen[i]) {
                complete = false;
                break;
            }
        }

        if (complete) {
            return true;
        }

        if ((read_cycles() - start) > timeout_cycles) {
            return false;
        }
    }
}

/* Two-phase barrier: announce, collect, ack, collect. Returns true if success. */
bool distributed_barrier(uint8_t myid, uint8_t n_nodes, uint32_t epoch) {
    msg_t m;
    uint8_t seen[MAX_NODES] = {0};

    // Phase 1: broadcast announcement
    m.src = myid;
    m.type = MSG_ANN;
    m.epoch = epoch;
    if (!send_msg_broadcast(&m)) {
        return false; // platform send
    }

    // Collect all announcements
    seen[myid] = 1; // self
    if (!collect_epoch(myid, epoch, n_nodes, false, seen, TIMEOUT_CYCLES)) {
        return false;
    }

    // Phase 2: broadcast ACK
    m.type = MSG_ACK;
    if (!send_msg_broadcast(&m)) {
        return false;
    }

    // Collect all ACKs
    for (uint8_t i = 0; i < n_nodes; ++i) {
        seen[i] = 0;
    }
    seen[myid] = 1;
    if (!collect_epoch(myid, epoch, n_nodes, true, seen, TIMEOUT_CYCLES)) {
        return false;
    }

    return true;
}