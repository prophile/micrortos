#include "rtos_impl.h"

struct consideration {
    CLK_T current_time;
    CLK_T earliest_until;
    bool any_blocked;
};

static bool _is_runnable(struct task_status* status, struct consideration* consider)
{
    if (status->exited) {
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
        } else {
            // Task is truly blocked
            consider->any_blocked = true;
        }
    }

    return false;
}

static struct task_status*
_next_task(struct task_status* prev, CLK_T* wait, int* exit_code)
{
    struct consideration consider = {
        .any_blocked = false,
        .earliest_until = CLK_ZERO,
        .current_time = CLK_CLOCK()
    };

    struct task_status* first_considered = prev->next;

    struct task_status* candidate = first_considered;
    do {
        if (_is_runnable(candidate, &consider)) {
            return candidate;
        }

        candidate = candidate->next;
    } while (candidate != first_considered);

    if (UNLIKELY(!CLK_NONZERO(consider.earliest_until))) {
        if (consider.any_blocked) {
            *exit_code = K_DEADLOCK;
        } else {
            *exit_code = K_EXITALL;
        }
    } else {
        CLK_T difference = consider.earliest_until;
        CLK_SUB(difference, consider.current_time);
        *exit_code = 0;
        *wait = difference;
    }
    return NULL;
}

int _sched(struct task_status* first_task)
{
    // Now the weird bit; from this SYS_context_get onwards this code is entered
    // _every time_ a yield occurs. NB: interrupts are always disabled at this
    // point.
    struct task_status* running;
    struct task_status* next;
    CLK_T wait;
    int exit_code;
done_idle:
    running = (struct task_status*)SYS_context_get(&g_kernel.yieldcontext);

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
        g_kernel.running = next;
        next->run_after = CLK_ZERO;
        SYS_context_set(&(next->ctx), NULL);
        __builtin_unreachable();
    }
}
