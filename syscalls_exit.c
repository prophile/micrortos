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
