#include "rtos_impl.h"

void K_exit(void)
{
    SYS_intr_disable();
    gettask()->exited = true;
    yield();
    __builtin_unreachable();
}

void K_exitall(void)
{
    SYS_intr_disable();
    struct task_status* status = gettask();
    status->exited = true;
    ITEROTHERS(task, status)
    {
        task->exited = true;
    }
    yield();
    __builtin_unreachable();
}
