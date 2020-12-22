#include "rtos.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "rtos_impl.h"

struct task_status* g_statuses;

SYS_context_t g_exitcontext;
SYS_context_t g_yieldcontext;
struct task_status* volatile g_running;

static void
exectask(void* arg)
{
    SYS_intr_enable();
    struct task_status* status = (struct task_status*)arg;
    const struct task_def* def = status->definition;
    def->execute(def->argument);
    status->exited = true;
    SYS_intr_disable();
    _yield();
    __builtin_unreachable();
}

struct task_status* _gettask(void)
{
    return g_running;
}

int K_exec(const struct task_def* tasks)
{
    int ntasks = 0;
    for (int n = 0; tasks[n].execute; ++n) {
        ++ntasks;
    }
    if (ntasks == 0) {
        return K_EXITALL;
    }
    SYS_intr_disable();
    struct task_status statuses_array[ntasks];
    g_statuses = statuses_array;
    for (int n = 0; n < ntasks; ++n) {
        struct task_status* status = &(statuses_array[n]);
        SYS_context_init(&(status->ctx),
            &exectask,
            (void*)status,
            tasks[n].stack,
            tasks[n].stacksize);
        status->tid = n;
        status->futex = NULL;
        status->run_after = CLK_ZERO;
        status->exited = false;
        status->definition = &(tasks[n]);
    }

    for (int n = 0; n < ntasks - 1; ++n) {
        statuses_array[n].next = &(statuses_array[n + 1]);
    }
    statuses_array[ntasks - 1].next = &(statuses_array[0]);

    g_running = NULL;
    void* status = SYS_context_get(&g_exitcontext);
    if (status == NULL) {
        _sched(&(statuses_array[0]));
    }

    SYS_intr_enable();
    return (int)(ptrdiff_t)status;
}
