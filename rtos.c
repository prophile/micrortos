#define OPT_NOINTR
#define OPT_CTIME

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

struct task_def {
    void (*execute)(void*);
    void* argument;
    void* stack;
    size_t stacksize;
};

typedef uint32_t milliseconds_t;

typedef uint32_t SYS_clock_t;
extern SYS_clock_t SYS_clock(void);
extern void SYS_idle(SYS_clock_t);
extern SYS_clock_t SYS_ms_to_clock(milliseconds_t ms);
extern void SYS_intr_enable(void);
extern void SYS_intr_disable(void);

#ifdef OPT_CTIME
#include <time.h>
#include <unistd.h>

static struct timespec ctime_clock(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}

static inline void ctime_idle(struct timespec clock) {
    nanosleep(&clock, NULL);
}

static inline struct timespec ctime_fromms(milliseconds_t ms) {
    struct timespec ts;
    ts.tv_sec = (ms / 1000);
    ts.tv_nsec = 1000000 * (ms % 1000);
    return ts;
}

static inline bool ctime_after(struct timespec a, struct timespec b) {
    if (a.tv_sec > b.tv_sec) {
        return true;
    }
    if (a.tv_sec < b.tv_sec) {
        return false;
    }
    return a.tv_nsec > b.tv_nsec;
}

static inline struct timespec ctime_add(struct timespec a, struct timespec b) {
    a.tv_nsec += b.tv_nsec;
    a.tv_sec += b.tv_sec;
    if (a.tv_nsec > 1000000000) {
        a.tv_nsec -= 1000000000;
        a.tv_sec += 1;
    }
    return a;
}

static inline struct timespec ctime_sub(struct timespec a, struct timespec b) {
    a.tv_nsec -= b.tv_nsec;
    a.tv_sec -= b.tv_sec;
    if (a.tv_nsec < 0) {
        a.tv_nsec += 1000000000;
        a.tv_sec -= 1;
    }
    return a;
}

#define  CLK_T struct timespec
#define  CLK_IDLE ctime_idle
#define  CLK_CLOCK ctime_clock
#define  CLK_FROMMS ctime_fromms
#define  CLK_ZERO ((struct timespec){.tv_sec = 0, .tv_nsec = 0})
#define  CLK_AFTER(a, b) ((a).tv_sec > (b).tv_sec || ((a).tv_sec == (b).tv_sec && (a).tv_nsec > (b).tv_nsec))
#define  CLK_NONZERO(a) CLK_AFTER((a), CLK_ZERO)
#define  CLK_ADD(a, b) { (a) = ctime_add((a), (b)); }
#define  CLK_SUB(a, b) { (a) = ctime_sub((a), (b)); }
#else
#define  CLK_T  SYS_clock_t
#define  CLK_IDLE SYS_idle
#define  CLK_CLOCK SYS_clock
#define  CLK_FROMMS SYS_ms_to_clock
#define  CLK_ZERO 0
#define  CLK_AFTER(a, b) ((a) > (b))
#define  CLK_NONZERO(a) ((bool)(a))
#define  CLK_ADD(a, b) { (a) += (b); }
#define  CLK_SUB(a, b) { (a) -= (b); }
#endif

#ifdef OPT_NOINTR
#define INTREN()
#define INTRDI()
#else
#define INTREN() SYS_intr_enable()
#define INTRDI() SYS_intr_disable()
#endif

// Begin x86 goo
typedef volatile struct _SYS_context_x86_64 {
    uint64_t ip;
    uint64_t rbx;
    uint64_t rsp;
    uint64_t rbp;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
} SYS_context_t;

extern void SYS_context_get(SYS_context_t*);
extern void SYS_context_set(SYS_context_t*) __attribute__((noreturn));
extern void _SYS_context_bootstrap();

