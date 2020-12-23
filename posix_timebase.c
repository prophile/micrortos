#include "posix_timebase.h"

#include <time.h>

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

const struct kernel_timebase TIMEBASE_POSIX = {
    .ud = NULL,
    .get_time = &tb_get_time,
    .from_ms = &tb_from_ms,
    .delay = &tb_delay
};
