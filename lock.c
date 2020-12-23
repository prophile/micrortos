#include "lock.h"

void lock_lock(kernel_t kernel, lock_t* lock)
{
    bool got_lock;
    do {
        int expectation = 0;
        got_lock = atomic_compare_exchange_weak_explicit(
            lock, &expectation, 1, memory_order_acquire, memory_order_relaxed);
        if (!got_lock && expectation == 1) {
            (void)K_wait(kernel, (volatile int*)lock, 1);
        }
    } while (!got_lock);
}

bool lock_trylock(kernel_t kernel, lock_t* lock)
{
    (void)kernel;
    int swapped = atomic_exchange_explicit(lock, 1, memory_order_acquire);
    return swapped == 0;
}

void lock_unlock(kernel_t kernel, lock_t* lock)
{
    atomic_store_explicit(lock, 0, memory_order_release);
    K_wake_one(kernel, (volatile int*)lock);
}