void SYS_context_init(SYS_context_t* ctx, void (*callable)(void*), void* arg, void* stack, size_t stacksize)
{
    ctx->ip = (uint64_t)&_SYS_context_bootstrap;
    ctx->rbx = (uint64_t)callable;
    ctx->rsp = (uint64_t)((uint8_t*)stack + stacksize);
    ctx->rbp = (uint64_t)((uint8_t*)stack + stacksize);
    ctx->r12 = (uint64_t)arg;
    ctx->r13 = 0;
    ctx->r14 = 0;
    ctx->r15 = 0;
}

__asm__(
    "_SYS_context_get:\n"
    "    movq (%rsp), %rax\n"
    "    movq %rax, (%rdi)\n"
    "    movq %rbx, 8(%rdi)\n"
    "    movq %rsp, %rax\n"
    "    addq $8, %rax\n"
    "    movq %rax, 16(%rdi)\n"
    "    movq %rbp, 24(%rdi)\n"
    "    movq %r12, 32(%rdi)\n"
    "    movq %r13, 40(%rdi)\n"
    "    movq %r14, 48(%rdi)\n"
    "    movq %r15, 56(%rdi)\n"
    "    ret\n"
    "\n"
    "_SYS_context_set:\n"
    "    movq (%rdi), %rax\n"
    "    movq 8(%rdi), %rbx\n"
    "    movq 16(%rdi), %rsp\n"
    "    movq 24(%rdi), %rbp\n"
    "    movq 32(%rdi), %r12\n"
    "    movq 40(%rdi), %r13\n"
    "    movq 48(%rdi), %r14\n"
    "    movq 56(%rdi), %r15\n"
    "    jmp *%rax\n"
    "\n"
    "__SYS_context_bootstrap:\n"
    "    movq %rbx, %rax\n"
    "    movq %r12, %rdi\n"
    "    callq *%rax\n"
    "    ud2\n"
);
// End x86 goo

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
const int K_EXITALL = -2;
const int K_DEADLOCK = -3;
const int K_UNKNOWN = -4;

static SYS_context_t g_exitcontext;
static SYS_context_t g_yieldcontext;
static volatile int g_running;

static void _yield(void);

static void exectask(void* arg) {
    INTREN();
    int tid = (int)(ptrdiff_t)arg;
    task_definitions[tid].execute(task_definitions[tid].argument);
    g_statuses[tid].exited = true;
    INTRDI();
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
        INTREN();
        CLK_IDLE(wait);
        INTRDI();
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
    INTRDI();
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

    INTREN();
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
    INTRDI();
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
    INTREN();
    return ret;
}

bool K_wait(volatile int* address, int value) {
    return _wait(address, value, CLK_ZERO) > 0;
}

bool K_wait_timeout(volatile int* address, int value, milliseconds_t timeout) {
    return _wait(address, value, CLK_FROMMS(timeout)) != 0;
}

void K_wake_one(volatile int* address) {
    INTRDI();
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
    INTREN();
}

void K_wake_all(volatile int* address) {
    INTRDI();
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
    INTREN();
}

int K_gettid(void) {
    return g_running;
}

void K_yield(void) {
    INTRDI();
    _yield();
    INTREN();
}

void K_exit(void) {
    INTRDI();
    g_statuses[g_running].exited = true;
    _yield();
    __builtin_unreachable();
}

void K_exitall(void) {
    INTRDI();
    for (int i = 0; i < g_ntasks; ++i) {
        g_statuses[i].exited = true;
    }
    _yield();
    __builtin_unreachable();
}

void K_sleep(milliseconds_t interval) {
    INTRDI();
    CLK_T base = CLK_CLOCK();
    CLK_T basecopy = base;
    CLK_ADD(base, CLK_FROMMS(interval));
    g_statuses[g_running].run_after = base;
    _yield();
    INTREN();
}

int K_exec(void) {
    return _entry();
}

