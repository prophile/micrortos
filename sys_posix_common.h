#ifndef __INCLUDED_SYS_POSIX_COMMON_H
#define __INCLUDED_SYS_POSIX_COMMON_H

#include <time.h>
#include <unistd.h>
#include "rtos.h"

static inline void SYS_intr_enable(void)
{
}

static inline void SYS_intr_disable(void)
{
}

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
#define  CLK_AFTER ctime_after
#define  CLK_NONZERO(a) CLK_AFTER((a), CLK_ZERO)
#define  CLK_ADD(a, b) { (a) = ctime_add((a), (b)); }
#define  CLK_SUB(a, b) { (a) = ctime_sub((a), (b)); }

#endif
