#include "rtos_impl.h"

void K_yield(void)
{
    SYS_intr_disable();
    yield();
    SYS_intr_enable();
}

void K_sleep(milliseconds_t interval)
{
    SYS_intr_disable();
    CLK_T base = CLK_CLOCK();
    CLK_ADD(base, CLK_FROMMS(interval));
    gettask()->run_after = base;
    yield();
    SYS_intr_enable();
}
