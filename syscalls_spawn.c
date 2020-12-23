#include "rtos_impl.h"

struct task_control_block {
    struct task_status status;
    struct task_def definition;
};

void K_spawn(kernel_t kernel, const struct task_def* definition, cleanup_callback_t cleanback, void* ud)
{
    SYS_intr_disable();

    struct task_status* our_status = gettask(kernel);

    struct task_control_block* tcb = (struct task_control_block*)(definition->stack);
    tcb->definition = *definition;
    init_task(kernel, &(tcb->status), &(tcb->definition), sizeof(struct task_control_block));
    tcb->status.cleanback = cleanback;
    tcb->status.cleanback_ptr = ud;

    tcb->status.next = our_status->next;
    tcb->status.prev = our_status;
    our_status->next = &(tcb->status);

    yield(kernel);

    SYS_intr_enable();
}
