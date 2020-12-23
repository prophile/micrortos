#include "rtos_impl.h"

// Important note: signalling that a task is dead is done by setting
// its definition to NULL.

void K_exit(void)
{
    SYS_intr_disable();
    gettask()->definition = NULL;
    yield();
    __builtin_unreachable();
}

void K_exitall(void)
{
    SYS_intr_disable();
    struct task_status* status = gettask();
    status->definition = NULL;
    ITEROTHERS(task, status)
    {
        task->definition = NULL;
    }
    yield();
    __builtin_unreachable();
}
