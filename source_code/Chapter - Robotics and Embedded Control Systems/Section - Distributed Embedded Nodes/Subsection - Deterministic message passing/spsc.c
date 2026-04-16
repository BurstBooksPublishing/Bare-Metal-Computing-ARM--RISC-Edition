High-Performance SPSC Ring Buffer Implementation for ARM (AArch64) and RISC-V


#include <stdatomic.h>
#include <stdint.h>

static inline void platform_fence(void) {
#if defined(__aarch64__)
    __asm__ volatile("dmb ish" ::: "memory");
#elif defined(__riscv)
    __asm__ volatile("fence rw, rw" ::: "memory");
#else
    atomic_thread_fence(memory_order_seq_cst);
#endif
}

#define CACHE_LINE_ALIGN __attribute__((aligned(64)))

typedef struct CACHE_LINE_ALIGN {
    uint64_t msg_id;
    uint64_t timestamp;
    uint8_t payload[48];
} msg_t;

typedef struct CACHE_LINE_ALIGN {
    atomic_uint_fast32_t head;
    atomic_uint_fast32_t tail;
    uint32_t capacity;
    msg_t buffer[];
} spsc_ring_t;

static inline void spsc_init(spsc_ring_t *r, uint32_t capacity) {
    atomic_store_explicit(&r->head, 0, memory_order_relaxed);
    atomic_store_explicit(&r->tail, 0, memory_order_relaxed);
    r->capacity = capacity;
    platform_fence();
}

static inline int spsc_enqueue(spsc_ring_t *r, const msg_t *m) {
    uint32_t tail = atomic_load_explicit(&r->tail, memory_order_relaxed);
    uint32_t head = atomic_load_explicit(&r->head, memory_order_acquire);
    uint32_t next = (tail + 1) & (r->capacity - 1);
    if (next == head) return -1;
    r->buffer[tail] = *m;
    platform_fence();
    atomic_store_explicit(&r->tail, next, memory_order_release);
    return 0;
}

static inline int spsc_dequeue(spsc_ring_t *r, msg_t *out) {
    uint32_t head = atomic_load_explicit(&r->head, memory_order_relaxed);
    uint32_t tail = atomic_load_explicit(&r->tail, memory_order_acquire);
    if (head == tail) return -1;
    *out = r->buffer[head];
    platform_fence();
    atomic_store_explicit(&r->head, (head + 1) & (r->capacity - 1),
                          memory_order_release);
    return 0;
}