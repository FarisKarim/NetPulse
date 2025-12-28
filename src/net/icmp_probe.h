#ifndef NETPULSE_ICMP_PROBE_H
#define NETPULSE_ICMP_PROBE_H

#include <stdbool.h>
#include <stdint.h>

/*
 * ICMP Probe - Linux-only raw socket ICMP Echo implementation
 *
 * On Linux with CAP_NET_RAW: Uses real ICMP ping for true RTT measurement
 * On Linux without CAP_NET_RAW: Returns error, caller should fallback to TCP
 * On macOS: Stub implementation that returns "not available"
 */

typedef struct {
    int sock;               // Raw socket fd (-1 if not initialized)
    uint16_t identifier;    // ICMP identifier (typically PID)
    uint16_t sequence;      // Incrementing sequence number
} icmp_probe_state_t;

/*
 * Initialize ICMP probe state.
 *
 * Returns:
 *   0  - Success, ICMP probing available
 *  -1  - ICMP not available (no permission or not supported on platform)
 *
 * On failure, caller should fall back to TCP probing.
 */
int icmp_probe_init(icmp_probe_state_t *state);

/*
 * Clean up ICMP probe state.
 * Safe to call even if init failed or was never called.
 */
void icmp_probe_cleanup(icmp_probe_state_t *state);

/*
 * Send ICMP Echo Request and wait for reply.
 *
 * Parameters:
 *   state      - Initialized ICMP probe state
 *   host       - Target IP address (must be IPv4 dotted notation)
 *   timeout_ms - Maximum time to wait for reply
 *
 * Returns:
 *   >= 0  - RTT in milliseconds (success)
 *   < 0   - Probe failed (timeout, unreachable, or error)
 */
double icmp_probe_ping(icmp_probe_state_t *state, const char *host, int timeout_ms);

/*
 * Check if ICMP probing is available on this platform.
 * Does not require initialization.
 */
bool icmp_probe_available(void);

/*
 * Get human-readable reason why ICMP is not available.
 * Returns static string, do not free.
 */
const char *icmp_probe_unavailable_reason(void);

#endif // NETPULSE_ICMP_PROBE_H
