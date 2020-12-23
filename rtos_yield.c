#include "rtos_impl.h"

void yield(void)
{
    // Call with interrupts disabled, and do a swapcontext
    struct task_status* status = gettask();
    void* has_swapped = SYS_context_get(&(status->ctx));
    if (has_swapped) {
        return;
    }
    SYS_context_set(&g_kernel.yieldcontext, status);
    __builtin_unreachable();
}
