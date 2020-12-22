#include "rtos_impl.h"

void yield(void)
{
    // Call with interrupts disabled, and do a swapcontext
    volatile ptrdiff_t has_swapped = 0;
    struct task_status* status = gettask();
    SYS_context_get(&(status->ctx));
    if (has_swapped) {
        return;
    }
    has_swapped = true;
    SYS_context_set(&g_kernel.yieldcontext, status);
    __builtin_unreachable();
}
