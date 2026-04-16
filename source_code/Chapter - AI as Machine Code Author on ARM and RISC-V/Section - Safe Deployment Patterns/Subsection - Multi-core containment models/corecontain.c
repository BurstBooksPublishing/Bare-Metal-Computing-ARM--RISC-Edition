Core Containment and Inter-Processor Communication for ARM (AArch64) and RISC-V


#include <stdatomic.h>
#include <stdint.h>

#define NUM_CORES 4

enum core_cmd {
    CMD_PARK = 0,
    CMD_RUN  = 1,
    CMD_RESET = 2
};

/* Shared command array; use C11 atomics for safe cross-core updates. */
static atomic_int core_cmd[NUM_CORES];

/* Platform-specific: send an inter-processor interrupt to 'core_id'. */
extern void send_ipi(int core_id);

/* Called by secondary cores at early init. 'me' must be retrieved from affinity register. */
void core_containment_entry(int me) {
    for (;;) {
        int cmd = atomic_load_explicit(&core_cmd[me], memory_order_acquire);
        if (cmd == CMD_RUN) {
            return; /* proceed to run untrusted workload or assigned task */
        } else if (cmd == CMD_RESET) {
            /* platform-specific reset sequence; replace with watchdog-triggered reset as needed */
            for (;;) { /* spin; watchdog or monitor will reset */ }
        } else { /* CMD_PARK: enter low-power wait until monitor wakes us */
#if defined(__aarch64__)
            __asm__ volatile("wfe" ::: "memory"); /* wait for event (monitor uses SEV) */
#elif defined(__riscv)
            __asm__ volatile("wfi" ::: "memory"); /* wait for interrupt; monitor should send MSIP */
#else
            __asm__ volatile("" ::: "memory"); /* fallback busy-wait if no arch primitive */
#endif
        }
    }
}

/* Monitor-side operation to park/unpark a core atomically. */
void monitor_set_core_state(int core_id, enum core_cmd new_cmd) {
    atomic_store_explicit(&core_cmd[core_id], new_cmd, memory_order_release);
    /* Wake the core via platform IPI if park->run or when resetting. */
    send_ipi(core_id);
}