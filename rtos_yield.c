#include "rtos_impl.h"

void _yield(void) {
    // Call with interrupts disabled, and do a swapcontext
    volatile ptrdiff_t has_swapped = 0;
    SYS_context_get(&(g_statuses[g_running].ctx));
    if (has_swapped) {
        return;
    }
    has_swapped = true;
    SYS_context_set(&g_yieldcontext);
    __builtin_unreachable();
}
