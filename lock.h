#ifndef __INCLUDED_LOCK_H
#define __INCLUDED_LOCK_H

#include <stdatomic.h>
#include <stdbool.h>

#include "rtos.h"

typedef _Atomic int lock_t;

#define LOCK_INIT 0

void lock_lock(kernel_t kernel, lock_t* lock);
bool lock_trylock(kernel_t kernel, lock_t* lock);
void lock_unlock(kernel_t kernel, lock_t* lock);

#endif
