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

static bool _is_runnable(struct task_status* status, bool* any_blocked, CLK_T current_time, CLK_T* earliest_until) {
    if (status->exited) {
        return false;
    }

    if (CLK_NONZERO(status->run_after)) {
        if (!CLK_NONZERO(*earliest_until) || CLK_AFTER(*earliest_until, status->run_after)) {
            *earliest_until = status->run_after;
        }

        if (CLK_AFTER(current_time, status->run_after)) {
            // Run after says to run this immediately
            return true;
        }
    } else {
        // No "run after": live task, run it if not blocked on a futex
        if (!(status->futex)) {
            return true;
        } else {
            // Task is truly blocked
            *any_blocked = true;
        }
    }

    return false;
}

static struct task_status*
_next_task(struct task_status* prev, CLK_T* wait, int* exit_code)
{
    CLK_T current_time = CLK_CLOCK();
    CLK_T earliest_until = CLK_ZERO;
    bool any_blocked = false;
    int first_considered;
    if (prev != NULL) {
        first_considered = _next_after(prev->tid);
    } else {
        first_considered = 0;
    }
    int candidate = first_considered;
    do {
        struct task_status* status = &(g_statuses[candidate]);

        if (_is_runnable(status, &any_blocked, current_time, &earliest_until)) {
            return status;
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
    int exit_code;
done_idle:
    running = (struct task_status*)SYS_context_get(&g_yieldcontext);

    next = _next_task(running, &wait, &exit_code);
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
