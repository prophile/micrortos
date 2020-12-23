#include "rtos_impl.h"

void yield(kernel_t kernel)
{
    // Call with interrupts disabled, and do a swapcontext
    struct task_status* status = gettask(kernel);
    void* has_swapped = SYS_context_get(&(status->ctx));
    if (has_swapped) {
        return;
    }
    SYS_context_set(&kernel->yieldcontext, status);
    __builtin_unreachable();
}
