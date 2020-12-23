#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "lock.h"
#include "rtos.h"

static lock_t the_lock = LOCK_INIT;
static lock_t the_other_lock = LOCK_INIT;

static void
task_0(kernel_t kernel, void* arg)
{
    (void)arg;
    lock_lock(kernel, &the_lock);
    K_sleep(kernel, 200);
    lock_lock(kernel, &the_other_lock);
    K_sleep(kernel, 200);
    lock_unlock(kernel, &the_other_lock);
    lock_unlock(kernel, &the_lock);
}

static void
task_1(kernel_t kernel, void* arg)
{
    (void)arg;
    lock_lock(kernel, &the_other_lock);
    K_sleep(kernel, 200);
    lock_lock(kernel, &the_lock);
    K_sleep(kernel, 200);
    lock_unlock(kernel, &the_lock);
    lock_unlock(kernel, &the_other_lock);
}

static void
subtask(kernel_t kernel, void* arg)
{
    (void)arg;
    puts("Subtask!");
    K_sleep(kernel, 1000);
    puts("Done with subtask");
}

static void freestack(kernel_t kernel, void* ptr)
{
    free(ptr);
}

static void
task_2(kernel_t kernel, void* arg)
{
    for (int i = 0; i < 10; ++i) {
        printf("Task 2: %d\n", i);
        K_sleep(kernel, 250);
        if (i == 3) {
            void* stack = malloc(8192);
            struct task_def definition = {
                .execute = &subtask,
                .argument = NULL,
                .stack = stack,
                .stacksize = 8192
            };
            K_spawn(kernel, &definition, freestack, stack);
        }
    }
}

static char stack1[8192];
static char stack2[8192];
static char stack3[8192];

static const struct task_def task_definitions[] = {
    { .execute = &task_0,
        .argument = NULL,
        .stack = stack1,
        .stacksize = sizeof(stack1) },
    { .execute = &task_1,
        .argument = NULL,
        .stack = stack2,
        .stacksize = sizeof(stack2) },
    { .execute = &task_2,
        .argument = NULL,
        .stack = stack3,
        .stacksize = sizeof(stack3) }
};

static void root_task(kernel_t kernel, void* arg)
{
    (void)arg;
    K_spawn(kernel, &task_definitions[0], NULL, NULL);
    K_spawn(kernel, &task_definitions[1], NULL, NULL);
    K_spawn(kernel, &task_definitions[2], NULL, NULL);
}

char root_stack[4096];

static uint64_t tb_get_time(void* ud)
{
    (void)ud;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t units = 0;
    units += ts.tv_nsec;
    units += ts.tv_sec * 1000000000;
    return units;
}

static uint64_t tb_from_ms(milliseconds_t ms, void* ud)
{
    (void)ud;
    return (uint64_t)ms * 1000000;
}

static void tb_delay(uint64_t pause, void* ud)
{
    (void)ud;
    struct timespec ts;
    ts.tv_nsec = (pause % 1000000000);
    ts.tv_sec = (pause / 1000000000);
    nanosleep(&ts, NULL);
}

static const struct kernel_timebase timebase = {
    .ud = NULL,
    .get_time = &tb_get_time,
    .from_ms = &tb_from_ms,
    .delay = &tb_delay
};

int main()
{
    puts("Starting");
    struct task_def root = {
        .execute = root_task,
        .argument = NULL,
        .stack = root_stack,
        .stacksize = sizeof(root_stack)
    };
    int status = kernel_exec(&root, &timebase);
    printf("Done, status = %d\n", status);
    return 0;
}
