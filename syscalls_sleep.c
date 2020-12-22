#include "rtos_impl.h"

void K_yield(void) {
    SYS_intr_disable();
    _yield();
    SYS_intr_enable();
}

void K_sleep(milliseconds_t interval) {
    SYS_intr_disable();
    CLK_T base = CLK_CLOCK();
    CLK_ADD(base, CLK_FROMMS(interval));
    g_statuses[g_running].run_after = base;
    _yield();
    SYS_intr_enable();
}
