#include "rtos_impl.h"

static int
wait(kernel_t kernel, volatile int* address, int value, CLK_T timeout)
{
    int ret = 0;
    CLK_T base;
    if (CLK_NONZERO(timeout)) {
        // Do this before disabling interrupts
        base = CLK_CLOCK();
    }
    SYS_intr_disable();
    int loaded = *address;
    if (loaded != value) {
        ret = -1;
    } else {
        struct task_status* status = gettask(kernel);
        status->futex = address;
        if (CLK_NONZERO(timeout)) {
            CLK_ADD(base, timeout);
            status->run_after = base;
        }
        yield(kernel);
        if (status->futex == NULL) {
            ret = 0;
        } else {
            status->futex = NULL;
            ret = 1;
        }
    }
    SYS_intr_enable();
    return ret;
}

bool K_wait(kernel_t kernel, volatile int* address, int value)
{
    return wait(kernel, address, value, CLK_ZERO) > 0;
}

bool K_wait_timeout(kernel_t kernel, volatile int* address, int value, milliseconds_t timeout)
{
    return wait(kernel, address, value, CLK_FROMMS(timeout)) != 0;
}

void K_wake_one(kernel_t kernel, volatile int* address)
{
    SYS_intr_disable();
    bool did_wake = false;
    ITEROTHERS(task, gettask(kernel))
    {
        if (task->futex == address) {
            task->futex = NULL;
            task->run_after = CLK_ZERO;
            did_wake = true;
            break;
        }
    }
    if (did_wake)
        yield(kernel);
    SYS_intr_enable();
}

void K_wake_all(kernel_t kernel, volatile int* address)
{
    SYS_intr_disable();
    bool did_wake = false;
    ITEROTHERS(task, gettask(kernel))
    {
        if (task->futex == address) {
            task->futex = NULL;
            task->run_after = CLK_ZERO;
            did_wake = true;
        }
    }
    if (did_wake)
        yield(kernel);
    SYS_intr_enable();
}
