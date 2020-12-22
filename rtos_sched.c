#include "rtos_impl.h"

static int
_next_after(int prev)
{
    if (prev == g_ntasks - 1 || prev < 0) {
        return 0;
    } else {
        return prev + 1;
    }
}

static struct task_status*
_next_task(int prev, CLK_T* wait, int* exit_code)
{
    CLK_T current_time = CLK_CLOCK();
    CLK_T earliest_until = CLK_ZERO;
    bool any_blocked = false;
    int first_considered = _next_after(prev);
    int candidate = first_considered;
    do {
        struct task_status* status = &(g_statuses[candidate]);
        if (status->exited) {
            // Do nothing here
        } else if (CLK_NONZERO(status->run_after)) {
            if (!CLK_NONZERO(earliest_until) || CLK_AFTER(earliest_until, status->run_after)) {
                earliest_until = status->run_after;
            }

            if (CLK_AFTER(current_time, status->run_after)) {
                // Run after says to run this immediately
                return status;
            }
        } else {
            // No "run after": live task, run it if not blocked on a futex
            if (!(status->futex)) {
                return status;
            } else {
                // Task is truly blocked
                any_blocked = true;
            }
        }

        candidate = _next_after(candidate);
    } while (candidate != first_considered);

    if (UNLIKELY(!CLK_NONZERO(earliest_until))) {
        if (any_blocked) {
            *exit_code = K_DEADLOCK;
        } else {
            *exit_code = K_EXITALL;
        }
    } else {
        CLK_T difference = earliest_until;
        CLK_SUB(difference, current_time);
        *exit_code = 0;
        *wait = difference;
    }
    return NULL;
}

void _sched(void)
{
    // Now the weird bit; from this SYS_context_get onwards this code is entered
    // _every time_ a yield occurs. NB: interrupts are always disabled at this
    // point.
    struct task_status* running;
    struct task_status* next;
    CLK_T wait;
    int running_tid, exit_code;
done_idle:
    running = (struct task_status*)SYS_context_get(&g_yieldcontext);

    if (running) {
        running_tid = running->tid;
    } else {
        running_tid = TASK_IDLE;
    }
    next = _next_task(running_tid, &wait, &exit_code);
    if (UNLIKELY(next == NULL)) {
        if (UNLIKELY(exit_code)) {
            SYS_context_set(&g_exitcontext, (void*)(ptrdiff_t)exit_code);
            __builtin_unreachable();
        }
        SYS_intr_enable();
        CLK_IDLE(wait);
        SYS_intr_disable();
        goto done_idle;
    } else {
        g_running = next;
        next->run_after = CLK_ZERO;
        SYS_context_set(&(next->ctx), NULL);
        __builtin_unreachable();
    }
}
