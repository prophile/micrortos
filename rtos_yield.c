#include "rtos_impl.h"

void _yield(void)
{
    // Call with interrupts disabled, and do a swapcontext
    volatile ptrdiff_t has_swapped = 0;
    struct task_status* status = _gettask();
    SYS_context_get(&(status->ctx));
    if (has_swapped) {
        return;
    }
    has_swapped = true;
    SYS_context_set(&g_yieldcontext, status);
    __builtin_unreachable();
}
