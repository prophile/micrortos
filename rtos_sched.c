#include "rtos_impl.h"

#include <stdio.h>

struct consideration {
    CLK_T current_time;
    CLK_T earliest_until;
};

static bool _is_runnable(struct task_status* status, struct consideration* consider)
{
    if (CLK_NONZERO(status->run_after)) {
        if (!CLK_NONZERO(consider->earliest_until) || CLK_AFTER(consider->earliest_until, status->run_after)) {
            consider->earliest_until = status->run_after;
        }

        if (CLK_AFTER(consider->current_time, status->run_after)) {
            // Run after says to run this immediately
            return true;
        }
    } else {
        // No "run after": live task, run it if not blocked on a futex
        if (!(status->futex)) {
            return true;
        }
    }

    return false;
}

static bool _cleanup_exited_successors(struct task_status* status)
{
    while (status->next->exited) {
        // Note carefully here: the cleanback can and will delete the task status block
        if (status->next == status) {
            // Last task exited
            return true;
        }
        struct task_status* old_successor = status->next;
        struct task_status* new_successor = old_successor->next;
        if (old_successor->cleanback) {
            old_successor->cleanback(old_successor->cleanback_ptr);
        }
        status->next = new_successor;
    }
    return false;
}

static struct task_status*
_next_task(struct task_status* prev, CLK_T* wait, int* exit_code)
{
    struct consideration consider = {
        .earliest_until = CLK_ZERO,
        .current_time = CLK_CLOCK()
    };

    struct task_status* first_considered = prev->next;

    struct task_status* candidate = first_considered;
    do {
        if (_cleanup_exited_successors(candidate)) {
            // This signals that all processes have exited
            *exit_code = K_EXITALL;
            return NULL;
        }

        if (_is_runnable(candidate, &consider)) {
            return candidate;
        }

        candidate = candidate->next;
    } while (candidate != first_considered);

    if (UNLIKELY(!CLK_NONZERO(consider.earliest_until))) {
        *exit_code = K_DEADLOCK;
    } else {
        CLK_T difference = consider.earliest_until;
        CLK_SUB(difference, consider.current_time);
        *exit_code = 0;
        *wait = difference;
    }
    return NULL;
}

int _sched(struct kernel* kernel, struct task_status* first_task)
{
    // Now the weird bit; from this SYS_context_get onwards this code is entered
    // _every time_ a yield occurs. NB: interrupts are always disabled at this
    // point.
    struct task_status* running;
    struct task_status* next;
    CLK_T wait;
    int exit_code;
    kernel->running = NULL;
done_idle:
    running = (struct task_status*)SYS_context_get(&kernel->yieldcontext);

    if (running) {
        next = _next_task(running, &wait, &exit_code);
    } else {
        next = first_task;
    }

    if (UNLIKELY(next == NULL)) {
        if (UNLIKELY(exit_code)) {
            return exit_code;
        }
        SYS_intr_enable();
        CLK_IDLE(wait);
        SYS_intr_disable();
        goto done_idle;
    } else {
        kernel->running = next;
        next->run_after = CLK_ZERO;
        SYS_context_set(&(next->ctx), NULL);
        __builtin_unreachable();
    }
}
