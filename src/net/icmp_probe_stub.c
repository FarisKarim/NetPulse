/*
 * ICMP Probe Stub Implementation
 *
 * This stub is used on platforms where ICMP probing is not supported (macOS).
 * All functions return "not available" status.
 */

#include "net/icmp_probe.h"
#include <stddef.h>

int icmp_probe_init(icmp_probe_state_t *state) {
    if (state != NULL) {
        state->sock = -1;
        state->identifier = 0;
        state->sequence = 0;
    }
    return -1;  // Not available
}

void icmp_probe_cleanup(icmp_probe_state_t *state) {
    if (state != NULL) {
        state->sock = -1;
    }
}

double icmp_probe_ping(icmp_probe_state_t *state, const char *host, int timeout_ms) {
    (void)state;
    (void)host;
    (void)timeout_ms;
    return -1.0;  // Not available
}

bool icmp_probe_available(void) {
    return false;
}

const char *icmp_probe_unavailable_reason(void) {
    return "ICMP probing not supported on this platform (macOS)";
}
