#ifndef __INCLUDED_RTOS_IMPL_H
#define __INCLUDED_RTOS_IMPL_H

#include "rtos.h"
#include "sys_auto.h"

struct task_status {
    int tid;
    volatile int* futex;
    CLK_T run_after;
    bool exited;
    const struct task_def* definition;
    SYS_context_t ctx;
};

#define LIKELY(x) __builtin_expect((x), true)
#define UNLIKELY(x) __builtin_expect((x), false)

extern int g_ntasks;
extern struct task_status* g_statuses;

static const int TASK_IDLE = -1;

extern SYS_context_t g_exitcontext;
extern SYS_context_t g_yieldcontext;
extern struct task_status * volatile g_running;

void _yield(void);
void _sched(void) __attribute__((noreturn));
struct task_status* _gettask(void);

#endif
