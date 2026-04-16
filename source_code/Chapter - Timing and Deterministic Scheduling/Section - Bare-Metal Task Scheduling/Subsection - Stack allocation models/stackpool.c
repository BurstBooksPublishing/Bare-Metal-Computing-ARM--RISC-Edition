ARM (AArch64) Stack Management Implementation


#include <stdint.h>
#include <stddef.h>

#define MAX_TASKS 32
#define STACK_SIZE 4096                     // per-task stack (must be power-of-two)

static uint8_t stacks[MAX_TASKS][STACK_SIZE] __attribute__((aligned(16))); // stack storage
static uint32_t free_bitmap = (1U << MAX_TASKS) - 1U; // 1=free, 0=used

// Set a 64-bit canary at stack base for overflow detection.
static inline void init_stack_guard(int id) {
    uint64_t *guard = (uint64_t *)&stacks[id][0];      // lowest address -> guard
    *guard = 0xDEADBEEFDEADBEEFULL;                   // canary value
}

// Allocate a stack and return top-of-stack pointer (aligned).
void *allocate_stack(void) {
    uint32_t mask = free_bitmap;
    if (mask == 0) {
        return NULL;                       // no free stacks
    }
    int id = __builtin_ctz(mask);         // find first free
    free_bitmap &= ~(1U << id);           // mark used
    init_stack_guard(id);
    uintptr_t top = (uintptr_t)&stacks[id][STACK_SIZE]; // stack grows down
    top &= ~((uintptr_t)0xF);             // ensure 16-byte alignment
    return (void *)top;
}

// Free previously allocated stack given pointer returned by allocate_stack.
void free_stack(void *sp) {
    uintptr_t top = (uintptr_t)sp;
    int id = (int)((top - (uintptr_t)&stacks[0][0]) / STACK_SIZE);
    free_bitmap |= (1U << id);            // mark free
}