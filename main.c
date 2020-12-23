#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#include "lock.h"
#include "posix_timebase.h"
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

static void spawn(kernel_t kernel, void (*callback)(kernel_t, void*), void* arg)
{
    const int STACK_SIZE = 8192;
    void* stack = malloc(STACK_SIZE);
    struct task_def def = {
        .execute = callback,
        .argument = arg,
        .stack = stack,
        .stacksize = STACK_SIZE
    };
    K_spawn(kernel, &def, freestack, stack);
}

static void
task_2(kernel_t kernel, void* arg)
{
    for (int i = 0; i < 10; ++i) {
        printf("Task 2: %d\n", i);
        K_sleep(kernel, 250);
        if (i == 3) {
            spawn(kernel, &subtask, NULL);
        }
    }
}

static void root_task(kernel_t kernel, void* arg)
{
    spawn(kernel, task_0, NULL);
    spawn(kernel, task_1, NULL);
    spawn(kernel, task_2, NULL);
}

char root_stack[4096];

int main()
{
    puts("Starting");
    struct task_def root = {
        .execute = root_task,
        .argument = NULL,
        .stack = root_stack,
        .stacksize = sizeof(root_stack)
    };
    int status = kernel_exec(&root, &TIMEBASE_POSIX);
    printf("Done, status = %d\n", status);
    return 0;
}
