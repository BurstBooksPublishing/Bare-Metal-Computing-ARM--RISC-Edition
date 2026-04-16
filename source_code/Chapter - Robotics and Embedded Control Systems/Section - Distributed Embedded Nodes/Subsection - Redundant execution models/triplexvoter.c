Triplex Voting Implementation for ARM (AArch64) and RISC-V Systems


#include <stdint.h>

#define N_PEERS 3
#define TIMEOUT_TICKS 1000

volatile struct {
    uint32_t seq;
    uint32_t crc;
    uint32_t cmd;
    uint32_t reserved;
} peer_slots[N_PEERS];

static inline void mem_barrier(void) {
#if defined(__aarch64__)
    __asm__ volatile("dsb ish" ::: "memory");
#elif defined(__riscv)
    __asm__ volatile("fence rw, rw" ::: "memory");
#else
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
#endif
}

int triplex_vote(uint32_t seq, uint32_t *out_cmd, uint32_t deadline_tick) {
    uint32_t counts[3] = {0};
    uint32_t values[N_PEERS];
    int seen = 0;

    while (1) {
        mem_barrier();
        for (int i = 0; i < N_PEERS; ++i) {
            uint32_t s = peer_slots[i].seq;
            if (s != seq) continue;
            uint32_t c = peer_slots[i].crc;
            uint32_t v = peer_slots[i].cmd;
            if (!crc32_verify(&v, sizeof(v), c)) continue;
            values[seen++] = v;
        }
        if (seen == N_PEERS) break;
        if (ticks_now() > deadline_tick) return -1;
        cpu_relax();
    }

    for (int i = 0; i < N_PEERS; ++i) {
        int matches = 1;
        for (int j = 0; j < N_PEERS; ++j) {
            if (i != j && values[i] == values[j]) {
                matches++;
            }
        }
        if (matches >= 2) {
            *out_cmd = values[i];
            return 0;
        }
    }
    return -2;
}