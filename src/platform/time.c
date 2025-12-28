#define _POSIX_C_SOURCE 200809L

#include "platform/platform.h"
#include <sys/time.h>

#ifdef PLATFORM_MACOS
#include <mach/mach_time.h>
#include <stdbool.h>

static mach_timebase_info_data_t timebase_info;
static bool timebase_initialized = false;

uint64_t now_ns(void) {
    if (!timebase_initialized) {
        mach_timebase_info(&timebase_info);
        timebase_initialized = true;
    }

    uint64_t ticks = mach_absolute_time();
    // Convert ticks to nanoseconds
    return ticks * timebase_info.numer / timebase_info.denom;
}

#else // Linux and other POSIX systems

#include <time.h>

uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

#endif

// Wall-clock time using gettimeofday (works on both macOS and Linux)
uint64_t wall_clock_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)tv.tv_usec / 1000ULL;
}
