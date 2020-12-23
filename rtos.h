#ifndef __INCLUDED_RTOS_H
#define __INCLUDED_RTOS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PUBLIC __attribute__((visibility("default")))

struct task_def {
    void (*execute)(void*);
    void* argument;
    void* stack;
    size_t stacksize;
};

typedef void (*cleanup_callback_t)(void*);

typedef uint32_t milliseconds_t;

typedef struct kernel* kernel_t;

bool K_wait(kernel_t kernel, volatile int* address, int value) PUBLIC;
bool K_wait_timeout(kernel_t kernel, volatile int* address, int value, milliseconds_t timeout) PUBLIC;
void K_wake_one(kernel_t kernel, volatile int* address) PUBLIC;
void K_wake_all(kernel_t kernel, volatile int* address) PUBLIC;
void K_yield(kernel_t kernel) PUBLIC;
void K_exit(kernel_t kernel) PUBLIC;
void K_exitall(kernel_t kernel) PUBLIC;
void K_sleep(kernel_t kernel, milliseconds_t interval) PUBLIC;
void K_spawn(
    kernel_t kernel,
    const struct task_def* definition,
    cleanup_callback_t cleanback,
    void* ud) PUBLIC;

kernel_t kernel_create(const struct task_def* task) PUBLIC;
int kernel_exec(kernel_t kernel) PUBLIC;

static const int K_EXITALL = -2;
static const int K_DEADLOCK = -3;
static const int K_UNKNOWN = -4;

#endif
