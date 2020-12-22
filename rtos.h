#ifndef __INCLUDED_RTOS_H
#define __INCLUDED_RTOS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct task_def {
    void (*execute)(void*);
    void* argument;
    void* stack;
    size_t stacksize;
};
extern const struct task_def task_definitions[];

typedef uint32_t milliseconds_t;

bool K_wait(volatile int* address, int value);
bool K_wait_timeout(volatile int* address, int value, milliseconds_t timeout);
void K_wake_one(volatile int* address);
void K_wake_all(volatile int* address);
int K_gettid(void);
void K_yield(void);
void K_exit(void);
void K_exitall(void);
void K_sleep(milliseconds_t interval);

int K_exec(void);
static const int K_EXITALL = -2;
static const int K_DEADLOCK = -3;
static const int K_UNKNOWN = -4;

#endif
