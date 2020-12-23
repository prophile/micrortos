#include "rtos.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "rtos_impl.h"

static void
exectask(void* knl, void* arg)
{
    SYS_intr_enable();
    struct task_status* status = (struct task_status*)arg;
    kernel_t kernel = (kernel_t)knl;
    const struct task_def* def = status->definition;
    def->execute(kernel, def->argument);
    status->definition = NULL;
    SYS_intr_disable();
    yield(kernel);
    __builtin_unreachable();
}

void init_task(kernel_t kernel, struct task_status* status, const struct task_def* definition, size_t stackused)
{
    // Should stackused be rounded up for alignment?
    SYS_context_init(&(status->ctx),
        &exectask,
        (void*)kernel,
        (void*)status,
        (void*)((const uint8_t*)definition->stack + stackused),
        definition->stacksize - stackused);
    status->futex = NULL;
    status->run_after = CLK_ZERO;
    status->definition = definition;
    status->cleanback = NULL;
    status->cleanback_ptr = NULL;
}

int kernel_exec(const struct task_def* task)
{
    struct kernel kernel;
    struct task_status init;

    init_task(&kernel, &init, task, 0);
    init.next = &init;
    init.prev = &init;

    SYS_intr_disable();
    int status = sched(&kernel, &init);
    SYS_intr_enable();

    return status;
}
