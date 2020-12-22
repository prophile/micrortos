#ifndef __INCLUDED_LOCK_H
#define __INCLUDED_LOCK_H

#include <stdatomic.h>
#include <stdbool.h>
#include "rtos.h"

typedef _Atomic int lock_t;

#define LOCK_INIT 0

void lock_lock(lock_t* lock);
bool lock_trylock(lock_t* lock);
void lock_unlock(lock_t* lock);

#endif
