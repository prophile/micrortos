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
    for (int i = 0; i < g_ntasks; ++i) {
        g_statuses[i].exited = true;
    }
    _yield();
    __builtin_unreachable();
}
