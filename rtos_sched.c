#include "rtos_impl.h"

#include <stdio.h>

struct consideration {
    CLK_T current_time;
    CLK_T earliest_until;
};

static bool is_runnable(struct task_status* status, struct consideration* consider)
{
    if (!status->definition) {
        // Exited
        return false;
    }

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

static struct task_status*
next_task(struct task_status* prev, CLK_T* wait, int* exit_code)
{
    struct consideration consider = {
        .earliest_until = CLK_ZERO,
        .current_time = CLK_CLOCK()
    };

    struct task_status* first_considered = prev->next;

    struct task_status* candidate = first_considered;
    do {
        if (is_runnable(candidate, &consider)) {
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

int sched(struct kernel* kernel, struct task_status* first_task)
{
    // Now the weird bit; from this SYS_context_get onwards this code is entered
    // _every time_ a yield occurs. NB: interrupts are always disabled at this
    // point.
    struct task_status* running;
    struct task_status* next;
    struct task_status* witness;
    CLK_T wait;
    int exit_code;
    witness = first_task;
    kernel->running = NULL;
    running = (struct task_status*)SYS_context_get(&kernel->yieldcontext);

redo_idle:
    if (running) {
        next = next_task(running, &wait, &exit_code);
    } else {
        next = first_task;
    }

    // Handle the exit condition
    if (running && !running->definition) {
        // Drop it from the linked list
        running->prev->next = running->next;
        running->next->prev = running->prev;
        bool was_last_task = false;
        if (running == witness) {
            if (running->next == witness) {
                was_last_task = true;
            } else {
                witness = running->next;
            }
        }
        // Call the cleanback; note that this may well destroy the task status object
        if (running->cleanback) {
            running->cleanback(running->cleanback_ptr);
        }
        running = NULL;

        if (UNLIKELY(was_last_task)) {
            return K_EXITALL;
        }
    }

    if (UNLIKELY(next == NULL)) {
        if (UNLIKELY(exit_code)) {
            return exit_code;
        }
        SYS_intr_enable();
        CLK_IDLE(wait);
        SYS_intr_disable();
        running = witness;
        goto redo_idle;
    } else {
        kernel->running = next;
        next->run_after = CLK_ZERO;
        SYS_context_set(&(next->ctx), next);
        __builtin_unreachable();
    }
}
