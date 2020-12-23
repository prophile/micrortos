#include "rtos_impl.h"

void K_yield(kernel_t kernel)
{
    SYS_intr_disable();
    yield(kernel);
    SYS_intr_enable();
}

void K_sleep(kernel_t kernel, milliseconds_t interval)
{
    SYS_intr_disable();
    CLK_T base = CLK_CLOCK();
    CLK_ADD(base, CLK_FROMMS(interval));
    gettask(kernel)->run_after = base;
    yield(kernel);
    SYS_intr_enable();
}
