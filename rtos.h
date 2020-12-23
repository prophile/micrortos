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

bool K_wait(volatile int* address, int value) PUBLIC;
bool K_wait_timeout(volatile int* address, int value, milliseconds_t timeout) PUBLIC;
void K_wake_one(volatile int* address) PUBLIC;
void K_wake_all(volatile int* address) PUBLIC;
void K_yield(void) PUBLIC;
void K_exit(void) PUBLIC;
void K_exitall(void) PUBLIC;
void K_sleep(milliseconds_t interval) PUBLIC;
void K_spawn(const struct task_def* definition, cleanup_callback_t cleanback, void* ud) PUBLIC;

int K_exec(const struct task_def* task) PUBLIC;

static const int K_EXITALL = -2;
static const int K_DEADLOCK = -3;
static const int K_UNKNOWN = -4;

#endif
