#ifndef __INCLUDED_RTOS_IMPL_H
#define __INCLUDED_RTOS_IMPL_H

#include "rtos.h"
#include "sys_auto.h"

struct task_status {
    volatile int* futex;
    const struct task_def* definition; // Setting the definition to 0 signals exit
    struct task_status* prev;
    struct task_status* next;
    uint64_t run_after;
    SYS_context_t ctx;
    cleanup_callback_t cleanback;
    void* cleanback_ptr;
};

#define LIKELY(x) __builtin_expect((x), true)
#define UNLIKELY(x) __builtin_expect((x), false)

struct kernel {
    SYS_context_t yieldcontext;
    struct task_status* running;
    const struct kernel_timebase* timebase;
};

static inline uint64_t tb_clock(kernel_t kernel)
{
    return kernel->timebase->get_time(kernel->timebase->ud);
}

static inline uint64_t tb_from_ms(kernel_t kernel, milliseconds_t ms)
{
    return kernel->timebase->from_ms(ms, kernel->timebase->ud);
}

static inline void tb_delay(kernel_t kernel, uint64_t pause)
{
    return kernel->timebase->delay(pause, kernel->timebase->ud);
}

void yield(kernel_t);
int sched(struct kernel* kernel, struct task_status* first_task);
static inline struct task_status* gettask(kernel_t kernel)
{
    return kernel->running;
}
void init_task(
    kernel_t kernel,
    struct task_status* status,
    const struct task_def* definition,
    size_t stackused);

#define ITEROTHERS(var, than_task) for (struct task_status* var = (than_task)->next; var != (than_task); var = var->next)

#endif
