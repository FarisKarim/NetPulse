#ifndef NETPULSE_PLATFORM_H
#define NETPULSE_PLATFORM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * Cross-platform timing
 */

// Returns current monotonic time in nanoseconds (for measuring intervals)
uint64_t now_ns(void);

// Convenience: monotonic time in milliseconds (for measuring intervals)
static inline uint64_t now_ms(void) {
    return now_ns() / 1000000ULL;
}

// Returns current wall-clock time in milliseconds since Unix epoch
// Use this for timestamps that will be displayed to users
uint64_t wall_clock_ms(void);

/*
 * Filesystem utilities
 */

// Create directory and all parent directories (like mkdir -p)
// Returns 0 on success, -1 on error
int mkdir_p(const char *path);

// Expand ~ to home directory. Caller must free returned string.
// Returns NULL on error.
char *expand_home_path(const char *path);

// Get the netpulse data directory (~/.netpulse/)
// Creates it if it doesn't exist. Caller must free returned string.
// Returns NULL on error.
char *get_data_dir(void);

#endif // NETPULSE_PLATFORM_H
