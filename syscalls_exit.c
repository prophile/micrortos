#include "rtos_impl.h"

void K_exit(void)
{
    SYS_intr_disable();
    _gettask()->exited = true;
    _yield();
    __builtin_unreachable();
}

void K_exitall(void)
{
    SYS_intr_disable();
    struct task_status* status = _gettask();
    status->exited = true;
    ITEROTHERS(task, status) {
        task->exited = true;
    }
    _yield();
    __builtin_unreachable();
}
