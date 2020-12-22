#include <stdatomic.h>
#include <stdio.h>

#include "lock.h"
#include "rtos.h"

static lock_t the_lock = LOCK_INIT;
static lock_t the_other_lock = LOCK_INIT;

static void
task_0(void* arg)
{
    (void)arg;
    lock_lock(&the_lock);
    K_sleep(200);
    lock_lock(&the_other_lock);
    K_sleep(200);
    lock_unlock(&the_other_lock);
    lock_unlock(&the_lock);
}

static void
task_1(void* arg)
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

static const struct task_def task_definitions[] = {
    { .execute = &task_0,
        .argument = NULL,
        .stack = stack1,
        .stacksize = sizeof(stack1) },
    { .execute = &task_1,
        .argument = NULL,
        .stack = stack2,
        .stacksize = sizeof(stack2) },
    { .execute = NULL, .argument = NULL, .stack = NULL, .stacksize = 0 }
};

int main()
{
    puts("Starting");
    int status = K_exec(task_definitions);
    printf("Done, status = %d\n", status);
    return 0;
}