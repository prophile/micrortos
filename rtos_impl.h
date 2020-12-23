#ifndef __INCLUDED_RTOS_IMPL_H
#define __INCLUDED_RTOS_IMPL_H

#include "rtos.h"
#include "sys_auto.h"

struct task_status {
    volatile int* futex;
    CLK_T run_after;
    const struct task_def* definition; // Setting the definition to 0 signals exit
    struct task_status* prev;
    struct task_status* next;
    SYS_context_t ctx;
    cleanup_callback_t cleanback;
    void* cleanback_ptr;
};

#define LIKELY(x) __builtin_expect((x), true)
#define UNLIKELY(x) __builtin_expect((x), false)

struct kernel {
    SYS_context_t yieldcontext;
    struct task_status* volatile running;
};

extern struct kernel g_kernel;

void yield(void);
int sched(struct kernel* kernel, struct task_status* first_task);
struct task_status* gettask(void);
void init_task(struct task_status* status, const struct task_def* definition);

#define ITEROTHERS(var, than_task) for (struct task_status* var = (than_task)->next; var != (than_task); var = var->next)

#endif
