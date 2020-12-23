#include "rtos_impl.h"

struct task_control_block {
    struct task_status status;
    struct task_def definition;
};

void K_spawn(const struct task_def* definition, cleanup_callback_t cleanback, void* ud)
{
    SYS_intr_disable();

    struct task_status* our_status = gettask();
    struct task_control_block* tcb = (struct task_control_block*)(definition->stack);
    tcb->definition.execute = definition->execute;
    tcb->definition.argument = definition->argument;
    tcb->definition.stack = (void*)((const uint8_t*)(definition->stack) + sizeof(struct task_control_block));
    tcb->definition.stacksize = definition->stacksize - sizeof(struct task_control_block);
    init_task(&(tcb->status), &(tcb->definition));
    tcb->status.cleanback = cleanback;
    tcb->status.cleanback_ptr = ud;

    tcb->status.next = our_status->next;
    our_status->next = &(tcb->status);

    yield();

    SYS_intr_enable();
}
