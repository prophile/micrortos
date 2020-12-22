#include "rtos.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "rtos_impl.h"

int g_ntasks;
struct task_status* g_statuses;

SYS_context_t g_exitcontext;
SYS_context_t g_yieldcontext;
volatile int g_running;

static void
exectask(void* arg)
{
    SYS_intr_enable();
    int tid = (int)(ptrdiff_t)arg;
    task_definitions[tid].execute(task_definitions[tid].argument);
    g_statuses[tid].exited = true;
    SYS_intr_disable();
    _yield();
    __builtin_unreachable();
}

int K_exec(void)
{
    SYS_intr_disable();
    int ntasks = 0;
    for (int n = 0; task_definitions[n].execute; ++n) {
        ++ntasks;
    }
    g_ntasks = ntasks;
    struct task_status statuses_array[ntasks];
    g_statuses = statuses_array;
    for (int n = 0; n < ntasks; ++n) {
        SYS_context_init(&(statuses_array[n].ctx),
            &exectask,
            (void*)(ptrdiff_t)n,
            task_definitions[n].stack,
            task_definitions[n].stacksize);
        statuses_array[n].futex = NULL;
        statuses_array[n].run_after = CLK_ZERO;
        statuses_array[n].exited = false;
    }

    g_running = TASK_IDLE;
    SYS_context_get(&g_exitcontext);
    int status = g_running;
    if (status == TASK_IDLE) {
        _sched();
    }

    SYS_intr_enable();
    return status;
}
