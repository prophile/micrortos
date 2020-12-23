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
    uint64_t deadline = tb_clock(kernel) + tb_from_ms(kernel, interval);
    gettask(kernel)->run_after = deadline;
    yield(kernel);
    SYS_intr_enable();
}
