ARM (AArch64) Task Context Switching Implementation


#include <stdint.h>

#define MAX_TASKS 8

typedef struct {
    uint64_t regs[13];    /* saved x19..x30, sp at index 12 */
    void *stack_ptr;
    void (*entry)(void *);
    void *arg;
    uint8_t state;        /* 0 = ready, 1 = running, 2 = blocked */
    uint8_t prio;
} tcb_t;

extern void switch_context(uint64_t *old_regs, uint64_t *new_regs);

static tcb_t tasks[MAX_TASKS];
static int current = 0;
static int task_count = 0;

/* Called by task to yield; must be callable with interrupts enabled. */
void task_yield(void) {
    int next = (current + 1) % task_count;
    /* brief critical: protect current/next swap (single-core: disable IRQs) */
    __asm__ volatile ("msr daifset, #2" ::: "memory"); // disable IRQ on AArch64
    tcb_t *old = &tasks[current];
    tcb_t *nw  = &tasks[next];
    current = next;
    __asm__ volatile ("msr daifclr, #2" ::: "memory"); // re-enable IRQ
    switch_context(old->regs, nw->regs);
}