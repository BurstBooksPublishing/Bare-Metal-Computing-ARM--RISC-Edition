\begin{figure}[ht]
\centering
\caption{High-quality C11 implementation of a ticket lock for ARM (AArch64) and RISC-V systems}
\end{figure}

#include <stdatomic.h>
#include <stdint.h>

typedef struct {
    atomic_uint_least64_t next;  // ticket allocator
    atomic_uint_least64_t owner; // current serving ticket
} ticket_lock_t;

static inline void lock_init(ticket_lock_t *l) {
    atomic_init(&l->next, 0);
    atomic_init(&l->owner, 0);
}

static inline void lock_acquire(ticket_lock_t *l) {
    // fetch-and-increment gives a unique ticket (relaxed for alloc)
    uint64_t ticket = atomic_fetch_add_explicit(&l->next, 1,
                                               memory_order_relaxed);
    // spin until owner equals our ticket; use acquire to synchronize
    while (atomic_load_explicit(&l->owner, memory_order_acquire) != ticket) {
        // optional: PAUSE/YIELD instruction to reduce power/coherence noise
#if defined(__aarch64__)
        __asm__ volatile("yield" ::: "memory");
#elif defined(__riscv)
        __asm__ volatile("wfi" ::: "memory"); // or 'nop' if WFI inappropriate
#else
        __asm__ volatile("pause" ::: "memory");
#endif
    }
    // critical section begins: acquire semantics ensured
}

static inline void lock_release(ticket_lock_t *l) {
    // increment owner with release semantics to publish writes
    uint64_t next_owner = atomic_load_explicit(&l->owner, memory_order_relaxed) + 1;
    atomic_store_explicit(&l->owner, next_owner, memory_order_release);
}

static inline int lock_try_acquire(ticket_lock_t *l) {
    uint64_t cur_owner = atomic_load_explicit(&l->owner, memory_order_relaxed);
    uint64_t next = cur_owner + 1;
    // try to atomically take the next ticket only if next==current next allocator
    uint64_t expected = cur_owner;
    // compare_exchange the 'next' counter is not used here; try simple fast-path:
    if (atomic_compare_exchange_strong_explicit(&l->next, &expected, next,
                                                memory_order_acq_rel,
                                                memory_order_relaxed)) {
        // we obtained the ticket and are owner immediately
        return 1;
    }
    return 0; // failed
}