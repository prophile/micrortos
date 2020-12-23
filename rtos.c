#include "rtos.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "rtos_impl.h"

struct kernel g_kernel;

static void
exectask(void* arg)
{
    SYS_intr_enable();
    struct task_status* status = (struct task_status*)arg;
    const struct task_def* def = status->definition;
    def->execute(def->argument);
    status->definition = NULL;
    SYS_intr_disable();
    yield();
    __builtin_unreachable();
}

struct task_status* gettask(void)
{
    return g_kernel.running;
}

void init_task(struct task_status* status, const struct task_def* definition)
{
    SYS_context_init(&(status->ctx),
        &exectask,
        (void*)status,
        definition->stack,
        definition->stacksize);
    status->futex = NULL;
    status->run_after = CLK_ZERO;
    status->definition = definition;
    status->cleanback = NULL;
    status->cleanback_ptr = NULL;
}

int K_exec(const struct task_def* task)
{
    SYS_intr_disable();
    // Build the root task
    struct task_status root;
    init_task(&root, task);

    root.next = &root; // Make it a closed chain
    root.prev = &root;

    int status = sched(&g_kernel, &root);
    SYS_intr_enable();

    return status;
}