/*
SKETCH OF BEHAVIOURS
getcontext:
    save all callee-saved registers into context
    save stack pointer into context
    save return address into context IP
    return
setcontext:
    load all callee-saved registers from context
    load stack pointer from context
    load target IP from context
    jump to target IP
makecontext:
    initialise all callee-saved registers zeros except A to target fn and B to arg
    set stack pointer to correct end of the allocated stack
    set target IP to _bootstrap
_bootstrap:
    move B to argument 1 register
    indirect call argument A
    cause explicit trap

struct context_x86_64 {
    uint64_t ip;
    uint64_t rbx;
    uint64_t rsp;
    uint64_t rbp;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
};

_SYS_context_get:
    movq (%rsp), %rax
    movq %rax, (%rdi)
    movq %rbx, 8(%rdi)
    movq %rsp, %rax
    subq $8, %rax
    movq %rax, 16(%rdi)
    movq %rbp, 24(%rdi)
    movq %r12, 32(%rdi)
    movq %r13, 40(%rdi)
    movq %r14, 48(%rdi)
    movq %r15, 56(%rdi)
    ret

_SYS_context_put:
    movq (%rdi), %rax
    movq 8(%rdi), %rbx
    movq 16(%rdi), %rsp
    movq 24(%rdi), %rbp
    movq 32(%rdi), %r12
    movq 40(%rdi), %r13
    movq 48(%rdi), %r14
    movq 56(%rdi), %r15

__SYS_context_bootstrap:
    movq %rbx, %rax
    movq %r12, %rdi
    callq (%rax)
    ud2

SYS_context_init(
    SYS_context* ctx,
    void (*callable)(void*),
    void* arg,
    void* stack,
    size_t stacksize
) {
    ctx->ip = &_SYS_context_bootstrap;
    ctx->rbx = (uint64_t)callable;
    ctx->rsp = (uint8_t*)stack + stacksize;
    ctx->rbp = (uint8_t*)stack + stacksize;
    ctx->r12 = (uint64_t)arg;
    ctx->r13 = 0;
    ctx->r14 = 0;
    ctx->r15 = 0;
}

*/

#include <stdio.h>
#include <stdatomic.h>

typedef _Atomic int lock_t;
static const int LOCK_INIT = 0;

static void lock_lock(lock_t* lock)
{
    bool got_lock;
    do {
        int expect = 0;
        got_lock = atomic_compare_exchange_weak_explicit(
            lock,
            &expect,
            1,
            memory_order_acquire,
            memory_order_relaxed
        );
        if (!got_lock) {
            K_wait((volatile int*)lock, expect);
        }
    } while (!got_lock);
}

static void lock_unlock(lock_t* lock)
{
    atomic_store_explicit(lock, 0, memory_order_release);
    K_wake_one((volatile int*)lock);
}

static lock_t the_lock = LOCK_INIT;
static lock_t the_other_lock = LOCK_INIT;

static void task_0(void* arg)
{
    (void)arg;
    lock_lock(&the_lock);
    K_sleep(200);
    lock_lock(&the_other_lock);
    K_sleep(200);
    lock_unlock(&the_other_lock);
    lock_unlock(&the_lock);
}

static void task_1(void* arg)
{
    (void)arg;
    lock_lock(&the_other_lock);
    K_sleep(200);
    lock_lock(&the_lock);
    K_sleep(200);
    lock_unlock(&the_lock);
    lock_unlock(&the_other_lock);
}

static char stack1[8192];
static char stack2[8192];

const struct task_def task_definitions[] = {
    {.execute = &task_0, .argument = NULL, .stack = stack1, .stacksize = sizeof(stack1)},
    {.execute = &task_1, .argument = NULL, .stack = stack2, .stacksize = sizeof(stack2)},
    {.execute = NULL, .argument = NULL, .stack = NULL, .stacksize = 0}
};

int main() {
    puts("Starting");
    int status = K_exec();
    printf("Done, status = %d\n", status);
    return 0;
}
