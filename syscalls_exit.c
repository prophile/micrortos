#include "rtos_impl.h"

// Important note: signalling that a task is dead is done by setting
// its definition to NULL.

void K_exit(kernel_t kernel)
{
    SYS_intr_disable();
    gettask(kernel)->definition = NULL;
    yield(kernel);
    __builtin_unreachable();
}

void K_exitall(kernel_t kernel)
{
    SYS_intr_disable();
    struct task_status* status = gettask(kernel);
    status->definition = NULL;
    ITEROTHERS(task, status)
    {
        task->definition = NULL;
    }
    yield(kernel);
    __builtin_unreachable();
}
