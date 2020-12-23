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

struct root_block {
    struct kernel kernel;
    struct task_status init;
};

kernel_t kernel_create(const struct task_def* task)
{
    struct root_block* root = (struct root_block*)task->stack;
    init_task(&root->kernel, &root->init, task, sizeof(struct root_block));
    root->init.next = &root->init;
    root->init.prev = &root->init;
    return &root->kernel;
}

int kernel_exec(kernel_t kernel)
{
    // Rely on some knowledge of structure here
    struct root_block* root = (struct root_block*)kernel;
    SYS_intr_disable();
    int status = sched(kernel, &root->init);
    SYS_intr_enable();

    return status;
}
