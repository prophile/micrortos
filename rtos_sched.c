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

static int
_next_task(int prev, CLK_T* wait)
{
    CLK_T current_time = CLK_CLOCK();
    CLK_T earliest_until = CLK_ZERO;
    int nexited = 0;
    int first_considered = _next_after(prev);
    int candidate = first_considered;
    do {
        struct task_status* status = &(g_statuses[candidate]);
        if (status->exited) {
            ++nexited;
        } else if (CLK_NONZERO(status->run_after)) {
            if (!CLK_NONZERO(earliest_until) || CLK_AFTER(earliest_until, status->run_after)) {
                earliest_until = status->run_after;
            }

            if (CLK_AFTER(current_time, status->run_after)) {
                // Run after says to run this immediately
                return candidate;
            }
        } else {
            // No "run after": live task, run it if not blocked on a futex
            if (!(status->futex)) {
                return candidate;
            }
        }

        candidate = _next_after(candidate);
    } while (candidate != first_considered);

    if (nexited == g_ntasks) {
        return K_EXITALL;
    } else if (!CLK_NONZERO(earliest_until)) {
        return K_DEADLOCK;
    } else {
        CLK_T difference = earliest_until;
        CLK_SUB(difference, current_time);
        *wait = difference;
        return TASK_IDLE;
    }
}

void _sched(void)
{
    // Now the weird bit; from this SYS_context_get onwards this code is entered
    // _every time_ a yield occurs. NB: interrupts are always disabled at this
    // point.
    struct task_status* running;
    CLK_T wait;
    int running_tid, next;
done_idle:
    running = (struct task_status*)SYS_context_get(&g_yieldcontext);

    if (running) {
        running_tid = running->tid;
    } else {
        running_tid = TASK_IDLE;
    }
    next = _next_task(running_tid, &wait);
    if (UNLIKELY(next == TASK_IDLE)) {
        SYS_intr_enable();
        CLK_IDLE(wait);
        SYS_intr_disable();
        goto done_idle;
    } else if (UNLIKELY(next < 0)) {
        SYS_context_set(&g_exitcontext, (void*)(ptrdiff_t)next);
        __builtin_unreachable();
    } else {
        struct task_status* next_status = &(g_statuses[next]);
        g_running = next_status;
        next_status->run_after = CLK_ZERO;
        SYS_context_set(&(next_status->ctx), NULL);
        __builtin_unreachable();
    }
}
