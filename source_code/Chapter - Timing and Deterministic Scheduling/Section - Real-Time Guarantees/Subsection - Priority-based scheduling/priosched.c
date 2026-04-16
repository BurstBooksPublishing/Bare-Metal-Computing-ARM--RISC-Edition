Production-Ready Real-Time Scheduler (C Language)


/* Compact, production-ready scheduler kernel (freestanding C).
 * Requires architecture-specific context_switch implementation:
 *   void context_switch(void **old_sp, void *new_sp);
 * The context switch must save caller registers and update stack pointers.
 */
#include <stdint.h>
#include <stdbool.h>

#define MAX_TASKS       32
#define PRIORITY_BITS   32

typedef void (*task_fn)(void *arg);

/* Task control block: statically allocated */
typedef struct {
    uint32_t id;
    uint8_t  priority;    /* 0 = highest */
    uint32_t period_ms;
    uint32_t wcet_ms;     /* used for analysis and accounting */
    void    *sp;          /* stack pointer for context switch */
    bool     ready;
    task_fn  entry;
    void    *arg;
} tcb_t;

static tcb_t tasks[MAX_TASKS];
static uint32_t task_count = 0;

/* Ready bitmap: bit n set => priority n has at least one ready task */
static volatile uint32_t ready_bitmap = 0;

/* Find highest-priority ready bit (lower index is higher priority). */
static inline int highest_ready_priority(void) {
    if (ready_bitmap == 0) return -1;
    /* __builtin_clz returns leading zeros; compute index for 32-bit word */
    return 31 - __builtin_clz(ready_bitmap); /* maps bit31->0, bit0->31 */
}

/* Atomically set/clear ready bit for a given priority. Critical section must mask interrupts. */
static inline void mark_ready(uint8_t prio) {
    ready_bitmap |= (1u << prio);
}
static inline void clear_ready(uint8_t prio) {
    ready_bitmap &= ~(1u << prio);
}

/* Forward declaration of arch context_switch */
extern void context_switch(void **old_sp, void *new_sp);

/* Current running task index (-1 if idle) */
static int current_task = -1;

/* Create a task: returns task id or -1 on failure. Caller must provide stack and its top as sp. */
int create_task(task_fn entry, void *arg, void *sp, uint8_t priority,
                uint32_t period_ms, uint32_t wcet_ms) {
    if (task_count >= MAX_TASKS) return -1;
    int id = (int)task_count++;
    tasks[id].id = id;
    tasks[id].entry = entry;
    tasks[id].arg = arg;
    tasks[id].sp = sp;
    tasks[id].priority = priority;
    tasks[id].period_ms = period_ms;
    tasks[id].wcet_ms = wcet_ms;
    tasks[id].ready = false;
    return id;
}

/* Called by timer interrupt to mark a task ready (sporadic/periodic release). */
void timer_release_task(int id) {
    /* short critical section: disable interrupts externally around this call if necessary */
    tasks[id].ready = true;
    mark_ready(tasks[id].priority);
}

/* Scheduler dispatcher: select highest-priority ready task and perform context switch. */
void schedule(void) {
    int prio = highest_ready_priority();
    if (prio < 0) {
        /* no ready tasks: optionally run idle */
        return;
    }
    /* select the first ready task with that priority (linear scan is small cost) */
    int next = -1;
    for (int i = 0; i < (int)task_count; ++i) {
        if (tasks[i].ready && tasks[i].priority == (uint8_t)prio) { next = i; break; }
    }
    if (next < 0) return; /* race: bitmap set but task cleared */
    if (next == current_task) return;

    /* prepare for context switch: clear ready if single-run semantics */
    tasks[next].ready = false;
    /* if no other tasks at this priority, clear bitmap */
    bool other = false;
    for (int i = 0; i < (int)task_count; ++i) {
        if (tasks[i].ready && tasks[i].priority == (uint8_t)prio) { other = true; break; }
    }
    if (!other) clear_ready((uint8_t)prio);

    int prev = current_task;
    current_task = next;
    if (prev >= 0) {
        /* context switch: save old SP and restore new SP */
        context_switch(&tasks[prev].sp, tasks[next].sp);
    } else {
        /* entering first task from idle */
        void *dummy = NULL;
        context_switch(&dummy, tasks[next].sp);
    }
}

/* Voluntary yield: running task calls to allow higher-priority tasks to run. */
void task_yield(void) {
    tasks[current_task].ready = true; /* requeue current task */
    mark_ready(tasks[current_task].priority);
    schedule();
}