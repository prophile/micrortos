#include "rtos.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "sys_auto.h"

extern const struct task_def task_definitions[];

struct task_status {
    volatile int* futex;
    CLK_T run_after;
    bool exited;
    SYS_context_t ctx;
};

#define LIKELY(x) __builtin_expect((x), true)
#define UNLIKELY(x) __builtin_expect((x), false)

static int g_ntasks;
static struct task_status* g_statuses;

static const int TASK_IDLE = -1;

static SYS_context_t g_exitcontext;
static SYS_context_t g_yieldcontext;
static volatile int g_running;

static void _yield(void);

static void exectask(void* arg) {
    SYS_intr_enable();
    int tid = (int)(ptrdiff_t)arg;
    task_definitions[tid].execute(task_definitions[tid].argument);
    g_statuses[tid].exited = true;
    SYS_intr_disable();
    _yield();
    __builtin_unreachable();
}

static int _next_after(int prev) {
    if (prev == g_ntasks - 1 || prev < 0) {
        return 0;
    } else {
        return prev + 1;
    }
}

static int _next_task(int prev, CLK_T* wait) {
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

static void _sched(void) __attribute__((noreturn));

static void _sched(void) {
    // Now the weird bit; from this SYS_context_get onwards this code is entered _every time_
    // a yield occurs. NB: interrupts are always disabled at this point.
done_idle:
    SYS_context_get(&g_yieldcontext);

    CLK_T wait;
    int running = g_running;
    int next = _next_task(running, &wait);
    g_running = next;
    if (UNLIKELY(next == TASK_IDLE)) {
        SYS_intr_enable();
        CLK_IDLE(wait);
        SYS_intr_disable();
        goto done_idle;
    } else if (UNLIKELY(next < 0)) {
        SYS_context_set(&g_exitcontext);
        __builtin_unreachable();
    } else {
        g_statuses[next].run_after = CLK_ZERO;
        SYS_context_set(&(g_statuses[next].ctx));
        __builtin_unreachable();
    }
}

static int _entry(void) {
    SYS_intr_disable();
    int ntasks = 0;
    for (int n = 0; task_definitions[n].execute; ++n) {
        ++ntasks;
    }
    g_ntasks = ntasks;
    struct task_status statuses_array[ntasks];
    g_statuses = statuses_array;
    for (int n = 0; n < ntasks; ++n) {
        SYS_context_init(
            &(statuses_array[n].ctx),
            &exectask,
            (void*)(ptrdiff_t)n,
            task_definitions[n].stack,
            task_definitions[n].stacksize
        );
        statuses_array[n].futex = NULL;
        statuses_array[n].run_after = CLK_ZERO;
        statuses_array[n].exited = false;
    }

    g_running = TASK_IDLE;
    SYS_context_get(&g_exitcontext);
    int status = g_running;
    if (status == TASK_IDLE) {
        _sched();
    }

    SYS_intr_enable();
    return status;
}

static void _yield(void) {
    // Call with interrupts disabled, and do a swapcontext
    volatile ptrdiff_t has_swapped = 0;
    SYS_context_get(&(g_statuses[g_running].ctx));
    if (has_swapped) {
        return;
    }
    has_swapped = true;
    SYS_context_set(&g_yieldcontext);
    __builtin_unreachable();
}

static int _wait(volatile int* address, int value, CLK_T timeout) {
    int ret = 0;
    CLK_T base;
    if (CLK_NONZERO(timeout)) {
        // Do this before disabling interrupts
        base = CLK_CLOCK();
    }
    SYS_intr_disable();
    int loaded = *address;
    if (loaded != value) {
        ret = -1;
    } else {
        struct task_status* status = &(g_statuses[g_running]);
        status->futex = address;
        if (CLK_NONZERO(timeout)) {
            CLK_ADD(base, timeout);
            status->run_after = base;
        }
        _yield();
        if (status->futex == NULL) {
            ret = 0;
        } else {
            status->futex = NULL;
            ret = 1;
        }
    }
    SYS_intr_enable();
    return ret;
}

bool K_wait(volatile int* address, int value) {
    return _wait(address, value, CLK_ZERO) > 0;
}

bool K_wait_timeout(volatile int* address, int value, milliseconds_t timeout) {
    return _wait(address, value, CLK_FROMMS(timeout)) != 0;
}

void K_wake_one(volatile int* address) {
    SYS_intr_disable();
    bool did_wake = false;
    for (int i = 0; i < g_ntasks; ++i) {
        if (g_statuses[i].futex == address) {
            g_statuses[i].futex = NULL;
            g_statuses[i].run_after = CLK_ZERO;
            did_wake = true;
            break;
        }
    }
    if (did_wake)
        _yield();
    SYS_intr_enable();
}

void K_wake_all(volatile int* address) {
    SYS_intr_disable();
    bool did_wake = false;
    for (int i = 0; i < g_ntasks; ++i) {
        if (g_statuses[i].futex == address) {
            g_statuses[i].futex = NULL;
            g_statuses[i].run_after = CLK_ZERO;
            did_wake = true;
        }
    }
    if (did_wake)
        _yield();
    SYS_intr_enable();
}

int K_gettid(void) {
    return g_running;
}

void K_yield(void) {
    SYS_intr_disable();
    _yield();
    SYS_intr_enable();
}

void K_exit(void) {
    SYS_intr_disable();
    g_statuses[g_running].exited = true;
    _yield();
    __builtin_unreachable();
}

void K_exitall(void) {
    SYS_intr_disable();
    for (int i = 0; i < g_ntasks; ++i) {
        g_statuses[i].exited = true;
    }
    _yield();
    __builtin_unreachable();
}

void K_sleep(milliseconds_t interval) {
    SYS_intr_disable();
    CLK_T base = CLK_CLOCK();
    CLK_ADD(base, CLK_FROMMS(interval));
    g_statuses[g_running].run_after = base;
    _yield();
    SYS_intr_enable();
}

int K_exec(void) {
    return _entry();
}
