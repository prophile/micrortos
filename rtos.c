#include "rtos.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "rtos_impl.h"

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
    struct task_status statuses[ntasks];
    for (int n = 0; n < ntasks; ++n) {
        struct task_status* status = &(statuses[n]);
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
        statuses[n].next = &(statuses[n + 1]);
    }
    statuses[ntasks - 1].next = &(statuses[0]);

    g_running = NULL;
    int status = _sched(&(statuses[0]));

    SYS_intr_enable();

    return status;
}
